#!/usr/bin/env python3
import os
import sys
import urllib.request
from PIL import Image

URL = "https://www.masswerk.at/nowgobang/images/VTterm-roms.png"
IMGFILE = "decvt.cache.png"

if __name__ == "__main__":
    if not os.path.exists(IMGFILE):
        with urllib.request.urlopen(URL) as f_in, open(IMGFILE, 'wb') as f_out:
            f_out.write(f_in.read())
    src = Image.open(IMGFILE).convert('L')

    rw, rh =  8, 10  # ROM glyph width/height
    gw, gh = 10, 10  # output glyph width/height
    ncols = 16       # number of columns in output = rows in input
    sw, sh = 20, 24  # source cell width/height
    sz = 2   # source zoom
    y0 = 26  # source Y start position
    for destfile, nrows, x0 in (
        ( "decvt100.pbm",  8,   2),
        ( "decvt220.pbm", 18, 199),
    ):
        dest = Image.new('1', (gw * ncols, gh * nrows))
        for cy in range(nrows):
            for cx in range(ncols):
                # determine source glyph position
                sx = x0 + cy * sw
                sy = y0 + cx * sh
                # extract glyph from source
                g = src.crop((sx, sy, sx + rw * sz, sy + rh * sz)).resize((rw, rh), Image.NEAREST).tobytes()
                # convert to 2D nested list
                g = [list(g[o:o+rw]) for o in range(0, len(g), rw)]
                # pad to output size
                g = [r + [r[-1]] * (gw - rw) for r in g] + [[255] * gw for r in range(rh, gh)]
                # apply "dot stretching"
                g = [[r[0]] + [min(r[i], r[i-1]) for i in range(1,gw)] for r in g]
                # flatten to 1D again
                g = b''.join(map(bytes, g))
                # convert to image again
                g = Image.frombytes('L', (gw, gh), g).convert('1', dither=Image.Dither.NONE)
                # paste into destination
                dest.paste(g, (cx * gw, cy * gh))
        dest.save(destfile)
