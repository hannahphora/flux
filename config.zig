pub const enabled_systems: packed struct {
    renderer: bool = true,
    input: bool = true,
    ui: bool = false,
    audio: bool = false,
    physics: bool = false,
    animation: bool = false,
} = .{};
