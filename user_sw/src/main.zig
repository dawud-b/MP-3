const std = @import("std");
const user_sw = @import("user_sw");

const Launcher = struct {
    file: std.fs.File,

    pub const Command = enum(u8) {
        stop = 0x20,
        up = 0x2,
        down = 0x1,
        left = 0x4,
        right = 0x8,
        up_left = 0x2 | 0x4,
        up_right = 0x2 | 0x8,
        down_left = 0x1 | 0x4,
        down_right = 0x1 | 0x8,
    };

    pub fn init(path: []const u8) !@This() {
        return .{ .file = try std.fs.openFileAbsolute(path, .{ .mode = .write_only }) };
    }

    pub fn send_command(self: @This(), command: Command) !void {
        const byte = &[_]u8{@intFromEnum(command)};
        _ = try self.file.write(byte);
    }

    pub fn deinit(self: @This()) void {
        self.file.close();
    }
};

pub fn main() !void {
    const lnchr = try Launcher.init("/dev/null");
    try lnchr.send_command(Launcher.Command.up_right);

    // Prints to stderr, ignoring potential errors.
    std.debug.print("All your {s} are belong to us.\n", .{"codebase"});
    try user_sw.bufferedPrint();
}

test "simple test" {
    const gpa = std.testing.allocator;
    var list: std.ArrayList(i32) = .empty;
    defer list.deinit(gpa); // Try commenting this out and see if zig detects the memory leak!
    try list.append(gpa, 42);
    try std.testing.expectEqual(@as(i32, 42), list.pop());
}

test "fuzz example" {
    const Context = struct {
        fn testOne(context: @This(), input: []const u8) anyerror!void {
            _ = context;
            // Try passing `--fuzz` to `zig build test` and see if it manages to fail this test case!
            try std.testing.expect(!std.mem.eql(u8, "canyoufindme", input));
        }
    };
    try std.testing.fuzz(Context{}, Context.testOne, .{});
}
