#!/usr/bin/env python3
import unicodedata
import collections
import glob
import sys
import re
import os

FORCE_VERBOSE = False

class coord:
    def __init__(self, x, y): self.x, self.y = x, y
    def __str__(self):        return f"{self.x},{self.y}"
    def __repr__(self):       return f"coord({self.x},{self.y})"

def ParseCodepointRange(line):
    m = re.match(r'("(?P<str>[^"]+)"|-|' + r"(U\+(?P<au>[0-9a-f]+)|'(?P<ac>.)')(\.{2,3}(U\+(?P<bu>[0-9a-f]+)|'(?P<bc>.)'))?)($|(?=#)|\s+)", line, flags=re.I)
    if not m: raise ValueError(f"syntax error in codepoint range: `{line}`")
    if m.group(1) == '-':
        cps = [0]
    elif m.group('str'):
        cps = [ord(c) for c in m.group('str')]
    else:
        a = int(m.group('au'), 16) if m.group('au') else  ord(m.group('ac'))
        b = int(m.group('bu'), 16) if m.group('bu') else (ord(m.group('bc')) if m.group('bc') else a)
        if a > b: raise ValueError(f"reverse codepoint range (U+{a:04X}...U+{b:04X}): `{m.group(1)}`")
        cps = list(range(a, b+1))
    return line[m.end(0):], cps

def charname(cp):
    try:
        return unicodedata.name(chr(cp))
    except ValueError:
        return "(unknown)"

################################################################################

