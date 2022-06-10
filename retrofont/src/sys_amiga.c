#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

uint32_t amiga_map_color(uint32_t sys_id, uint32_t color, uint32_t default1, bool fg) {
    if (color == RF_COLOR_DEFAULT) { color = default1; }
    if (color == RF_COLOR_DEFAULT) {
        if (((sys_id >> 8) & 0xFF) == '1') {
            color = fg ? (RF_COLOR_WHITE | RF_COLOR_BRIGHT) : RF_COLOR_BLUE;
        } else {
            color = fg ? RF_COLOR_BLACK                     : RF_COLOR_WHITE;
        }
    }
    color = RF_MapRGBToStandardColor(color, 200);
    switch (color) {
        case RF_COLOR_BLUE:                    return 0x0055AA;
        case RF_COLOR_YELLOW:                  return 0xFF8000;
        case RF_COLOR_BLACK | RF_COLOR_BRIGHT: return 0x555555;
        default: return RF_MapStandardColorToRGB(color, 0x00,0xAA, 0x00,0xFF);
    }
}

uint32_t amiga_map_border_color(uint32_t sys_id, uint32_t color) {
    return amiga_map_color(sys_id, color, RF_COLOR_DEFAULT, false);
}

void amiga_render_cell(const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    if (!cmd || !cmd->cell) { return; }
    fg = amiga_map_color(cmd->ctx->system->sys_id, cmd->cell->fg, cmd->ctx->default_fg, true);
    bg = amiga_map_color(cmd->ctx->system->sys_id, cmd->cell->bg, cmd->ctx->default_bg, false);
    RF_RenderCell(cmd, fg, bg, 0,0, 0,0, cmd->is_cursor, false, true, true);
}

bool amiga_check_font(uint32_t sys_id, const RF_Font* font) {
    // reject tall fonts in non-interlaced mode, 'cause that'd look ridiculous
    return ((sys_id >> 24) == 'i')
        || ((font->font_size.y * 2) < (font->font_size.x * 3));
}

const RF_SysClass amigaclass = {
    amiga_map_border_color,
    amiga_render_cell,
    amiga_check_font,
};

//                                          sys_id,                       name,                                              class,      scrn,  scrsz,                                  cellsz, fontsz,  b_ul,    b_lr,   aspect, blink, default_font_id
const RF_System RF_Sys_Amiga_KS1_NTSC  = { RF_MAKE_ID('A','1','N','p'), "Commodore Amiga (Kickstart 1.x, NTSC)",            &amigaclass, NULL, {640|RF_SIZE_PIXELS,200|RF_SIZE_PIXELS}, {0,0},  {0,0},  {60,20}, {58,20}, {1,2},      0, RF_MAKE_ID('T','8','1','2') };
const RF_System RF_Sys_Amiga_KS1_NTSCi = { RF_MAKE_ID('A','1','N','i'), "Commodore Amiga (Kickstart 1.x, NTSC interlaced)", &amigaclass, NULL, {640|RF_SIZE_PIXELS,400|RF_SIZE_PIXELS}, {0,0},  {0,0},  {60,40}, {58,40}, {1,1},      0, RF_MAKE_ID('T','9','1','2') };
const RF_System RF_Sys_Amiga_KS1_PAL   = { RF_MAKE_ID('A','1','P','p'), "Commodore Amiga (Kickstart 1.x, PAL)",             &amigaclass, NULL, {640|RF_SIZE_PIXELS,256|RF_SIZE_PIXELS}, {0,0},  {0,0},  {48,16}, {48,16}, {1,2},      0, RF_MAKE_ID('T','8','1','2') };
const RF_System RF_Sys_Amiga_KS1_PALi  = { RF_MAKE_ID('A','1','P','i'), "Commodore Amiga (Kickstart 1.x, PAL interlaced)",  &amigaclass, NULL, {640|RF_SIZE_PIXELS,512|RF_SIZE_PIXELS}, {0,0},  {0,0},  {48,32}, {48,32}, {1,1},      0, RF_MAKE_ID('T','9','1','2') };
const RF_System RF_Sys_Amiga_KS2_NTSC  = { RF_MAKE_ID('A','2','N','p'), "Commodore Amiga (Kickstart 2/3, NTSC)",            &amigaclass, NULL, {640|RF_SIZE_PIXELS,200|RF_SIZE_PIXELS}, {0,0},  {0,0},  {60,20}, {58,20}, {1,2},      0, RF_MAKE_ID('T','8','2','0') };
const RF_System RF_Sys_Amiga_KS2_NTSCi = { RF_MAKE_ID('A','2','N','i'), "Commodore Amiga (Kickstart 2/3, NTSC interlaced)", &amigaclass, NULL, {640|RF_SIZE_PIXELS,400|RF_SIZE_PIXELS}, {0,0},  {0,0},  {60,40}, {58,40}, {1,1},      0, RF_MAKE_ID('T','9','2','0') };
const RF_System RF_Sys_Amiga_KS2_PAL   = { RF_MAKE_ID('A','2','P','p'), "Commodore Amiga (Kickstart 2/3, PAL)",             &amigaclass, NULL, {640|RF_SIZE_PIXELS,256|RF_SIZE_PIXELS}, {0,0},  {0,0},  {48,16}, {48,16}, {1,2},      0, RF_MAKE_ID('T','8','2','0') };
const RF_System RF_Sys_Amiga_KS2_PALi  = { RF_MAKE_ID('A','2','P','i'), "Commodore Amiga (Kickstart 2/3, PAL interlaced)",  &amigaclass, NULL, {640|RF_SIZE_PIXELS,512|RF_SIZE_PIXELS}, {0,0},  {0,0},  {48,32}, {48,32}, {1,1},      0, RF_MAKE_ID('T','9','2','0') };
