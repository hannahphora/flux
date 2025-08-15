const std = @import("std");
const zcc = @import("compile_commands");

const additional_flags: []const []const u8 = &.{"-std=c++20"};
const runtime_check_flags: []const []const u8 = &.{
    // asan and leak are linux/macos only in 0.14.1
    "-fsanitize=array-bounds,null,alignment,unreachable,undefined,address,leak",
    "-fstack-protector-strong",
    "-fno-omit-frame-pointer",
};
const warning_flags: []const []const u8 = &.{
    "-Wall",
    "-Wextra",
    "-Wpedantic",
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
    // warning disables
    "-Wno-unused-parameter",
    "-Wno-missing-field-initializers",
};
const debug_flags = warning_flags ++ additional_flags;

pub fn build(b: *std.Build) !void {
    // build options
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const flux = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });
    const exe = b.addExecutable(.{
        .root_module = flux,
        .name = "flux",
    });

    // add engine srcs
    exe.addCSourceFiles(.{ .files = &.{
        "src/engine/engine.cpp",
        "src/renderer/renderer.cpp",
        "src/input/input.cpp",
        "src/audio/audio.cpp",
        "src/physics/physics.cpp",
    }, .flags = debug_flags });

    // get VULKAN_SDK paths
    var env_map = try std.process.getEnvMap(b.allocator);
    defer env_map.deinit();
    const path = env_map.get("VULKAN_SDK") orelse @panic("failed to get VULKAN_SDK from envmap");

    // dep srcs
    try addCSourceFilesInDir(b, exe, "deps/src/imgui", &.{});
    try addCSourceFilesInDir(b, exe, "deps/src/meshoptimizer", &.{});
    try addCSourceFilesInDir(b, exe, "deps/src/vkb", &.{});

    // dep linking
    flux.addLibraryPath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) });
    flux.addLibraryPath(b.path("deps/lib"));
    flux.linkSystemLibrary(if (target.result.os.tag == .windows) "vulkan-1" else "vulkan", .{});
    flux.linkSystemLibrary("glfw3", .{});
    if (target.result.os.tag == .windows) {
        b.installBinFile("deps/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("deps/lib/glfw3.so", "glfw3.so.0");
        flux.addRPathSpecial("$ORIGIN");
    }

    // dep includes
    flux.addIncludePath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) });
    flux.addIncludePath(b.path("src"));
    flux.addIncludePath(b.path("deps/include"));
    flux.addIncludePath(b.path("deps/include/meshoptimizer"));
    flux.addIncludePath(b.path("deps/include/imgui"));
    flux.addIncludePath(b.path("deps/include/imgui/backends"));

    try compileShaders(b);
    b.installArtifact(exe);

    // run option
    const run = b.addRunArtifact(exe);
    run.step.dependOn(b.getInstallStep());
    if (b.args) |args| run.addArgs(args);
    b.step("run", "Run the engine").dependOn(&run.step);

    // generate compile_commands.json option
    var targets = std.ArrayList(*std.Build.Step.Compile).init(b.allocator);
    defer targets.deinit();
    try targets.append(exe);
    _ = zcc.createStep(b, "cmds", try targets.toOwnedSlice());
}

fn compileShaders(b: *std.Build) !void {
    var dir = try b.build_root.handle.openDir("res/shaders", .{ .iterate = true });
    defer dir.close();
    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    while (try walker.next()) |entry| {
        if (entry.kind == .file and std.mem.eql(u8, std.fs.path.extension(entry.basename), ".slang")) {
            const name = std.fs.path.stem(entry.basename);
            const source = try std.fmt.allocPrint(b.allocator, "res/shaders/{s}.slang", .{name});
            const outpath = try std.fmt.allocPrint(b.allocator, "res/shaders/{s}.spv", .{name});

            const shader_comp = b.addSystemCommand(&.{"slangc"});
            shader_comp.addArgs(&.{
                "-target",
                "spirv",
                "-fvk-use-entrypoint-name",
                "-o",
            });
            const output = shader_comp.addOutputFileArg(outpath);
            shader_comp.addFileArg(b.path(source));

            b.getInstallStep().dependOn(&b.addInstallBinFile(output, outpath).step);
        }
    }
}

fn addCSourceFilesInDir(b: *std.Build, c: *std.Build.Step.Compile, path: []const u8, flags: []const []const u8) !void {
    var dir = try b.build_root.handle.openDir(path, .{ .iterate = true });
    defer dir.close();

    var itr = dir.iterate();
    while (try itr.next()) |entry| {
        if (entry.kind == .file and
            (std.mem.eql(u8, std.fs.path.extension(entry.name), ".c") or
                std.mem.eql(u8, std.fs.path.extension(entry.name), ".cpp")))
        {
            const src_path = try std.fmt.allocPrint(b.allocator, "{s}/{s}", .{ path, entry.name });
            c.addCSourceFile(.{ .file = b.path(src_path), .flags = flags });
        }
    }
}
