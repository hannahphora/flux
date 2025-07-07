const std = @import("std");

const systems: packed struct {
    renderer: bool = true,
    input: bool = true,
    ui: bool = false,
    audio: bool = false,
    physics: bool = false,
    animation: bool = false,
} = .{};

const flags = .{
    "-pedantic-errors",
    "-Wc++11-extensions",
    "-std=c++20",
    "-g",
};

const BuildMode = enum { debug, release };

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const mode = b.option(BuildMode, "mode", "select build mode (default = debug)") orelse .debug;

    const options = b.addOptions();
    options.addOption(BuildMode, "mode", mode);

    const flux = b.createModule(.{
        .target = target,
        .optimize = if (mode == .debug) .Debug else .ReleaseFast,
        .link_libcpp = true,
    });

    const host = b.addExecutable(.{ .root_module = flux, .name = "host" });
    host.addCSourceFile(.{ .file = b.path("src/host.cpp"), .flags = &flags });

    const engine = b.addLibrary(.{
        .root_module = flux,
        .name = "flux",
        .linkage = .dynamic,
    });

    var src_files = try std.ArrayList([]const u8).initCapacity(b.allocator, @bitSizeOf(@TypeOf(systems)) + 1);
    defer src_files.deinit();
    src_files.appendAssumeCapacity("core/engine.cpp");
    if (systems.renderer) src_files.appendAssumeCapacity("renderer/renderer.cpp");
    if (systems.input) src_files.appendAssumeCapacity("input/input.cpp");
    if (systems.ui) src_files.appendAssumeCapacity("ui/ui.cpp");
    if (systems.audio) src_files.appendAssumeCapacity("audio/audio.cpp");
    if (systems.physics) src_files.appendAssumeCapacity("physics/physics.cpp");
    if (systems.animation) src_files.appendAssumeCapacity("animation/animation.cpp");
    engine.addCSourceFiles(.{
        .files = src_files.items,
        .flags = &flags,
        .root = b.path("src"),
    });

    // get vulkan sdk path from env
    var env_map = try std.process.getEnvMap(b.allocator);
    defer env_map.deinit();
    const path = env_map.get("VK_SDK_PATH") orelse @panic("failed to get VK_SDK_PATH from envmap");

    flux.linkSystemLibrary(if (target.result.os.tag == .windows) "vulkan-1" else "vulkan", .{});
    flux.addLibraryPath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) });
    flux.addIncludePath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) });
    flux.addLibraryPath(b.path("deps/lib"));
    flux.linkSystemLibrary("glfw3", .{});
    flux.addIncludePath(b.path("src"));
    flux.addIncludePath(b.path("deps/include"));

    try compile_shaders(b, engine);
    b.installArtifact(engine);

    if (target.result.os.tag == .windows) {
        b.installBinFile("deps/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("deps/lib/glfw3.so", "glfw3.so.0");
        flux.addRPathSpecial("$ORIGIN");
    }

    // add run host option
    const run = b.addRunArtifact(host);
    run.stdio = .inherit;
    run.step.dependOn(&b.addInstallArtifact(host, .{}).step);
    run.step.dependOn(b.getInstallStep());
    if (b.args) |args| run.addArgs(args);
    b.step("run", "Run the host application").dependOn(&run.step);
}

fn compile_shaders(b: *std.Build, lib: *std.Build.Step.Compile) !void {
    var shaders_dir = try b.build_root.handle.openDir("res/shaders", .{ .iterate = true });
    defer shaders_dir.close();

    var itr = shaders_dir.iterate();
    while (try itr.next()) |entry| {
        if (entry.kind == .file and std.mem.eql(u8, std.fs.path.extension(entry.name), ".slang")) {
            const name = std.fs.path.stem(entry.name);
            const source = try std.fmt.allocPrint(b.allocator, "res/shaders/{s}.slang", .{name});
            const outpath = try std.fmt.allocPrint(b.allocator, "res/shaders/{s}.spv", .{name});

            const shader_compilation = b.addSystemCommand(&.{"slangc"});
            shader_compilation.addArgs(&.{
                "-target",
                "spirv",
                "-profile",
                "spirv_1_4",
                "-fvk-use-entrypoint-name",
                "-o",
            });
            const output = shader_compilation.addOutputFileArg(outpath);
            shader_compilation.addFileArg(b.path(source));

            lib.step.dependOn(&b.addInstallBinFile(output, outpath).step);
        }
    }
}
