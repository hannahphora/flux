const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "flux",
        .root_source_file = null,
        .target = target,
        .optimize = optimize,
        .use_lld = true,
    });
    exe.addCSourceFiles(.{ .files = &.{
        "src/main.cpp",
        "src/core/engine.cpp",
        "src/log/log.cpp",
        "src/renderer/renderer.cpp",
        "src/input/input.cpp",
    }, .flags = &.{
        "-pedantic-errors",
        "-Wc++11-extensions",
        "-std=c++20",
        "-g",
    } });

    // vulkan
    const vk_lib_name = if (target.result.os.tag == .windows) "vulkan-1" else "vulkan";
    exe.linkSystemLibrary(vk_lib_name);
    const env_map = try std.process.getEnvMap(b.allocator);
    if (env_map.get("VK_SDK_PATH")) |path| {
        exe.addLibraryPath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) });
        exe.addIncludePath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) });
    }

    // linking
    exe.linkLibCpp();
    exe.addLibraryPath(b.path("deps/lib"));
    exe.linkSystemLibrary("glfw3");

    // includes
    exe.addIncludePath(b.path("src"));
    exe.addIncludePath(b.path("deps/include"));

    try compile_shaders(b, exe);

    b.installArtifact(exe);
    // copy glfw to out dir
    if (target.result.os.tag == .windows) {
        b.installBinFile("deps/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("deps/lib/glfw3.so", "glfw3.so.0");
        exe.root_module.addRPathSpecial("$ORIGIN");
    }

    // add run option
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);
    const run_step = b.step("run", "Run the program");
    run_step.dependOn(&run_cmd.step);
}

fn compile_shaders(b: *std.Build, exe: *std.Build.Step.Compile) !void {
    var shaders_dir = try b.build_root.handle.openDir("shaders", .{ .iterate = true });
    defer shaders_dir.close();

    var itr = shaders_dir.iterate();
    while (try itr.next()) |entry| {
        if (entry.kind == .file and std.mem.eql(u8, std.fs.path.extension(entry.name), ".slang")) {
            const name = std.fs.path.stem(entry.name);
            std.debug.print("Found shader: {s}\n", .{name});
            const source = try std.fmt.allocPrint(b.allocator, "shaders/{s}.slang", .{name});
            const outpath = try std.fmt.allocPrint(b.allocator, "shaders/{s}.spv", .{name});

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

            exe.step.dependOn(&shader_compilation.step);
            exe.step.dependOn(&b.addInstallBinFile(output, outpath).step);
        }
    }
}
