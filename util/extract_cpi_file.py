#!/usr/bin/env python3
"""
Extract fonts from MS-DOS/PC-DOS/Win9x CPI files.
Input is a .cpi file, output is a bunch of .pbm files.
No error checking is performed.
"""
import struct
import sys
import os

CharsPerScanline = 32

def read_struct(f, fmt):
    res = struct.unpack(fmt, f.read(struct.calcsize(fmt)))
    return res if (len(res) > 1) else res[0]

if __name__ == "__main__":
    infile = sys.argv[1]
    cp_filter = set(map(int, sys.argv[2:]))
    out_base = os.path.basename(infile).replace('.', '_')
    f = open(infile, 'rb')

    magic, fih_offset = read_struct(f, '<8s8x3xI')
    f.seek(fih_offset)
    num_codepages = read_struct(f, '<H')
    print("file contains", num_codepages, "codepages:")
    next_cpeh_offset = f.tell()

    for dummy in range(num_codepages):
        f.seek(next_cpeh_offset)
        next_cpeh_offset, device_type, device_name, codepage, cpih_offset = \
            read_struct(f, '<2xIH8sH6xI')
        device_name = device_name.strip(b'\x00 ').decode(errors='replace')
        if device_type != 1:
            print(f"- cp{codepage} font for printer device '{device_name}'")
            continue

        f.seek(cpih_offset)
        num_fonts = read_struct(f, '<2xH2x')
        extract = not(cp_filter) or (codepage in cp_filter)
        mode = ":" if extract else " [ignored]"
        print(f"- cp{codepage} font for screen device '{device_name}' in {num_fonts} sizes{mode}")
        if not extract:
            continue

        for dummy in range(num_fonts):
            height, width, num_chars = read_struct(f, '<BB2xH')
            print(f"  - {num_chars} characters of size {width}x{height}", end=' ')
            data = f.read(num_chars * height * ((width + 7) >> 3))
            if (width != 8) or (num_chars % CharsPerScanline):
                print("[unsupported parameters]")
                continue
            filename = f"{out_base}_cp{codepage}_{width}x{height}.pbm"
            print("=>", filename)

            with open(filename, 'wb') as out:
                out.write(f"P4\n{CharsPerScanline*width} {num_chars//CharsPerScanline*height}\n".encode())
                for row in range(8):
                    slice = data[row*CharsPerScanline*height : (row+1)*CharsPerScanline*height]
                    for scanline in range(height):
                        out.write(slice[scanline::height])
