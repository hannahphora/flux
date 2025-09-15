const std = @import("std");
//const zcc = @import("compile_commands");

const additional_flags: []const []const u8 = &.{
    "-std=c++20",
    // warning disables
    "-Wno-unused-parameter",
    "-Wno-missing-field-initializers",
};
const debug_flags = warning_flags ++ additional_flags;

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libcpp = true,
        }),
        .name = "flux",
    });

    // add engine srcs
    exe.addCSourceFiles(.{ .files = &.{
        "src/engine/engine.cpp",
        "src/renderer/renderer.cpp",
        "src/input/input.cpp",
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
    exe.addLibraryPath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) });
    exe.addLibraryPath(b.path("deps/lib"));
    exe.root_module.linkSystemLibrary(if (target.result.os.tag == .windows) "vulkan-1" else "vulkan", .{});
    exe.root_module.linkSystemLibrary("glfw3", .{});
    if (target.result.os.tag == .windows) {
        b.installBinFile("deps/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("deps/lib/glfw3.so", "glfw3.so.0");
        exe.root_module.addRPathSpecial("$ORIGIN");
    }

    // dep includes
    exe.addIncludePath(.{ .cwd_relative = try std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) });
    exe.addIncludePath(b.path("src"));
    exe.addIncludePath(b.path("deps/include"));
    exe.addIncludePath(b.path("deps/include/meshoptimizer"));
    exe.addIncludePath(b.path("deps/include/imgui"));
    exe.addIncludePath(b.path("deps/include/imgui/backends"));

    try compileShaders(b, "res/shaders");
    b.installArtifact(exe);

    // run option
    const run = b.addRunArtifact(exe);
    run.step.dependOn(b.getInstallStep());
    if (b.args) |args| run.addArgs(args);
    b.step("run", "Run the engine").dependOn(&run.step);

    // generate compile_commands.json
    var targets = std.ArrayList(*std.Build.Step.Compile){};
    defer targets.deinit(b.allocator);
    try targets.append(b.allocator, exe);
    //_ = zcc.createStep(b, "cmds", try targets.toOwnedSlice());
}

fn compileShaders(b: *std.Build, baseDirPath: []const u8) !void {
    var baseDirHandle = try b.build_root.handle.openDir(baseDirPath, .{ .iterate = true });
    defer baseDirHandle.close();
    var dirWalker = try baseDirHandle.walk(b.allocator);
    defer dirWalker.deinit();

    while (try dirWalker.next()) |entry| {
        if (entry.kind == .file and std.mem.eql(u8, std.fs.path.extension(entry.basename), ".slang")) {
            const src = try std.fmt.allocPrint(b.allocator, "{s}/{s}", .{ baseDirPath, entry.path });
            defer b.allocator.free(src);
            const dst = try std.mem.replaceOwned(u8, b.allocator, src, ".slang", ".spv");
            defer b.allocator.free(dst);

            const shader_comp = b.addSystemCommand(&.{"slangc"});
            shader_comp.addArgs(&.{
                "-target",
                "spirv",
                "-fvk-use-entrypoint-name",
                "-o",
            });
            const output = shader_comp.addOutputFileArg(dst);
            shader_comp.addFileArg(b.path(src));
            b.getInstallStep().dependOn(&b.addInstallBinFile(output, dst).step);
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

const runtime_check_flags: []const []const u8 = &.{
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
};
