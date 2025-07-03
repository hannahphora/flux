const std = @import("std");
const builtin = @import("builtin");

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
        "src/core/log/log.cpp",
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
        exe.addLibraryPath(.{ .cwd_relative = std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) catch @panic("OOM") });
        exe.addIncludePath(.{ .cwd_relative = std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) catch @panic("OOM") });
    }

    // linking
    exe.linkLibCpp();
    exe.addLibraryPath(b.path("deps/lib"));
    exe.linkSystemLibrary("glfw3");

    // includes
    exe.addIncludePath(b.path("src/"));
    exe.addIncludePath(b.path("deps/include"));

    compile_shaders(b, exe);

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
    const run_step = b.step("run", "Run the engine");
    run_step.dependOn(&run_cmd.step);

    // TODO: add unit tests step
}

fn compile_shaders(b: *std.Build, exe: *std.Build.Step.Compile) void {
    const shaders_dir = if (@hasDecl(@TypeOf(b.build_root.handle), "openIterableDir"))
        b.build_root.handle.openIterableDir("shaders", .{}) catch @panic("Failed to open shaders dir")
    else
        b.build_root.handle.openDir("shaders", .{ .iterate = true }) catch @panic("Failed to open shaders dir");

    var file_it = shaders_dir.iterate();
    while (file_it.next() catch @panic("Failed to iterate shader dir")) |entry| {
        if (entry.kind == .file) {
            const ext = std.fs.path.extension(entry.name);
            if (std.mem.eql(u8, ext, ".slang")) {
                const basename = std.fs.path.basename(entry.name);
                const name = basename[0 .. basename.len - ext.len];

                std.debug.print("Found shader: {s}\n", .{entry.name});
                const source = std.fmt.allocPrint(b.allocator, "shaders/{s}.slang", .{name}) catch @panic("OOM");
                const outpath = std.fmt.allocPrint(b.allocator, "shaders/{s}.spv", .{name}) catch @panic("OOM");

                // add compile step for shader
                const shader_comp = b.addSystemCommand(&.{"slangc"});
                shader_comp.addArgs(&.{
                    "-target",
                    "spirv",
                    "-profile",
                    "spirv_1_4",
                    "-fvk-use-entrypoint-name",
                    "-o",
                    outpath,
                    source,
                });
                exe.step.dependOn(&shader_comp.step);
                b.installBinFile(outpath, outpath);
            }
        }
    }
}
