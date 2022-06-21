#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "retrofont.h"

const uint32_t atari8_palettes[] = {
    //             ---00---  ---01---  ---02---  ---03---  ---04---  ---05---  ---06---  ---07---  ---08---  ---09---  ---10---  ---11---  ---12---  ---13---  ---14---  ---15---
    /* NTSC b0 */  0x040404, 0x041600, 0x1C0800, 0x2F0000, 0x390000, 0x380020, 0x2B0047, 0x16005F, 0x010063, 0x000453, 0x001332, 0x001F07, 0x002500, 0x002400, 0x001C00, 0x120E00,
    /*      b1 */  0x1B1B1B, 0x1B2D00, 0x331F00, 0x460F00, 0x50040A, 0x4E0037, 0x42005E, 0x2D0275, 0x130B7A, 0x011B6A, 0x002A49, 0x00361E, 0x003C00, 0x003B00, 0x103300, 0x292600,
    /*      b2 */  0x333333, 0x334600, 0x4B3700, 0x5E2800, 0x681B23, 0x66134F, 0x5A1276, 0x45188D, 0x2C2492, 0x143382, 0x044261, 0x014E36, 0x03540A, 0x115300, 0x284B00, 0x413E00,
    /*      b3 */  0x4F4F4F, 0x4F6200, 0x675300, 0x7A4416, 0x84373F, 0x822F6B, 0x762E91, 0x6135A9, 0x4840AD, 0x314F9E, 0x205E7D, 0x196A53, 0x1E7027, 0x2E6F04, 0x446700, 0x5D5A00,
    /*      b4 */  0x686868, 0x687B08, 0x806C12, 0x935D2F, 0x9D5058, 0x9B4884, 0x8E47AA, 0x7A4EC1, 0x6159C6, 0x4A68B6, 0x397796, 0x32836B, 0x378840, 0x47871D, 0x5D8009, 0x76730B,
    /*      b5 */  0x898989, 0x899B29, 0xA18D34, 0xB47E51, 0xBD7179, 0xBC6AA5, 0xAF69CB, 0x9B6FE2, 0x827AE7, 0x6B89D7, 0x5A98B7, 0x54A38C, 0x59A961, 0x68A83E, 0x7EA12B, 0x97942D,
    /*      b6 */  0xABABAB, 0xABBE4C, 0xC3B057, 0xD6A173, 0xE0949C, 0xDE8CC7, 0xD18BED, 0xBD91FF, 0xA49DFF, 0x8DACF9, 0x7DBAD9, 0x76C6AF, 0x7BCC84, 0x8ACA61, 0xA1C34E, 0xB9B650,
    /*      b7 */  0xD3D3D3, 0xD3E574, 0xEBD77E, 0xFDC89B, 0xFFBBC3, 0xFFB4EF, 0xF9B3FF, 0xE4B9FF, 0xCCC4FF, 0xB5D3FF, 0xA4E2FF, 0x9EEDD6, 0xA3F3AB, 0xB2F289, 0xC8EA76, 0xE1DD78,
    /* PAL  b0 */  0x040404, 0x2C0100, 0x380000, 0x390000, 0x350024, 0x2A0044, 0x18005A, 0x00015A, 0x000A44, 0x001624, 0x002200, 0x002600, 0x002100, 0x041700, 0x1A0A00, 0x2C0100,
    /*      b1 */  0x1B1B1B, 0x431300, 0x4F0700, 0x50040A, 0x4C003B, 0x41005B, 0x2F0270, 0x061470, 0x00225B, 0x002D3B, 0x00390A, 0x003C00, 0x053800, 0x1B2E00, 0x312100, 0x431300,
    /*      b2 */  0x333333, 0x5B2C00, 0x671F10, 0x681B23, 0x641353, 0x591373, 0x481888, 0x1E2D88, 0x0D3A73, 0x034653, 0x015123, 0x0A5500, 0x1D5000, 0x334600, 0x493A00, 0x5B2C00,
    /*      b3 */  0x4F4F4F, 0x77480B, 0x833C2D, 0x84373F, 0x80306F, 0x752F8F, 0x6434A4, 0x3B49A4, 0x29568F, 0x1E626F, 0x1A6D3F, 0x27710B, 0x396C00, 0x4F6200, 0x655600, 0x77480B,
    /*      b4 */  0x686868, 0x906124, 0x9B5446, 0x9D5058, 0x984988, 0x8D48A7, 0x7C4DBC, 0x5462BC, 0x426FA7, 0x377A88, 0x338658, 0x408924, 0x52840D, 0x687B06, 0x7E6E0D, 0x906124,
    /*      b5 */  0x898989, 0xB08246, 0xBC7667, 0xBD7179, 0xB96AA9, 0xAE6AC8, 0x9D6EDD, 0x7583DD, 0x6490C8, 0x599BA9, 0x54A779, 0x61AA46, 0x74A52F, 0x899C27, 0x9F8F2F, 0xB08246,
    /*      b6 */  0xABABAB, 0xD3A468, 0xDE988A, 0xE0949C, 0xDB8DCB, 0xD08CEA, 0xC091FF, 0x97A5FF, 0x86B2EA, 0x7BBECB, 0x77C99C, 0x84CC68, 0x96C852, 0xABBE4A, 0xC1B252, 0xD3A468,
    /*      b7 */  0xD3D3D3, 0xFACC90, 0xFFC0B1, 0xFFBBC3, 0xFFB4F3, 0xF8B4FF, 0xE7B8FF, 0xBFCDFF, 0xAED9FF, 0xA3E5F3, 0x9FF0C3, 0xACF490, 0xBEEF7A, 0xD3E672, 0xE8D97A, 0xFACC90,
};

