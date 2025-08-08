const std = @import("std");
const zcc = @import("compile_commands");

const additional_flags: []const []const u8 = &.{
    "-std=c++20",
    "-Wno-unused-parameter",
    "-Wno-missing-field-initializers",
};
const debug_flags = warning_flags ++ additional_flags;

var b: *std.Build = undefined;
var verbose: bool = undefined;

pub fn build(builder: *std.Build) !void {
    b = builder;
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    verbose = b.option(bool, "verbose", "enable verbose logging") orelse false;
    const hot_reloading = b.option(bool, "hot_reloading", "enables hot reloading (builds flux as dynamic lib)") orelse true;

    const flux = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    const host = b.addExecutable(.{
        .root_module = flux,
        .name = "host",
    });
    host.addCSourceFile(.{ .file = b.path("src/host.cpp"), .flags = debug_flags });

    const engine = b.addLibrary(.{
        .root_module = flux,
        .name = "flux",
        .linkage = if (hot_reloading) .dynamic else .static,
    });
    engine.addCSourceFiles(.{ .files = &.{
        "src/core/engine.cpp",
        "src/renderer/renderer.cpp",
        "src/input/input.cpp",
    }, .flags = debug_flags });

    try addCSourceFilesInDir(engine, "deps/include/vkb", &.{});
    try addCSourceFilesInDir(engine, "deps/include/meshoptimizer", &.{});

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

    if (hot_reloading) flux.addCMacro("HOT_RELOADING", "");

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
    if (hot_reloading) {
        run.stdio = .inherit;
        run.setCwd(.{ .cwd_relative = b.exe_dir });
    }
    b.step("run", "Run the host").dependOn(&run.step);

    // generate compile_commands.json
    var targets = std.ArrayList(*std.Build.Step.Compile).init(b.allocator);
    defer targets.deinit();
    try targets.append(host);
    try targets.append(engine);
    _ = zcc.createStep(b, "cmds", try targets.toOwnedSlice());
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

fn addCSourceFilesInDir(c: *std.Build.Step.Compile, path: []const u8, flags: []const []const u8) !void {
    var dir = try b.build_root.handle.openDir(path, .{ .iterate = true });
    defer dir.close();

    var itr = dir.iterate();
    while (try itr.next()) |entry| {
        if (entry.kind == .file and
            (std.mem.eql(u8, std.fs.path.extension(entry.name), ".c") or
                std.mem.eql(u8, std.fs.path.extension(entry.name), ".cpp")))
        {
            if (verbose) std.debug.print("Found source file: {s}\n", .{entry.name});
            const src_path = try std.fmt.allocPrint(b.allocator, "{s}/{s}", .{ path, entry.name });
            c.addCSourceFile(.{ .file = b.path(src_path), .flags = flags });
        }
    }
}

const runtime_check_flags: []const []const u8 = &.{
    "-fsanitize=array-bounds,null,alignment,unreachable,address,leak", // asan and leak are linux/macos only in 0.14.1
    "-fstack-protector-strong",
    "-fno-omit-frame-pointer",
};

const warning_flags: []const []const u8 = &.{
    "-Wall",
    "-Wextra",
    "-Wnull-dereference",
    "-Wuninitialized",
    "-Wshadow",
    "-Wpointer-arith",
    "-Wstrict-aliasing",
    "-Wstrict-overflow=5",
    "-Wcast-align",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wfloat-equal",
    "-Wformat=2",
    "-Wswitch-enum",
    "-Wmissing-declarations",
    "-Wunused",
    "-Wundef",
    "-Werror",
};
