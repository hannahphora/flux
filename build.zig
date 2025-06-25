const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const module = b.createModule(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // linking
    module.addLibraryPath(.{ .cwd_relative = "deps/lib" });
    module.linkSystemLibrary("glfw3");
    const vk_lib_name = if (target.result.os.tag == .windows) "vulkan-1" else "vulkan";
    module.linkSystemLibrary(vk_lib_name);

    // includes
    module.addIncludePath(b.path("deps/include"));
    module.addIncludePath(b.path("src"));
    const env_map = try std.process.getEnvMap(b.allocator);
    if (env_map.get("VK_SDK_PATH")) |path| {
        module.addLibraryPath(.{ .cwd_relative = std.fmt.allocPrint(b.allocator, "{s}/lib", .{path}) catch @panic("OOM") });
        module.addIncludePath(.{ .cwd_relative = std.fmt.allocPrint(b.allocator, "{s}/include", .{path}) catch @panic("OOM") });
    }

    const exe = b.addExecutable(.{
        .name = "zig_test",
        .root_module = module,
    });
    b.installArtifact(exe);

    compile_all_shaders(b, module);

    // copy glfw3 to bin dir
    if (target.result.os.tag == .windows) {
        b.installBinFile("thirdparty/lib/glfw3.dll", "glfw3.dll");
    } else {
        b.installBinFile("thirdparty/lib/glfw3.so", "glfw3.so.0");
        exe.root_module.addRPathSpecial("$ORIGIN");
    }

    // add run option
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);
    const run_step = b.step("run", "Run the program");
    run_step.dependOn(&run_cmd.step);

    // create unit test step
    const unit_tests = b.addTest(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    var iter = module.import_table.iterator();
    while (iter.next()) |e| {
        unit_tests.root_module.addImport(e.key_ptr.*, e.value_ptr.*);
    }
    const run_unit_tests = b.addRunArtifact(unit_tests);
    test_step.dependOn(&run_unit_tests.step);
}

fn compile_all_shaders(b: *std.Build, exe: *std.Build.Step.Compile) void {
    const shaders_dir = if (@hasDecl(@TypeOf(b.build_root.handle), "openIterableDir"))
        b.build_root.handle.openIterableDir("shaders", .{}) catch @panic("Failed to open shaders directory")
    else
        b.build_root.handle.openDir("shaders", .{ .iterate = true }) catch @panic("Failed to open shaders directory");

    var file_it = shaders_dir.iterate();
    while (try file_it.next()) |entry| {
        if (entry.kind == .file) {
            const ext = std.fs.path.extension(entry.name);
            if (std.mem.eql(u8, ext, ".slang")) {
                const basename = std.fs.path.basename(entry.name);
                const name = basename[0 .. basename.len - ext.len];

                std.debug.print("Found shader file to compile: {s}. Compiling with name: {s}\n", .{ entry.name, name });
                add_shader(b, exe, name);
            }
        }
    }
}

fn add_shader(b: *std.Build, exe: *std.Build.Step.Compile, name: []const u8) void {
    const source = std.fmt.allocPrint(b.allocator, "shaders/{s}.slang", .{name}) catch @panic("OOM");
    const outpath = std.fmt.allocPrint(b.allocator, "shaders/{s}.spv", .{name}) catch @panic("OOM");

    const shader_compilation = b.addSystemCommand(&.{"slangc"});
    shader_compilation.addArg("-target");
    shader_compilation.addArg("spirv");
    shader_compilation.addArg("-profile");
    shader_compilation.addArg("spirv_1_4");
    shader_compilation.addArg("-fvk-use-entrypoint-name");
    shader_compilation.addArg("-o");
    shader_compilation.addArg(outpath);
    shader_compilation.addArg(source);

    exe.step.dependOn(&shader_compilation.step);
}