const uint8_t atari8_default_color_map[16] = {
    // 0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
    // K     B     G     C     R     M     Y    LG    DG    LB    LG    LC    LR    LM    LY     W
    0x00, 0x19, 0x1D, 0x1A, 0x14, 0x16, 0x22, 0x50, 0x20, 0x59, 0x5D, 0x5A, 0x54, 0x56, 0x7F, 0x70,
};

uint8_t atari8_map_color(uint32_t sys_id, uint32_t color, uint8_t default1) {
    bool is_pal = (RF_EXTRACT_ID(sys_id, 3) == 'P');
    if (color == RF_COLOR_DEFAULT) {
        color = default1;
    } else {
        color = RF_MapRGBToStandardColor(color, 200);  // FIXME: perform proper color mapping
        color = atari8_default_color_map[color & 15];
        if (is_pal && (color & 0x0F)) {
            // shift hue for PAL
            color = ((color + 0x0E) & 0x0F) | (color & 0x70);
        }
    }
    if (is_pal) { color |= 0x80; }
    return (uint8_t)color;
}

uint32_t atari8_map_border_color(uint32_t sys_id, uint32_t color) {
    return atari8_palettes[atari8_map_color(sys_id, color, 0)];
}

void atari8_render_cell(const RF_RenderCommand* cmd) {
    uint8_t fg, bg;
    if (!cmd || !cmd->cell) { return; }
    bg = atari8_map_color(cmd->ctx->system->sys_id, cmd->ctx->default_bg, 0x29);
    fg = atari8_map_color(cmd->ctx->system->sys_id, cmd->ctx->default_fg, 0x50);
    fg = (fg & 0xF0) | (bg & 0x0F);  // ANTIC mode 2 color mapping
    RF_RenderCell(cmd, atari8_palettes[fg], atari8_palettes[bg], 0,0, 0,0, cmd->is_cursor, false, false, false);
}

const RF_SysClass a8class = {
    atari8_map_border_color,
    atari8_render_cell,
    NULL,  // check_font
};

static const char a8default[] =
    "\n  READY"
    "\n  ";

//                                     sys_id,                      name,                       class,    scrn,       scrsz,   cellsz,  fontsz,  b_ul,    b_lr,  aspect, blink, monitor,          default_font_id
const RF_System RF_Sys_Atari8_NTSC = { RF_MAKE_ID('A','8','0','N'), "Atari 400/800/XL (NTSC)",  &a8class, a8default, {40,24}, { 8, 8}, { 8, 8}, {32,20}, {24,20}, {1,1},     0, RF_MONITOR_COLOR, RF_MAKE_ID('A','8','0','0') };
const RF_System RF_Sys_Atari8_PAL  = { RF_MAKE_ID('A','8','0','P'), "Atari 400/800/XL (PAL)",   &a8class, a8default, {40,24}, { 8, 8}, { 8, 8}, {32,44}, {24,44}, {1,1},     0, RF_MONITOR_COLOR, RF_MAKE_ID('A','8','0','0') };
