const std = @import("std");
pub const parser = @import("parser.zig");

fn send_to_module(io: std.Io, buf: []u8) !void {
    const dst = try std.Io.net.IpAddress.parse("127.0.0.1", 209);
    const src = try std.Io.net.IpAddress.parse("0.0.0.0", 0);

    var sock = try std.Io.net.IpAddress.bind(&src, io, .{
        .mode = .dgram,
        .protocol = .udp,
    });

    sock.send(io, &dst, buf[0..]) catch {};
}

pub fn build_and_send(allocator: std.mem.Allocator, io: std.Io, lists: *std.ArrayList(parser.Result)) !void {
    defer lists.deinit(allocator);
    for (lists.items) |result| {
        switch (result) {
            .port => |port| {
                var buf: [4]u8 = undefined;
                buf[0] = 0;
                buf[1] = @intFromEnum(port.protocol);
                std.mem.writeInt(u16, buf[2..], port.port, .big);

                try send_to_module(io, &buf);
            },
            .netmask => |netmask| {
                var buf: [9]u8 = undefined;
                buf[0] = 2;
                std.mem.copyForwards(u8, buf[1..5], &netmask.address);
                std.mem.copyForwards(u8, buf[5..9], &netmask.mask);

                try send_to_module(io, &buf);
            },
        }
    }
}
