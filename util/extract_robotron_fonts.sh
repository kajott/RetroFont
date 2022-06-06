#!/bin/bash

BASEDIR="$1"
[ -z "$BASEDIR" ] && BASEDIR="."

python3 extract_font.py -a 0E00:1000,1E00:2000 -o kc_caos31.pbm "$BASEDIR/kc85/caos__e0.853"  # HC900 + HC900-CAOS / KC85/2 + HC-CAOS 2.2 / KC5/3 + HC-CAOS 3.1
python3 extract_font.py -a 0E00:1000,1E00:2000 -o kc_caos33.pbm "$BASEDIR/kc85/caos33.853"    # KC85/3 + KC-CAOS 3.3
python3 extract_font.py -a 0E00:1000,1E00:2000 -o kc_caos41.pbm "$BASEDIR/kc85/caos__e0.854"  # KC85/4 + KC-CAOS 4.1-4.2 (thin exclamation mark)
python3 extract_font.py -a 0E00:1000,1E00:2000 -o kc_caos43.pbm "$BASEDIR/kc85/caos__e0.855"  # KC85/3 + HC-CAOS 3.4i / KC85/5 KC-CAOS 4.3-4.4 (thick exclamation mark)

python3 extract_font.py -c 512 -o z1013.pbm "$BASEDIR/z1013/z1013_zg.rom"
python3 extract_font.py -a 0100 -o z9001.pbm "$BASEDIR/z9001/chargen.851"
python3 extract_font.py -a 0000:0400 -o mugler.pbm "$BASEDIR/muglerpc/chargen.pcm"