class SourceImage:
    def __init__(self, filename, font_size):
        self.font_size = font_size
        with open(filename, 'rb') as f:
            data = f.read()

        # decode PBM header
        nums = []
        number = comment = False
        for i, c in enumerate(data):
            c = bytes([c])
            if not(i):
                if c != b'P': raise IOError("not a valid PBM file: invalid header magic")
            elif comment:
                comment = (c != b'\n')
            elif c == b'#':
                comment, number = True, False
            elif c.isspace():
                number = False
                if len(nums) >= 3: break
            elif c.isdigit():
                if not number: nums.append(0)
                nums[-1] = nums[-1] * 10 + int(c)
                number = True
            else:
                raise IOError("not a valid PBM file: syntax error in header")
        f, w, h = nums
        if f != 4: raise IOError("not a valid PBM file: invalid header magic")
        data = data[i+1:]
        bpl = (w + 7) // 8
        size = h * bpl
        if len(data) < size:
            raise IOError("not a valid PBM file: file truncated")

        # decode PBM image data
        img = []
        for offset in range(0, size, bpl):
            row = []
            for byte in data[offset:offset+bpl]:
                row.extend((byte >> bit) & 1 for bit in (7,6,5,4,3,2,1,0))
            img.append(row)
        if 0:  # DEBUG
            fscale = 85 // (self.font_size.x + self.font_size.y)
            with open("abfall.pgm", "wb") as f:
                f.write(f"P5\n{bpl*8} {h}\n255\n".encode())
                for y,r in enumerate(img): f.write(bytes(fscale * (x%self.font_size.x + y%self.font_size.y) + p * 170 for x,p in enumerate(r)))

        # slice into glyphs and generate bitmaps for them
        self.glyphs = {}
        for gy in range(h // self.font_size.y):
            rows = img[gy * self.font_size.y : (gy+1) * self.font_size.y]
            for gx in range(w // self.font_size.x):
                cell = [row[gx * self.font_size.x : (gx+1) * self.font_size.x] for row in rows]
                bmp = bytearray()
                for row in cell:
                    bmp.extend(sum((row[base+bit] << bit) for bit in range(8)) for base in range(0, len(row), 8))
                self.glyphs[(gy,gx)] = bytes(bmp)

################################################################################

class Font:
    def __init__(self, font_id, name, img):
        self.font_id = font_id
        self.name = name
        self.img = img
        self.font_size = self.img.font_size
        self.glyphs = {}

################################################################################

g_err_prefix = ""
g_errors = 0
def err(msg):
    global g_errors
    print("ERROR:" + g_err_prefix, msg, file=sys.stderr)
    g_errors = 1
def warn(msg):
    print("WARNING:" + g_err_prefix, msg, file=sys.stderr)

def map_glyph(target, cp, glyph):
    if (cp in target) and (glyph != target[cp]):
        warn(f"re-assigning glyph for U+{cp:04X}")
    target[cp] = glyph

if __name__ == "__main__":
    verbose = FORCE_VERBOSE or ("-v" in sys.argv)
    tooldir = os.path.normpath(os.path.abspath(os.path.dirname(sys.argv[0])))
    os.chdir(os.path.join(os.path.dirname(tooldir), "fonts.in"))
    allfonts = []
    for specfile in glob.glob("*.fontspec"):
        fonts = []
        maps = {}
        img = None
        defmap = None
        if verbose: print("- processing font spec:", specfile)
        with open(specfile, encoding='utf-8') as f:
            lineno = 0
            for line in f:
                lineno += 1
                g_err_prefix = f" {specfile}:{lineno}:"
                line = line.strip()
                if line.startswith('#') or not(line): continue

                m = re.match(r'source\s+"([^"]+)"\s+(\d+)x(\d+)', line, flags=re.I)
                if m:
                    if verbose: print("  - loading image:", m.group(1), f" ({m.group(2)}x{m.group(3)})")
                    try:
                        img = SourceImage(m.group(1), coord(int(m.group(2)), int(m.group(3))))
                        if verbose: print("    - contains", len(img.glyphs), "glyph bitmaps")
                    except EnvironmentError as e:
                        err(f"can not import '{m.group(1)}': {e}")
                        img = None
                    continue

                m = re.match(r'font\s+"([^"]{4})"\s+"([^"]+)"', line, flags=re.I)
                if m:
                    if verbose: print(f"  - new font: {m.group(1)} ('{m.group(2)}')")
                    if not img:
                        err("'font' command invalid without an active source image")
                        continue
                    fonts.append(Font(m.group(1), m.group(2), img))
                    defmap = None
                    continue

                m = re.match(f'defmap\s+"([^"]+)"', line, flags=re.I)
                if m:
                    defmap = m.group(1)
                    maps[defmap] = {}
                    continue

                m = re.match(r'map\s+(\d+)(\.(\d+))?\s+', line, flags=re.I)
                if m:
                    if not(fonts) and not(defmap):
                        err("'map' command invalid without an active font")
                        continue
                    gy = int(m.group(1))
                    gx = int(m.group(3)) if m.group(2) else 0
                    line = line[m.end(0):]
                    while line and not(line.startswith('#')):
                        try:
                            line, cps = ParseCodepointRange(line)
                        except ValueError as e:
                            err(e)
                            break
                        for cp in cps:
                            if cp:
                                try:
                                    glyph = (gy,gx)
                                    if not defmap:
                                        glyph = img.glyphs[glyph]
                                except KeyError:
                                    err(f"no glyph at location {gy}.{gx} (for U+{cp:04X})")
                                    break
                                map_glyph(maps[defmap] if defmap else fonts[-1].glyphs, cp, glyph)
                            gx += 1
                    continue

                m = re.match(r'alias\s+', line, flags=re.I)
                if m:
                    if not(fonts) and not(defmap):
                        err("'alias' command invalid without an active font")
                        continue
                    line = line[m.end(0):]
                    while line and not(line.startswith('#')):
                        try:
                            line, dest = ParseCodepointRange(line)
                            line, src  = ParseCodepointRange(line)
                        except ValueError as e:
                            err(e)
                            break
                        if len(dest) != len(src):
                            err(f"codepoint ranges have different lengths (destination: {len(dest)}, source: {len(src)})")
                            continue
                        for d, s in zip(dest, src):
                            if s and d:
                                try:
                                    if defmap:
                                        glyph = maps[defmap][s]
                                    else:
                                        glyph = fonts[-1].glyphs[s]
                                except KeyError:
                                    err(f"codepoint U+{s:04X} missing in " + ("map" if defmap else "font"))
                                    break
                                map_glyph(maps[defmap] if defmap else fonts[-1].glyphs, d, glyph)
                    continue

                m = re.match(f'usemap\s+"([^"]+)"', line, flags=re.I)
                if m:
                    if not fonts:
                        err("'usemap' command invalid without an active font")
                        continue
                    try:
                        src_map = maps[m.group(1)]
                    except KeyError:
                        err(f"undefined map '{m.group(1)}'")
                        continue
                    for cp, pos in sorted(src_map.items()):
                        try:
                            glyph = img.glyphs[pos]
                        except KeyError:
                            err(f"no glyph at location {gy}.{gx} (for U+{cp:04X})")
                            break
                        map_glyph(fonts[-1].glyphs, cp, glyph)
                    continue

                err(f"unrecognized line `{line}`")
            allfonts += fonts
    fonts = allfonts
    g_err_prefix = ""

    # build composite bitmap and resolve glyphs
    bitmap = b''
    markers = collections.defaultdict(lambda: collections.defaultdict(set))
    for f in sorted(fonts, key=lambda f:(-f.font_size.y,-f.font_size.x)):
        for cp in sorted(f.glyphs):
            offset = bitmap.find(f.glyphs[cp])
            if offset < 0:
                offset = len(bitmap)
                bitmap += f.glyphs[cp]
            f.glyphs[cp] = offset
            markers[offset][cp].add(f.font_id)
    segments = sorted(markers) + [len(bitmap)]
    segments = list(zip(segments, segments[1:]))
    if verbose: print("- composite bitmap size:", len(bitmap), "bytes,", len(markers), "distinct glyphs")

    # generate fonts.c
    os.chdir(os.path.join(os.path.dirname(tooldir), "src"))
    namelen = max(len(f.name) for f in fonts) + 3
    dumplen = max(b-a for a,b in segments) * 6
    me = os.path.basename(sys.argv[0])
    if verbose: print("- generating fonts.c")
    with open("fonts.c", "w") as out:
        out.write(f'// This file has been auto-generated by {me}. DO NOT EDIT BY HAND!\n\n')
        out.write('#include "retrofont.h"\n\n')

        out.write('const uint8_t RF_GlyphBitmaps[] = {\n')
        for a, b in segments:
            dump = ''.join(f"0x{c:02X}, " for c in bitmap[a:b]).ljust(dumplen)
            annot = ', '.join('/'.join(sorted(markers[a][cp])) + f" U+{cp:04X} " + charname(cp) for cp in sorted(markers[a]))
            out.write(f'    {dump} //{a:6d}: {annot}\n')
        out.write('};  ' + " " * dumplen + f' //{len(bitmap):6d}: (end)\n\n')

        for f in fonts:
            out.write(f'const RF_GlyphMapEntry glyphmap_{f.font_id}[] = {{\n')
            for cp in sorted(f.glyphs):
                cp_s = f"0x{cp:04X}"
                out.write(f'    {{ {cp_s:>8},{f.glyphs[cp]:6d} }},  // {charname(cp)}\n')
            out.write('};\n\n')

        out.write('const RF_Font RF_FontList[] = {\n')
        for f in fonts:
            name = f'"{f.name}",'.ljust(namelen)
            out.write(f'    {{ RF_MAKE_ID("{f.font_id}"), {name} {{{f.font_size.x:2d},{f.font_size.y:2d}}}, glyphmap_{f.font_id}, {len(f.glyphs):4d},{f.glyphs.get(0xFFFD,0):6d} }},\n')
        name = "NULL,".ljust(namelen)
        out.write(f'    {{ 0,                  {name} {{ 0, 0}}, NULL,             0,     0 }}\n')
        out.write('};\n')

    sys.exit(g_errors)
