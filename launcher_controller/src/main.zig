const std = @import("std");
const launcher_controller = @import("launcher_controller");

const launcher_dev_path = "/dev/launcher0";
const led_addr: usize = 0x4120_0000;
const buttons_addr: usize = 0x4121_0000;
const switches_addr: usize = 0x4122_0000;

const Gpio = struct {
    _led_mapping: []u8,
    _buttons_mapping: []u8,
    _switches_mapping: []u8,
    _fd: std.posix.fd_t,

    led_data: *volatile u32,
    buttons_data: *volatile u32,
    switches_data: *volatile u32,

    pub const Buttons = struct {
        pub const center: u8 = 1 << 0;
        pub const down: u8 = 1 << 1;
        pub const left: u8 = 1 << 2;
        pub const right: u8 = 1 << 3;
        pub const up: u8 = 1 << 4;
    };

    pub fn init(leds_offset: usize, buttons_offset: usize, switches_offset: usize) !@This() {
        const fd = try std.posix.open("/dev/mem", .{
            .ACCMODE = .RDWR,
        }, 0);

        const led_mapping = try std.posix.mmap(null, std.heap.pageSize(), std.posix.PROT.READ | std.posix.PROT.WRITE, .{ .TYPE = .SHARED }, fd, leds_offset);
        const buttons_mapping = try std.posix.mmap(null, std.heap.pageSize(), std.posix.PROT.READ | std.posix.PROT.WRITE, .{ .TYPE = .SHARED }, fd, buttons_offset);
        const switches_mapping = try std.posix.mmap(null, std.heap.pageSize(), std.posix.PROT.READ | std.posix.PROT.WRITE, .{ .TYPE = .SHARED }, fd, switches_offset);

        const led_reg_ptr = @as(*volatile u32, @ptrCast((led_mapping.ptr)));
        const buttons_reg_ptr = @as(*volatile u32, @ptrCast((buttons_mapping.ptr)));
        const switches_reg_ptr = @as(*volatile u32, @ptrCast((switches_mapping.ptr)));

        return .{ ._led_mapping = led_mapping, ._buttons_mapping = buttons_mapping, ._switches_mapping = switches_mapping, ._fd = fd, .led_data = led_reg_ptr, .buttons_data = buttons_reg_ptr, .switches_data = switches_reg_ptr };
    }

    pub fn deinit(self: @This()) void {
        std.posix.munmap(self.led_mapping);
        std.posix.munmap(self.buttons_mapping);
        std.posix.munmap(self.switches_mapping);
        std.posix.close(self.fd);
    }
};

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
        fire = 0x10,
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

pub fn main() !noreturn {
    const lnchr = try Launcher.init(launcher_dev_path);
    defer lnchr.deinit();
    try lnchr.send_command(Launcher.Command.up_right);

    const gpio = try Gpio.init(led_addr, buttons_addr, switches_addr);
    defer gpio.deinit();

    gpio.led_data.* = 0;

    var buttons: u32 = 0;

    while (true) {
        const new_buttons = gpio.buttons_data.*;
        const new_activity = new_buttons ^ buttons;

        buttons = new_buttons;

        if (new_activity == 0) continue;

        switch (buttons) {
            Gpio.Buttons.center => lnchr.send_command(.fire) catch {},
            Gpio.Buttons.up | Gpio.Buttons.right => lnchr.send_command(.up_right) catch {},
            Gpio.Buttons.up | Gpio.Buttons.left => lnchr.send_command(.up_left) catch {},
            Gpio.Buttons.down | Gpio.Buttons.right => lnchr.send_command(.down_right) catch {},
            Gpio.Buttons.down | Gpio.Buttons.left => lnchr.send_command(.down_left) catch {},
            Gpio.Buttons.right => lnchr.send_command(.right) catch {},
            Gpio.Buttons.left => lnchr.send_command(.left) catch {},
            Gpio.Buttons.up => lnchr.send_command(.up) catch {},
            Gpio.Buttons.down => lnchr.send_command(.down) catch {},
            else => lnchr.send_command(.stop) catch {},
        }
        std.Thread.sleep(5_000_000);
    }

    // Prints to stderr, ignoring potential errors.
    std.debug.print("All your {s} are belong to us.\n", .{"codebase"});
    try launcher_controller.bufferedPrint();
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
