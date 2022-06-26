# Supported Systems, Features and Limitations

General remarks:
- unless otherwise noted, the fonts are extracted from ROM files
- Unicode mappings are derived from a mixture of the following sources:
  - Rebecca Bettencourt's work for ISO L2/19-025 (which lead to the [Symbols for Legacy Computing](https://www.unicode.org/charts/PDF/U1FB00.pdf) Unicode block), specifically the [19025-aux-LegacyComputingSources.pdf](https://www.unicode.org/L2/L2019/19025-aux-LegacyComputingSources.pdf) document and its attachments (which are also available as a [ZIP file](https://www.unicode.org/L2/L2019/19025-aux-mappings.zip)).
  - Unicode character mappings files (from [unicode.org](https://unicode.org/Public/MAPPINGS/))
  - original research (i.e. assigning characters to suitable code points by hand), if necessary
- video timing and aspect ratio for systems with TV-standard video output is usually derived as follows:
  - visible line count of 240 (NTSC) or 288 (PAL)
  - vertical positioning is derived from the systems's video generator's default settings if possible (i.e. they are documented or can be read out without too much hassle); otherwise, the active image area is centered in the visible area
  - horizontal visible size is derived from either the system's horizontal total length ("clocks per line") or the pixel clock and vertical line count
    - in the absence of more accurate information, 18.75% of horizontal blanking are assumed (equivalent to a visible 52µs out of a total 64µs horizontal time)
  - the test application assumes a 4:3 aspect ratio for the whole visible area
    - pixel aspect ratio is implied from that
- only the system's default OS font rendering options are emulated, no special "software-based" rendering modes; this means:
  - no font sizes other than what the system's OS supports natively
  - no bold and underline styles unless the system's OS supports them
  - no blinking text unless there is hardware support for it
    - exception: software-driven blinking cursors are emulated if the system's OS does it too (e.g. Commodore KERNAL)

## "Generic" Pseudo-System
- supports any font size
- no color restrictions
- supports bold, dim and underline attributes
- default screen size of 80x30 cells, which makes a standard 4:3 aspect ratio at the default 8x16 font size

----------------------------------------------------------------------

# 16-Bit Systems

## IBM-compatible PC
- some additional font sources in addition to IBM ROMs:
  - ATI and Amstrad ROMs (which look very different from IBM's)
  - Microsofts EGA.CPI and ISO.CPI files
- default boot screen for pre-VGA systems is from a 5150 PC and MS-DOS 3.30
- default boot screen for VGA systems is from a 486DX2-based clone with AMIBIOS and MS-DOS 6.22
- 9-pixel wide rendering is emulated on all relevant systems (MDA, and EGA/VGA 350/400-line text modes)
  - replication of the 8th column into the 9th for box-drawing characters is emulated as well
- for EGA/VGA, modes come in three flavors:
  - standard text mode (with blinking)
  - text mode without blinking, but with all 16 colors available as background colors (Int 10h with AX=1003h BX=0000h)
  - graphics mode with 8-pixel wide fonts, no blinking at all, and 480 instead of 400 lines for VGA
- RGBI "dark yellow" color maps to brown, as on real CGA/EGA/VGA

## Commodore Amiga
- a pure text screen is emulated, **no** AmigaDOS window with border and title bar
- Topaz 8 (8x8) and 9 (9x10) fonts extracted from Kickstart 1.0, 1.3 and 2.0 ROMs
- NTSC (240-line) and PAL (256-line) configurations provided
- Kickstart 1.x and 2.x/3.x color scheme (and boot text) provided
- since there's no real standard palette, a 16-color mode is simulated
  - special Kickstart 1.x 4-color default palette entries are included: dark yellow maps to orange, dark blue is Kickstart 1's default blue background
- can use any font size
- bold and underline styles are supported, but blinking is not
- Topaz 9 and non-interlaced mode is default (as on the original hardware)
- interlaced modes default to Topaz 8, under the assumption that these are used to cram as much onto the screen as possible

## Atari ST
- supports low-res / med-res (color) and high-res (monochrome) mode
- 4-color (med-res) palette uses the ST's default black/green/red/white mapping
- default screen replicates TOS 2.x's startup screen, albeit without the Atari logo

----------------------------------------------------------------------

# 8-Bit Systems

## Apple I/II
- 40-column monochrome mode only (the color modes on that system were never suitable for text anyway)
- in a slight deviation from original hardware, default screens show _both_ the "Apple ][" text _and_ the BASIC prompt simultaneously
- the default screen for unexpanded Apple II replicates Integer BASIC (with "`>`" prompt); IIe replicates Applesoft BASIC ("`]`" prompt)
- cursor style differences and vertical alignment differences between the platforms are emulated
  - note that the Apple IIe/IIc cursor uses a special character for its prompt that's not present in most fonts; switching away from the default font in that system might thus lead to an invisible cursor
- Apple IIe/IIc font uses the MouseText variant with the "running man" icons
- Apple I and unexpanded II use a 6x8 font which is a manual pixel-by-pixel restoration of the Signetics 2513 ROM

## Commodore

- the following systems are emulated:
  - PET 2001 ("classic" PET)
  - PET 8032 (as a representative of the 80-column CBM machines)
  - VIC-20 (NTSC/PAL)
  - C64, SX-64, C128 (NTSC/PAL)
  - C16/C116/Plus4 "TED" machines (NTSC/PAL)
- palettes for the color systems are taken from [VICE](https://vice-emu.sourceforge.io)'s NTSC/PAL emulation
- the mapping of the 16 standard RGBI colors to VIC/-II/TED colors is done manually

## Atari 8-Bit

- only ANTIC mode 2 (40x24 monochrome text) is emulated
  - no per-cell attributes except reverse video
- background color can be set to anything (with the 16 standard RGBI colors mapped manually); foreground is forced to the same hue but different brightness, as on the original hardware
- NTSC/PAL palettes extracted from the [Atari800](https://atari800.github.io) emulator

## Sinclair ZX80, ZX81, Spectrum

- fonts extracted from screenshots (ROM doesn't contain all graphics characters)
- ZX80/81 system defaults to the ZX81 font
  - for ZX80 "emulation", just pick the ZX81 system and the ZX80 font
- ZX Spectrum default screen hides the cursor (as it does on the real device)

## Amstrad CPC

- modes 0, 1 and 2 supported
- mode 0 emulates the ROM's default 14-color palette, plus the 15th entry for everything that blinks
  - the 16th entry (green/blue blinking) is currently not used at all
- mode 1 emulates the ROM's default 4-color palette (blue/yellow/cyan/red)
  - background for non-reversed cells forced to blue, because with that palette, anything else would end up in quite garish color combinations
- mode 2 is strictly monochrome (blue/yellow)
- border color can be set to any of the 27 CPC-RGB colors
  - 4096-color palette from CPC+ is not emulated (it would only affect the border anyway)

## Robotron systems
Three very different systems manufactured by various companies in the former GDR, collectively produced under the "robotron" brand:
- HC900 / KC85/2 / KC85/3 / KC85/4 ([info](https://en.wikipedia.org/wiki/KC_85))
  - as far as text capabilities go, these are nearly identical (even though the 85/4 has a totally different memory mapping); the emulation follows the KC85/4 closely
  - four "generations" of KC85 fonts are provided (for CAOS <= 3.1,  3.3, 4.1/4.2 and 4.3/4.4)
- Z 9001 / KC85/1 / KC87 ([info](https://en.wikipedia.org/wiki/Robotron_Z1013))
- Z 1013 ([info](https://en.wikipedia.org/wiki/Robotron_Z1013))
- all Unicode mappings are original
  - these systems are obscure enough that no one bothered to create Unicode mappings for these ...
