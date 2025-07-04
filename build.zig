const std = @import("std");

const version = std.SemanticVersion{ .major = 0, .minor = 0, .patch = 0 };

const enabled_systems: packed struct {
    renderer: bool = true,
    input: bool = true,
    ui: bool = false,
    audio: bool = false,
    physics: bool = false,
    animation: bool = false,
} = .{};

const debug_flags = .{
    "-pedantic-errors",
    "-Wc++11-extensions",
    "-std=c++20",
    "-g",
};

const release_flags = .{
    "-pedantic-errors",
    "-Wc++11-extensions",
    "-std=c++20",
    "-g",
};

const BuildMode = enum { debug, release };

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const build_mode = b.option(BuildMode, "build_mode", "build mode (default = debug)") orelse .debug;
    const flags = if (build_mode == .debug) &debug_flags else &release_flags;

    const options = b.addOptions();
    options.addOption(BuildMode, "build_mode", build_mode);

    const flux = b.createModule(.{
        .target = target,
        .optimize = if (build_mode == .debug) .Debug else .ReleaseFast,
        .link_libcpp = true,
    });

    const exe = b.addExecutable(.{
        .root_module = flux,
        .name = "runner",
        .use_lld = true,
        .use_llvm = true,
    });
    exe.addCSourceFile(.{ .file = b.path("src/main.cpp"), .flags = flags });

    const lib = b.addLibrary(.{
        .root_module = flux,
        .name = "flux",
        .linkage = .dynamic,
        .use_lld = true,
        .use_llvm = true,
        .version = version,
    });
    lib.addCSourceFile(.{ .file = b.path("src/core/engine.cpp"), .flags = flags });
    if (enabled_systems.renderer) lib.addCSourceFile(.{ .file = b.path("src/renderer/renderer.cpp"), .flags = flags });
    if (enabled_systems.input) lib.addCSourceFile(.{ .file = b.path("src/input/input.cpp"), .flags = flags });
    if (enabled_systems.ui) lib.addCSourceFile(.{ .file = b.path("src/ui/ui.cpp"), .flags = flags });
    if (enabled_systems.audio) lib.addCSourceFile(.{ .file = b.path("src/audio/audio.cpp"), .flags = flags });
    if (enabled_systems.physics) lib.addCSourceFile(.{ .file = b.path("src/physics/physics.cpp"), .flags = flags });
    if (enabled_systems.animation) lib.addCSourceFile(.{ .file = b.path("src/animation/animation.cpp"), .flags = flags });

    // vulkan
    var env_map = try std.process.getEnvMap(b.allocator);
    defer env_map.deinit();
    const path = env_map.get("VK_SDK_PATH") orelse @panic("failed to get value from envmap");

    flux.linkSystemLibrary(if (target.result.os.tag == .windows) "vulkan-1" else "vulkan", .{});
    flux.addLibraryPath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) });
    flux.addIncludePath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) });
    flux.addLibraryPath(b.path("deps/lib"));
    flux.linkSystemLibrary("glfw3", .{});
    flux.addIncludePath(b.path("src"));
    flux.addIncludePath(b.path("deps/include"));

    try compile_shaders(b, lib);
    b.installArtifact(lib);

    if (target.result.os.tag == .windows) {
        b.installBinFile("deps/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("deps/lib/glfw3.so", "glfw3.so.0");
        flux.addRPathSpecial("$ORIGIN");
    }

    // setup runner and start runner
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.stdio = .inherit;
    run_cmd.step.dependOn(&b.addInstallArtifact(exe, .{}).step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);
    const run_step = b.step("run", "Start the runner");
    run_step.dependOn(&run_cmd.step);
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
