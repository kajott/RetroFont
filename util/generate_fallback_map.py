#!/usr/bin/env python3
import urllib.request
import sys
import re
import os

def make_key(name):
    return name.replace(' ', '').replace('-', '').lower()

################################################################################

SOURCE_URL_BASE = "http://www.unicode.org/Public/UCD/latest/ucd/"

def ReadUCD(filename):
    cachefile = ".cache." + filename
    try:
        f = open(cachefile, 'rb')
        c = None
    except EnvironmentError:
        url = SOURCE_URL_BASE + filename
        print(f"Downloading: {url} => {cachefile}", file=sys.stderr)
        f = urllib.request.urlopen(url)
        c = open(cachefile, 'wb')
    for line in f:
        if c:
            c.write(line)
        yield line.decode(errors='replace')
    if c: c.close()
    f.close()

class Codepoint:
    def __init__(self, *fields):
        #for i,x in enumerate(fields): print(i,x)
        self.cp = int(fields[0], 16)
        self.name = fields[1]
        self.key = make_key(self.name)
        self.upper, self.lower, self.title = \
            ((int(f, 16) if f else None) for f in fields[12:15])
        decomp = fields[5]
        if decomp.startswith('<'):
            self.decomp_type, decomp = decomp[1:].split('>', 1)
        else:
            self.decomp_type = None
        self.decomp = [int(x, 16) for x in decomp.split()] if decomp else None

    def __gt__(a, b): return a.cp > b.cp
    def __lt__(a, b): return a.cp < b.cp

    def __str__(self):
        return f"U+{self.cp:04X} {self.name}"

class Codepoints(dict):
    def __init__(self):
        super().__init__()
        for line in ReadUCD("UnicodeData.txt"):
            if not line.strip(): continue
            cp = Codepoint(*(x.strip() for x in line.split(';')))
            self[cp.cp] = cp

    def name(self, cp):
        try:
            return str(self[cp])
        except KeyError:
            return f"U+{cp:04X} (unknown)"

    def findall(self, name):
        key = make_key(name)
        return sorted(cp for cp in self.values() if key in cp.key)

    def find(self, name):
        key = make_key(name)
        return min(cp for cp in self.values() if key in cp.key)

################################################################################

class UnicodeBlock:
    def __init__(self, a, b, name):
        self.a = int(a, 16)
        self.b = int(b, 16)
        self.name = name
        self.key = make_key(name)
    def __str__(self):
        return f"U+{self.a:04X}..U+{self.b:04X} {self.name}"

class UnicodeBlockRange:
    def __init__(self):
        self.ranges = []
        self.names = []

    def add(self, block):
        self.ranges.append((block.a, block.b))
        self.ranges.sort()
        self.names.append(block.name)

    def __iter__(self):
        last = 0
        for a, b in self.ranges:
            yield from range(max(last, a), b)
            last = b + 1

    def iter(self, cplist=None):
        if cplist:
            for cp in self:
                try:
                    yield cplist[cp]
                except KeyError:
                    pass
        else:
            yield from self

class UnicodeBlockMap:
    def __init__(self):
        self.blocks = []
        for line in ReadUCD("Blocks.txt"):
            m = re.match(r'\s*([0-9a-f]+)\s*\.+\s*([0-9a-f]+)\s*;\s*(.*)', line, flags=re.I)
            if not m: continue
            self.blocks.append(UnicodeBlock(*m.groups()))

    def find(self, *patterns) -> UnicodeBlockRange:
        if len(patterns) == 1:
            patterns = patterns[0]
            if isinstance(patterns, str):
                patterns = patterns.split(',')
        keys = {make_key(p) for p in patterns}
        r = UnicodeBlockRange()
        for b in self.blocks:
            if any(k in b.key for k in keys):
                r.add(b)
        return r

################################################################################

