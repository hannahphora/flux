const std = @import("std");

const BuildMode = enum { debug, release };

const flags = .{
    "-pedantic-errors",
    "-Wc++11-extensions",
    "-std=c++20",
    "-g",
};

var b: *std.Build = undefined;
var verbose: bool = undefined;

pub fn build(builder: *std.Build) !void {
    b = builder;
    const options = b.addOptions();
    const target = b.standardTargetOptions(.{});
    verbose = b.option(bool, "verbose", "enable verbose logging") orelse false;
    const build_mode = b.option(BuildMode, "build_mode", "build mode") orelse .debug;
    options.addOption(BuildMode, "build_mode", build_mode);

    const flux = b.createModule(.{
        .target = target,
        .optimize = if (build_mode == .debug) .Debug else .ReleaseFast,
        .link_libcpp = true,
    });

    const host = b.addExecutable(.{
        .root_module = flux,
        .name = "host",
    });
    host.addCSourceFile(.{ .file = b.path("src/host.cpp"), .flags = &flags });

    const engine = b.addLibrary(.{
        .root_module = flux,
        .name = "flux",
        .linkage = .dynamic,
    });
    engine.addCSourceFiles(.{ .files = &.{
        "src/core/engine.cpp",
        "src/renderer/renderer.cpp",
        "src/input/input.cpp",
    }, .flags = &flags });
    try addCSourceFilesRecursive(engine, "deps/src");

    var env_map = try std.process.getEnvMap(b.allocator);
    defer env_map.deinit();
    const path = env_map.get("VK_SDK_PATH") orelse @panic("failed to get from envmap");

    flux.linkSystemLibrary(if (target.result.os.tag == .windows) "vulkan-1" else "vulkan", .{});
    flux.addLibraryPath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) });
    flux.addIncludePath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) });
    flux.addLibraryPath(b.path("deps/lib"));
    flux.linkSystemLibrary("glfw3", .{});
    flux.addIncludePath(b.path("src"));

    flux.addIncludePath(b.path("deps/include"));
    flux.addIncludePath(b.path("deps/include/meshoptimizer"));

    try compileShaders();

    b.installArtifact(engine);

    if (target.result.os.tag == .windows) {
        b.installBinFile("deps/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("deps/lib/glfw3.so", "glfw3.so.0");
        flux.addRPathSpecial("$ORIGIN");
    }

    // run host
    const run = b.addRunArtifact(host);
    run.step.dependOn(&b.addInstallArtifact(host, .{}).step);
    run.step.dependOn(b.getInstallStep());
    run.stdio = .inherit;
    run.setCwd(host.getEmittedBinDirectory());
    b.step("run", "Run the host").dependOn(&run.step);
}

fn compileShaders() !void {
    var dir = try b.build_root.handle.openDir("res/shaders", .{ .iterate = true });
    defer dir.close();
    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    while (try walker.next()) |entry| {
        if (entry.kind == .file and std.mem.eql(u8, std.fs.path.extension(entry.basename), ".slang")) {
            const name = std.fs.path.stem(entry.basename);
            if (verbose) std.debug.print("Found shader: {s}\n", .{name});
            const source = try std.fmt.allocPrint(b.allocator, "res/shaders/{s}.slang", .{name});
            const outpath = try std.fmt.allocPrint(b.allocator, "res/shaders/{s}.spv", .{name});

            const shader_compilation = b.addSystemCommand(&.{"slangc"});
            shader_compilation.addArgs(&.{
                "-target",
                "spirv",
                "-fvk-use-entrypoint-name",
                "-o",
            });
            const output = shader_compilation.addOutputFileArg(outpath);
            shader_compilation.addFileArg(b.path(source));
            b.getInstallStep().dependOn(&b.addInstallBinFile(output, outpath).step);
        }
    }
}

fn addCSourceFilesRecursive(c: *std.Build.Step.Compile, path: []const u8) !void {
    var dir = try b.build_root.handle.openDir(path, .{ .iterate = true });
    defer dir.close();
    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    while (try walker.next()) |entry| {
        if (entry.kind == .file and
            (std.mem.eql(u8, std.fs.path.extension(entry.basename), ".c") or
                std.mem.eql(u8, std.fs.path.extension(entry.basename), ".cpp")))
        {
            if (verbose) std.debug.print("Found source file: {s}\n", .{entry.basename});
            const src_path = try std.fmt.allocPrint(b.allocator, "deps/src/{s}", .{entry.path});
            c.addCSourceFile(.{ .file = b.path(src_path), .flags = &flags });
        }
    }
}
