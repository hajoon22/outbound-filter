const std = @import("std");

pub const FilterType = enum {
    port,
    netmask,
};

pub const Protocol = enum(u8) {
    udp = 17,
    tcp = 6,
};

pub const PortFilter = struct {
    protocol: Protocol,
    port: u16,
};

pub const NetmaskFilter = struct {
    address: [4]u8,
    mask: [4]u8,
};

pub const Result = union(enum) {
    port: PortFilter,
    netmask: NetmaskFilter,
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

fn read(io: std.Io, allocator: std.mem.Allocator, path: []const u8) !std.ArrayList([]const u8) {
    const data = try std.Io.Dir.cwd().readFileAlloc(io, path, allocator, .unlimited);
    defer allocator.free(data);

    var results = std.ArrayList([]const u8).empty;
    errdefer {
        for (results.items) |item| {
            allocator.free(item);
        }

        results.deinit(allocator);
    }

    var lines = std.mem.splitScalar(u8, data, '\n');
    while (lines.next()) |line| {
        if (line.len == 0) continue;

        try results.append(allocator, try allocator.dupe(u8, line));
    }

    return results;
}

fn parse_port_filter(parts: *std.mem.SplitIterator(u8, .scalar), i: usize) !Result {
    var protocol: Protocol = undefined;
    if (parts.next()) |s| {
        protocol = parse_protocol(s) catch |err| {
            std.debug.print("{d}: invalid protocol\n", .{i});
            return err;
        };
    } else {
        std.debug.print("{d}: invalid port filter\n", .{i});
        return error.InvalidPortFilter;
    }

    var port: u16 = undefined;
    if (parts.next()) |p| {
        port = std.fmt.parseInt(u16, p, 10) catch |err| {
            std.debug.print("{d}: invalid port\n", .{i});
            return err;
        };
    } else {
        std.debug.print("{d}: invalid port filter\n", .{i});
        return error.InvalidPortFilter;
    }

    std.debug.print("{any} {any}\n", .{ protocol, port });

    return Result{
        .port = PortFilter{
            .protocol = protocol,
            .port = port,
        },
    };
}

fn parse_netmask_filter(parts: *std.mem.SplitIterator(u8, .scalar), i: usize) !Result {
    var netmask: std.mem.SplitIterator(u8, .scalar) = undefined;
    if (parts.next()) |s| {
        netmask = std.mem.splitScalar(u8, s, '/');
    } else {
        std.debug.print("{d}: invalid netmask filter\n", .{i});
        return error.InvalidNetmask;
    }

    const address = parse_ipv4(netmask.next().?) catch |err| {
        std.debug.print("{d}: is not ipv4\n", .{i});
        return err;
    };

    var mask: [4]u8 = undefined;
    if (netmask.next()) |s| {
        mask = parse_mask(s) catch |err| {
            std.debug.print("{d}: invalid mask\n", .{i});
            return err;
        };
    } else {
        std.debug.print("{d}: invalid netmask\n", .{i});
        return error.InvalidMask;
    }

    std.debug.print("{any} {any}\n", .{ address, mask });

    return Result{
        .netmask = NetmaskFilter{
            .address = address,
            .mask = mask,
        },
    };
}

pub fn read_and_parse(io: std.Io, allocator: std.mem.Allocator, path: []const u8) !std.ArrayList(Result) {
    var lines = try read(io, allocator, path);
    defer {
        for (lines.items) |item| {
            allocator.free(item);
        }

        lines.deinit(allocator);
    }

    var results = std.ArrayList(Result).empty;
    for (lines.items, 0..) |line, i| {
        var parts = std.mem.splitScalar(u8, line, ':');
        const filter_type = parse_filter_type(parts.next().?) catch |err| {
            std.debug.print("{d}: invalid filter type\n", .{i});
            return err;
        };

        switch (filter_type) {
            .port => {
                try results.append(allocator, try parse_port_filter(&parts, i));
            },
            .netmask => {
                try results.append(allocator, try parse_netmask_filter(&parts, i));
            },
        }
    }

    return results;
}
