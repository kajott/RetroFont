#!/usr/bin/env python3
"""
Extract fonts from Amiga ROMs.
Input is a directory of Amiga ROM files (e.g. the WinUAE roms/ directory),
output is a bunch of PBM images.
"""
import hashlib
import struct
import sys
import os

CharsPerLine = 32
assert not(224 % CharsPerLine)

# see: https://wiki.amigaos.net/wiki/Graphics_Library_and_Text#The_Composition_of_a_Bitmap_Font_on_Disk

# "signature" to identify a struct TextFont as used by the Topazes
#         BaselineBoldSmeaAccessorLoChHiCh
#         v       v       v       v   v
Magic = b'\x00\x06\x00\x01\x99\x20\x20\xFF'
MagicOffset = 6


def ProcessFont(rom, mask, offset):
    # parse and check (relevant parts of) struct TextFont header
    y_size, x_size, char_data, modulo, char_loc, char_space, char_kern = \
        struct.unpack('>H2xH8xIHIII', rom[offset:offset+32])
    if (y_size > 16) or (x_size > 16) or (modulo > (224 * 2)):
        return  # implausible header: font too large
    char_data  &= mask;  char_data_end  = char_data  + modulo * y_size
    char_loc   &= mask;  char_loc_end   = char_loc   + 224 * 4
    char_space &= mask;  char_space_end = char_space + 224 * 2
    char_kern  &= mask;  char_kern_end  = char_kern  + 224 * 2
    if max(char_data_end, char_loc_end) > len(rom):
        return  # implausible header: data points outside of ROM

    # parse char_loc table
    char_loc = struct.unpack(">" + "H" * (224 * 2), rom[char_loc : char_loc_end])
    char_width = char_loc[1::2]
    char_loc   = char_loc[0::2]
    if max(char_width) > x_size:
        return  # implausibly high character widths
    if max(a+b for a,b in zip(char_width, char_loc)) > (modulo * 8):
        return  # implausibly high character end address

    # parse char_space and char_kern tables
    if char_space:
        char_space = struct.unpack(">" + "h" * 224, rom[char_space : char_space_end])
    else:
        char_space = [x_size] * 224
    if char_kern:
        char_kern = struct.unpack(">" + "h" * 224, rom[char_kern : char_kern_end])
    else:
        char_kern = [0] * 224
    if set(a+b for a,b in zip(char_space, char_kern)) != {x_size}:
        return  # font is proportional
    if max(a+b for a,b in zip(char_width, char_kern)) > x_size:
        return  # character(s) extend past the cell

    # parse char_data
    data_in = []
    for y in range(y_size):
        row = []
        for dbyte in rom[char_data + y * modulo : char_data + (y + 1) * modulo]:
            row.extend((dbyte >> bit) & 1 for bit in (7,6,5,4,3,2,1,0))
        data_in.append(row)

    # assemble output bitmap
    data_out = []
    for char_start in range(0, 224, CharsPerLine):
        rows = [[] for y in range(y_size)]
        for char_index in range(char_start, char_start + CharsPerLine):
            lpad  = [0] * char_kern[char_index]
            loc   = char_loc[char_index]
            width = char_width[char_index]
            end   = loc + width
            rpad  = [0] * (x_size - width - len(lpad))
            for y in range(y_size):
                rows[y].extend(lpad)
                rows[y].extend(data_in[y][loc : end])
                rows[y].extend(rpad)
        data_out.extend(rows)
    assert len(data_out) == ((224 // CharsPerLine) * y_size)
    assert set(map(len, data_out)) == {x_size * CharsPerLine}

    # pack PBM bitmap data and write file
    bitmap = b''.join(bytes(sum(row[i+7-b]<<b for b in range(8)) for i in range(0, len(row), 8))
                      for row in data_out)
    h = hashlib.sha1(bitmap).hexdigest()[:6].upper()
    filename = f"amiga_{x_size}x{y_size}_{h}.pbm"
    print("-->", filename)
    with open(filename, 'wb') as f:
        f.write(f"P4\n{len(data_out[0])} {len(data_out)}\n".encode())
        f.write(bitmap)


if __name__ == "__main__":
    have_fonts = set()
    for root, dirs, files in os.walk(sys.argv[1]):
        for filename in files:
            if not(os.path.splitext(filename)[-1].strip('.').lower() in ("rom", "bin")):
                continue
            with open(os.path.join(root, filename), 'rb') as f:
                rom = f.read()
            print("<--", filename)

            # determine address mask from ROM size
            mask = len(rom) - 1
            for shift in (1,2,4,8,16):
                mask |= mask >> shift

            # search for font signatures
            pos = 0
            while True:
                pos = rom.find(Magic, pos)
                if pos < MagicOffset:
                    break
                ProcessFont(rom, mask, pos - MagicOffset)
                pos += 1
