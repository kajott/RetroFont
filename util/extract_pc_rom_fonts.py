#!/usr/bin/env python3
"""
Extract fonts from PC ROMs.
Input is a directory of PC ROM files (e.g. the PCem roms/ directory),
output is a bunch of PBM images.
Use the '-a' switch to extract everything that looks like a font.
"""
import sys
import os

def SearchFonts(data, height):
    # valid fonts are all-empty at character indices 0, 32 and 255, and fully filled at index 291
    char0   = b'\x00' * height
    char219 = b'\xff' * height
    fontsize = height * 256
    pos = 0
    while True:
        pos = data.find(char0, pos)
        if (pos < 0) or (pos > (len(data) - fontsize)): break
        if  (data[pos + height *  32 : pos + height *  33] == char0) \
        and (data[pos + height * 255 : pos + height * 256] == char0) \
        and (data[pos + height * 219 : pos + height * 220] == char219):
            yield pos
        pos += 1

def SaveFont(filename, data, height=0, offset=0):
    if not height:
        height = (len(data) - offset) >> 8
    pbm = b''
    for row in range(8):
        slice = data[offset+row*32*height : offset+(row+1)*32*height]
        for scanline in range(height):
            pbm += slice[scanline::height]
    print("->", filename)
    with open(filename, 'wb') as f:
        f.write(f"P4\n256 {8*height}\n".encode())
        f.write(pbm)

def ExtractMatching(suffix, data, heights, index=0):
    if isinstance(heights, int):
        heights = [heights]
    for height in heights:
        for i, offset in enumerate(SearchFonts(data, height)):
            if i == index:
                SaveFont(f"pc_8x{height}_{suffix}.pbm", data, height, offset)
                break

patterns = []
def register_pattern(*pats):
    def reg(func):
        patterns.append([func] + list(pats))
    return reg


@register_pattern("ibm", "vga")
def extract_vga(data):
    ExtractMatching("vga", data, (8, 14, 16))

@register_pattern("mach64gx", "bios")
def extract_ati(data):
    ExtractMatching("ati", data, (8, 14, 16))

@register_pattern("pc1640", "40100")
def extract_amstrad(data):
    ExtractMatching("amstrad", data, 8)

@register_pattern("mda")
def extract_mda(data):
    assert len(data) == 8192
    # interleave 8x14 font
    ifnt = b''
    for c in range(256):
        ifnt += data[c*8 : (c+1)*8] + data[0x800+c*8 : 0x806+c*8]
    SaveFont("pc_8x14_mda.pbm", ifnt)
    SaveFont("pc_8x8_mda_thin.pbm",  data, 8, 0x1000)
    SaveFont("pc_8x8_mda_thick.pbm", data, 8, 0x1800)


if __name__ == "__main__":
    args = sys.argv[1:]
    blind_scan = ("-a" in args)
    if blind_scan: args.remove("-a")

    for root, dirs, files in os.walk(args.pop()):
        for f in files:
            path = os.path.join(root, f)
            load = blind_scan
            extractor = None

            if not blind_scan:
                for func, *pats in patterns:
                    if all(p in path.lower() for p in pats):
                        extractor = func
                        break

            if blind_scan or extractor:
                print("<-", path)
                with open(path, 'rb') as f:
                    data = f.read()

            if blind_scan:
                basename = "{}_{}".format(os.path.basename(os.path.dirname(path)), os.path.splitext(os.path.basename(path))[0]).replace(' ', '_').lower()
                for height in (8, 14, 16):
                    for index, offset in enumerate(SearchFonts(data, height)):
                        SaveFont(f"extracted_8x{height}_{basename}_{index}.pbm", data, height, offset)
            elif extractor:
                extractor(data)
