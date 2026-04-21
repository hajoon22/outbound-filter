const std = @import("std");

pub const FilterType = enum {
    port,
    netmask,
};

pub const Protocol = enum(u8) {
    udp = 17,
    tcp = 6,
};

pub fn parse_filter_type(s: []const u8) !FilterType {
    if (std.mem.eql(u8, s, "port")) return .port;
    if (std.mem.eql(u8, s, "netmask")) return .netmask;

    std.debug.print("{s}\n", .{s});

    return error.InvalidFilterType;
}

pub fn parse_protocol(s: []const u8) !Protocol {
    if (std.mem.eql(u8, s, "udp")) return .udp;
    if (std.mem.eql(u8, s, "tcp")) return .tcp;

    return error.InvalidProtocol;
}

pub fn parse_ipv4(s: []const u8) ![4]u8 {
    const address = try std.Io.net.IpAddress.parse(s, 0);
    return switch (address) {
        .ip4 => |a| a.bytes,
        .ip6 => error.IsNotIpv4,
    };
}

pub fn parse_mask(s: []const u8) ![4]u8 {
    if (std.mem.eql(u8, s, "32")) return .{ 255, 255, 255, 255 };
    if (std.mem.eql(u8, s, "24")) return .{ 255, 255, 255, 0 };
    if (std.mem.eql(u8, s, "16")) return .{ 255, 255, 0, 0 };
    if (std.mem.eql(u8, s, "8")) return .{ 255, 0, 0, 0 };

    return error.InvalidMask;
}
