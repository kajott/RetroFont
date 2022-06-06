#!/usr/bin/env python3
"""
Generic bitmap font extractor.
"""
import argparse
import hashlib
import sys
import os


class size:
    def __init__(self, s):
        if isinstance(s, size):
            self.x, self.y = s.x, s.y
        else:
            self.x, self.y = map(int, s.replace(',','x').lower().split('x'))
    def __str__(self):
        return f"{self.x}x{self.y}"


class arange:
    def __init__(self, s):
        if isinstance(s, arange):
            self.ranges = s.ranges[:]
        else:
            self.ranges = []
            for r in s.split(','):
                r = [int(x.lstrip('0').strip('$x') or '0', 16) for x in r.split(':')]
                if len(r) == 1: r.append(0)
                a,b = r
                self.ranges.append((a,b))

    def get_ranges(self, end):
        for a,b in self.ranges:
            if not b:
                b = min([o for o,_ in self.ranges if o>a] + [end])
            if (a > end) or (b > end):
                raise ValueError("extract range extends past file end")
            if a < b:
                yield a,b

    def __str__(self):
        return ','.join((f"{a:X}:{b:X}" if b else f"{a:X}") for a,b in self.ranges)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", metavar="INPUT", nargs='+',
                        help="input file(s)")
    parser.add_argument("-s", "--size", metavar="WxH", type=size, default=size("8x8"),
                        help="set font size to extract (default: %(default)s)")
    parser.add_argument("-a", "--address", metavar="FROM[:TO][,FROM:TO...]", type=arange, default=arange("0"),
                        help="address range(s) to extract (all values in hex; in --scan mode, restrict scanning to address range; default: whole file)")
    parser.add_argument("-x", "--scan", action='store_true',
                        help="search for fonts in the file (need to start with an empty glyph, followed by a few non-empty ones)")
    parser.add_argument("-m", "--min-nonempty", type=int, default=4,
                        help="number of non-empty glyphs required for a successful scan")
    parser.add_argument("-f", "--full", metavar="INDEX", type=int,
                        help="index of a fully filled glyph (used to improve search accuracy; default: no filled glyph)")
    parser.add_argument("-c", "--chars", metavar="COUNT", type=int, default=256,
                        help="maximum number of chars to extract (if address range doesn't have a TO component; default: %(default)s)")
    parser.add_argument("-o", "--output", metavar="PBM",
                        help="output PBM file (only useful for single inputs; address ranges will be merged into a single font; default: derive from input)")
    parser.add_argument("-n", "--dry-run", action='store_true',
                        help="don't actually write the output files")
    parser.add_argument("-r", "--bitrev", "--lsb-first", action='store_true',
                        help="interpret font data as LSB-first (default: MSB-first)")
    parser.add_argument("-w", "--width", type=int, default=32,
                        help="characters per row in the output image files (default: %(default)s)")
    args = parser.parse_args()

    bpl = (args.size.x + 7) >> 3  # bytes per scanline
    bpc = bpl * args.size.y       # bytes per character
    bitrange = (0,1,2,3,4,5,6,7) if args.bitrev else (7,6,5,4,3,2,1,0)
    empty = b'\x00' * bpc
    full = bytes(sum((int((i + b) < args.size.x) << s) for b,s in enumerate(bitrange)) for i in range(0, args.size.x, 8)) * args.size.y
    line_length = args.size.x * args.width

    inputs = []
    for i in args.inputs:
        if os.path.isdir(i):
            for root, dirs, files in os.walk(i):
                inputs.extend(os.path.join(root, f) for f in files)
        else:
            inputs.append(i)
    if (len(inputs) > 1) and args.output:
        parser.error("can't use --output option with multiple inputs")

    for infile in inputs:
        # load input file
        print("<--", infile)
        base = os.path.basename(infile).replace('.', '_')
        try:
            with open(infile, 'rb') as f:
                data = f.read()
        except EnvironmentError as e:
            print(f"ERROR: can't read input file '{infile}' - {e}", file=sys.stderr)
            continue
        addr_len = len(f"{len(data)-1:X}")

        # resolve address ranges
        try:
            ranges = list(args.address.get_ranges(len(data)))
        except ValueError as e:
            print(f"ERROR: {e} in input file '{infile}'", file=sys.stderr)
            continue

        # scan inside address ranges
        if args.scan:
            scanned = []
            min_size = (1 + args.min_nonempty) * bpc
            if args.full: min_size = max(min_size, (args.full + 1) * bpc)
            for start, end in ranges:
                while True:
                    start = data.find(empty, start, max(0, end - min_size))
                    if start < 0: break
                    if all((data[start + (i + 1) * bpc : start + (i + 2) * bpc] != empty) for i in range(args.min_nonempty)) \
                    and (not(args.full) or (data[start + args.full * bpc : start + (args.full + 1) * bpc] == full)):
                        scanned.append((start, end))
                    start += 1

            # sanitize resulting ranges
            ranges = sorted((a, min([b] + [o for o,_ in scanned if o>a])) for a,b in scanned)
            i = 0
            while i < len(ranges):
                a,b = ranges[i]
                if (b - a) >= bpc:
                    # range large enough -> keep it
                    i += 1
                elif ((i + 1) < len(ranges)) and (b == ranges[i+1][0]):
                    # range too small, but another range directly follows -> merge them
                    ranges[i] = (a, ranges[i+1][1])
                    del ranges[i+1]
                else:
                    # range too small, but no range follows -> delete it
                    del ranges[i]

        # load characters
        chars = []
        char_ranges = []
        for start, end in ranges:
            end = min(end, start + bpc * args.chars)
            char_ranges.append((start, len(chars)))
            for offset in range(start, end, bpc):
                char = []
                for y in range(args.size.y):
                    row = []
                    for x in data[offset + y * bpl : offset + (y + 1) * bpl]:
                        row.extend((x >> b) & 1 for b in bitrange)
                    char.append(row[:args.size.x])
                chars.append(char)
        if args.output:
            del char_ranges[1:]
        char_ranges.append((0, len(chars)))

        # save output file(s)
        for (addr, start), (dummy, end) in zip(char_ranges, char_ranges[1:]):
            addr = f"{addr:X}".rjust(addr_len, '0')
            outfile = args.output or f"{base}_{addr}_{args.size.x}x{args.size.y}.pbm"
            rows = (end - start + args.width - 1) // args.width
            out_height = rows * args.size.y
            data = bytearray(f"P4\n{line_length} {out_height}\n".encode())
            for offset in range(start, end, args.width):
                rows = [[] for y in range(args.size.y)]
                for char in chars[offset : min(end, offset + args.width)]:
                    for y in range(args.size.y):
                        rows[y].extend(char[y])
                for row in rows:
                    row.extend([0] * (line_length - len(row) + 8))
                    data.extend(sum((row[i+7-b] << b) for b in range(8)) for i in range(0, line_length, 8))
            hash = hashlib.sha1(data).hexdigest()[:8].upper()
            print("-->", outfile, f"({end-start} glyphs, {line_length}x{out_height}, hash: {hash})")
            if not args.dry_run:
                try:
                    with open(outfile, 'wb') as f:
                        f.write(data)
                except EnvironmentError as e:
                    print(f"ERROR: can't write output file '{outfile}' - {e}", file=sys.stderr)
                    continue
