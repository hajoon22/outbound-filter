const std = @import("std");
const filter = @import("filter/filter.zig");

pub fn main(init: std.process.Init) !void {
    const args = try init.minimal.args.toSlice(init.arena.allocator());
    if (args.len < 3) {
        std.debug.print("invalid format\nformat: {s} [add/remove] [filter path]\n", .{args[0]});
        return;
    }

    if (std.mem.eql(u8, args[1], "add")) {
        var results = try filter.parser.read_and_parse(init.io, init.arena.allocator(), args[2]);
        try filter.build_and_send(init.arena.allocator(), init.io, &results);
    }
}
