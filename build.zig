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
    const cflags = [_][]const u8{
        "-pedantic-errors",
        "-Wc++11-extensions",
        "-std=c++17",
        "-g",
    };

    // source files
    exe.addCSourceFile(.{ .file = b.path("src/main.cpp"), .flags = &cflags });

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

    b.installArtifact(exe);
    // copy glfw to out dir
    if (target.result.os.tag == .windows) {
        b.installBinFile("deps/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("deps/lib/glfw3.so", "glfw3.so.0");
        exe.root_module.addRPathSpecial("$ORIGIN");
    }

    // run option
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);
    const run_step = b.step("run", "Run the engine");
    run_step.dependOn(&run_cmd.step);
}
