#!/usr/bin/env python3
"""
Complete Unicode mappings in .fontspec files.

Variant 1: add name to codepoint
    map y.xx  U+1234  # <!>

Variant 2: add codepoint for name
    map y.xx  U+????  # LATIN SMALL LETTER X
"""
from generate_fallback_map import Codepoints
import sys
import io
import re

if __name__ == "__main__":
    args = sys.argv[1:]
    try:
        args.remove("-u")
        update = True
    except ValueError:
        update = False
    if (len(args) != 1) or ("-h" in args):
        print(__doc__.lstrip())
        print("Usage:", sys.argv[0], "[-u] <file.fontspec>")
        print("Use without -u to preview changes; use with -u to update file.")
        sys.exit(2)
    filename = args.pop()

    cps = Codepoints()
    out = io.StringIO()
    with open(filename, 'r') as f:
        for line in f:
            # CP -> name
            m = re.search(r'[uU]\+([0-9A-Fa-f]+).*(<!>)', line)
            if m:
                try:
                    name = cps[int(m.group(1), 16)].name
                except KeyError:
                    name = "(unknown)"
                line = line[:m.start(2)] + name + line[m.end(2):]

            # name -> CP
            m = re.search(r'([uU]\+\?+\s*).*#\s*(.*)', line)
            if m:
                try:
                    cp = cps.find(m.group(2))
                    cp = "U+{:04X}".format(cp.cp).ljust(len(m.group(1)))
                    line = line[:m.start(1)] + cp + line[m.end(1):]
                except KeyError:
                    pass

            out.write(line)

    if update:
        with open(filename, 'w') as f:
            f.write(out.getvalue())
    else:
        sys.stdout.write(out.getvalue())
