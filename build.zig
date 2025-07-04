const std = @import("std");
const config = @import("config.zig");

pub const BuildMode = enum { debug, release };

pub const debug_flags = .{
    "-pedantic-errors",
    "-Wc++11-extensions",
    "-std=c++20",
    "-g",
};

pub const release_flags = .{
    "-pedantic-errors",
    "-Wc++11-extensions",
    "-std=c++20",
    "-g",
};

pub fn build(b: *std.Build) !void {
    const options = b.addOptions();
    const target = b.standardTargetOptions(.{});
    const build_mode = b.option(BuildMode, "build_mode", "build mode (default = debug)") orelse .debug;
    options.addOption(BuildMode, "build_mode", build_mode);

    const flags = if (build_mode == .debug) &debug_flags else &release_flags;

    const mod = b.createModule(.{
        .target = target,
        .optimize = if (build_mode == .debug) .Debug else .ReleaseFast,
        .link_libcpp = true,
    });

    const runner = b.addExecutable(.{
        .root_module = mod,
        .name = "runner",
        .use_lld = true,
        .use_llvm = true,
    });
    runner.addCSourceFile(.{ .file = b.path("src/main.cpp"), .flags = flags });

    const flux = b.addLibrary(.{
        .root_module = mod,
        .name = "flux",
        .linkage = .dynamic,
        .use_lld = true,
        .use_llvm = true,
    });
    flux.addCSourceFile(.{ .file = b.path("src/core/engine.cpp"), .flags = flags });
    if (config.enabled_systems.renderer) flux.addCSourceFile(.{ .file = b.path("src/renderer/renderer.cpp"), .flags = flags });
    if (config.enabled_systems.input) flux.addCSourceFile(.{ .file = b.path("src/input/input.cpp"), .flags = flags });
    if (config.enabled_systems.ui) flux.addCSourceFile(.{ .file = b.path("src/ui/ui.cpp"), .flags = flags });
    if (config.enabled_systems.audio) flux.addCSourceFile(.{ .file = b.path("src/audio/audio.cpp"), .flags = flags });
    if (config.enabled_systems.physics) flux.addCSourceFile(.{ .file = b.path("src/physics/physics.cpp"), .flags = flags });
    if (config.enabled_systems.animation) flux.addCSourceFile(.{ .file = b.path("src/animation/animation.cpp"), .flags = flags });

    // vulkan
    var env_map = try std.process.getEnvMap(b.allocator);
    defer env_map.deinit();
    const path = env_map.get("VK_SDK_PATH") orelse @panic("failed to get from envmap");

    mod.linkSystemLibrary(if (target.result.os.tag == .windows) "vulkan-1" else "vulkan", .{});
    mod.addLibraryPath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) });
    mod.addIncludePath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) });
    mod.addLibraryPath(b.path("deps/lib"));
    mod.linkSystemLibrary("glfw3", .{});
    mod.addIncludePath(b.path("src"));
    mod.addIncludePath(b.path("deps/include"));

    try compile_shaders(b);

    b.installArtifact(flux);
    b.installArtifact(runner);

    if (target.result.os.tag == .windows) {
        b.installBinFile("deps/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("deps/lib/glfw3.so", "glfw3.so.0");
        mod.addRPathSpecial("$ORIGIN");
    }

    // add run option
    const run_cmd = b.addRunArtifact(runner);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);
    const run_step = b.step("run", "Run the program");
    run_step.dependOn(&run_cmd.step);
}

fn compile_shaders(b: *std.Build) !void {
    var shaders_dir = try b.build_root.handle.openDir("res/shaders", .{ .iterate = true });
    defer shaders_dir.close();

    var itr = shaders_dir.iterate();
    while (try itr.next()) |entry| {
        if (entry.kind == .file and std.mem.eql(u8, std.fs.path.extension(entry.name), ".slang")) {
            const name = std.fs.path.stem(entry.name);
            std.debug.print("Found shader: {s}\n", .{name});
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

            b.getInstallStep().dependOn(&b.addInstallBinFile(output, outpath).step);
        }
    }
}
