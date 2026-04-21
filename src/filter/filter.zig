const std = @import("std");
const parser = @import("parser.zig");

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

pub fn read_and_parse(io: std.Io, allocator: std.mem.Allocator, path: []const u8) !void {
    var lines = try read(io, allocator, path);
    defer {
        for (lines.items) |item| {
            allocator.free(item);
        }

        lines.deinit(allocator);
    }

    for (lines.items, 0..) |line, i| {
        var parts = std.mem.splitScalar(u8, line, ':');
        const filter_type = parser.parse_filter_type(parts.next().?) catch |err| {
            std.debug.print("{d}: invalid filter type\n", .{i});
            return err;
        };

        switch (filter_type) {
            .port => {
                var protocol: parser.Protocol = undefined;
                if (parts.next()) |s| {
                    protocol = parser.parse_protocol(s) catch |err| {
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
            },
            .netmask => {
                var netmask: std.mem.SplitIterator(u8, .scalar) = undefined;
                if (parts.next()) |s| {
                    netmask = std.mem.splitScalar(u8, s, '/');
                } else {
                    std.debug.print("{d}: invalid netmask filter\n", .{i});
                    return error.InvalidNetmask;
                }

                const address = parser.parse_ipv4(netmask.next().?) catch |err| {
                    std.debug.print("{d}: is not ipv4\n", .{i});
                    return err;
                };

                var mask: [4]u8 = undefined;
                if (netmask.next()) |s| {
                    mask = parser.parse_mask(s) catch |err| {
                        std.debug.print("{d}: invalid mask\n", .{i});
                        return err;
                    };
                } else {
                    std.debug.print("{d}: invalid netmask\n", .{i});
                    return error.InvalidMask;
                }

                std.debug.print("{any} {any}\n", .{ address, mask });
            },
        }
    }
}
