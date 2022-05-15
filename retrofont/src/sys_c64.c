#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

const uint32_t c64_palettes[2][16] = {
    /* 0: PAL  */  { 0x000000, 0xFFFFFF, 0x924A40, 0x84C5CC, 0x9351B6, 0x72B14B, 0x483AAA, 0xD5DF7C, 0x99692D, 0x675200, 0xC18178, 0x606060, 0x8A8A8A, 0xB3EC91, 0x867ADE, 0xB3B3B3 },
    /* 1: NTSC */  { 0x000000, 0xFFFFFF, 0x7E352B, 0x6EB7C1, 0x7F3BA6, 0x5CA035, 0x332799, 0xCBD765, 0x85531C, 0x503C00, 0xB46B61, 0x4A4A4A, 0x757575, 0xA3E77C, 0x7064D6, 0xA3A3A3 }
};

const uint8_t c64_default_color_map[16] = { 0, 6, 5, 3, 2, 4, 8, 15, 11, 14, 13, 3, 10, 4, 7, 1 };

uint32_t c64_map_color(uint32_t sys_id, uint32_t color, bool is_fg) {
    const uint32_t *pal = c64_palettes[(sys_id == RF_MAKE_ID('C','6','4','N')) ? 1 : 0];
    if (color == RF_COLOR_DEFAULT) {
        return is_fg ? pal[14] : pal[6];
    }
    color = RF_MapRGBToStandardColor(color, 200);  // TODO: do *not* do this (it loses fidelity!), but map RGB color to C64 color below
    if (RF_IS_STD_COLOR(color)) {
        return pal[c64_default_color_map[color & 15]];
    }
    return RF_MapStandardColorToRGB(color, 0, 170, 0, 255);  // TODO: map color properly here
}

uint32_t c64_map_border_color(uint32_t sys_id, uint32_t color) {
    return c64_map_color(sys_id, color, true);
}

void c64_render_cell(const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    if (!cmd || !cmd->cell) { return; }

    fg = cmd->cell->fg;
    if (fg == RF_COLOR_DEFAULT) { fg = cmd->default_fg; }
    fg = c64_map_color(cmd->sys_id, fg, true);
    bg = c64_map_color(cmd->sys_id, cmd->default_bg, false);

    RF_RenderCell(cmd, fg, bg, 0,0, 0,0, cmd->is_cursor && !cmd->blink_phase, false, false, false);
}

const RF_SysClass c64class = {
    c64_map_border_color,
    c64_render_cell
};

//                                  sys_id,             name,                   class,      scrsz,   cellsz,  fontsz,  b_ul,    b_lr,  double, blink, default_font_id
const RF_System RF_Sys_C64_PAL  = { RF_MAKE_ID('C','6','4','P'), "Commodore C64 (PAL)",  &c64class, {40,25}, { 8, 8}, { 8, 8}, {42,42}, {42,42}, false,     0, RF_MAKE_ID('C','6','4','s') };
const RF_System RF_Sys_C64_NTSC = { RF_MAKE_ID('C','6','4','N'), "Commodore C64 (NTSC)", &c64class, {40,25}, { 8, 8}, { 8, 8}, {46,17}, {46,17}, false,     0, RF_MAKE_ID('C','6','4','s') };