class Fallbacks:
    def __init__(self):
        self.fbs = {}
        self.verbose = None

    def add(self, from_cp, *to_cp):
        if (len(to_cp) == 1) and isinstance(to_cp[0], (list, tuple, set)):
            to_cp = to_cp[0]
        if isinstance(from_cp, Codepoint):
            from_cp = from_cp.cp
        try:
            fblist = self.fbs[from_cp]
        except KeyError:
            fblist = []
            self.fbs[from_cp] = fblist
        for cp in to_cp:
            if isinstance(cp, Codepoint):
                cp = cp.cp
            if (cp != from_cp) and not(cp in fblist):
                fblist.append(cp)
                if self.verbose:
                    print(self.verbose.get(from_cp, hex(from_cp)), "==>", self.verbose.get(cp, hex(cp)))

    def add_cross(self, a, b):
        self.add(a, b)
        self.add(b, a)

################################################################################

if __name__ == "__main__":
    args = sys.argv[1:]
    try:
        args.remove("-q")
        quiet = True
    except ValueError:
        quiet = False
    outfile = args.pop(0) if args else None

    cps = Codepoints()
    blocks = UnicodeBlockMap()
    fbs = Fallbacks()

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
    # RULES BEGIN HERE                                                        #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    # map latin characters with diacritics to their base letter
    for cp in blocks.find("Latin").iter(cps):
        if not(cp.decomp) or cp.decomp_type:
            continue
        decomp = [d for d in cp.decomp if d < 128]
        if len(decomp) == 1:
            fbs.add(cp, decomp)

    # create aliases for German Eszett
    #fbs.verbose = cps  #DEBUG
    fbs.add(cps.find("small letter sharp s"), cps.find("small letter beta"))
    fbs.add(cps.find("small letter sharp s"), cps.find("capital letter b"))

    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
    # RULES END HERE                                                          #
    # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

    # collect unique multi-fallbacks and build multi-lookup table
    multis = {tuple(fb): cp for cp,fb in fbs.fbs.items() if len(fb) > 1}
    multi_data = []
    multi_offsets = {}
    multi_offsets_inv = {}
    for m in sorted(multis):
        multi_offsets[m] = len(multi_data)
        multi_offsets_inv[len(multi_data)] = m
        multi_data.extend(m)

    # produce output
    if outfile:
        with open(outfile, 'w') as out:
            me = os.path.basename(sys.argv[0])
            out.write(f'// This file has been auto-generated by {me}. DO NOT EDIT BY HAND!\n\n')
            out.write('#include "retrofont.h"\n\n')

            out.write('const RF_GlyphMapEntry RF_FallbackMap[] = {\n')
            for cp, fb in sorted(fbs.fbs.items()):
                cp_s = f"0x{cp:04X}"
                if len(fb) > 1:
                    fb_s = f"({len(fb)} codepoints)"
                    fb = (len(fb) << 24) | multi_offsets[tuple(fb)]
                else:
                    fb = fb[0]
                    fb_s = cps.name(fb)
                out.write(f'    {{ {cp_s:>8}, 0x{fb:08X} }},  // {cps.name(cp)} ===> {fb_s}\n')
            out.write('};\n\n')
            out.write(f'const uint32_t RF_FallbackMapSize = {len(fbs.fbs)};\n\n')

            out.write('const uint32_t RF_MultiFallbackData[] = {\n')
            for i, fb in enumerate(multi_data):
                if i in multi_offsets_inv:
                    cp_s = cps.name(multis[multi_offsets_inv[i]])
                fb_s = f"0x{fb:04X}"
                out.write(f'    {fb_s:>8},  // {cp_s} ===> {cps.name(fb)}\n')
            out.write('};\n')

    # print statistics
    if not(quiet) or not(outfile):
        print("codepoints with fallbacks:      ", len(fbs.fbs))
        print("... with single fallbacks:      ", sum((len(fb) == 1) for fb in fbs.fbs.values()))
        print("... with multiple fallbacks:    ", sum((len(fb) >  1) for fb in fbs.fbs.values()))
        print("maximum fallbacks per codepoint:", max(map(len, fbs.fbs.values())))
        print("total data size (bytes):        ", 4 * len(multi_data) + 8 * len(fbs.fbs) + 4)
