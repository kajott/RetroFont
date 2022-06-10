#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

uint32_t st_map_color(uint32_t sys_id, uint32_t color, uint32_t default1, uint32_t default2) {
    if (color == RF_COLOR_DEFAULT) { color = default1; }
    if (color == RF_COLOR_DEFAULT) { color = default2; }
    color = RF_MapRGBToStandardColor(color, 200);
    if ((sys_id >> 24) == 'H') {
        // high-res: reduce everything to black & white
        color &= ~RF_COLOR_BLACK;
        color = ((color & RF_COLOR_GREEN) || ((color & RF_COLOR_RED) && (color & RF_COLOR_BLUE)))
              ? (RF_COLOR_WHITE | RF_COLOR_BRIGHT) : RF_COLOR_BLACK;
    }
    else if ((sys_id >> 24) == 'M') {
        // medium-res: palette only has black, white, red, green
        static const uint32_t medres_map[4] = {
            RF_COLOR_BLACK,                    // black  and blue    -> black
            RF_COLOR_GREEN,                    // green  and cyan    -> green
            RF_COLOR_RED,                      // red    and magenta -> red
            RF_COLOR_WHITE | RF_COLOR_BRIGHT,  // yellow and white   -> white
        };
        color = medres_map[(color & 7) >> 1];
    }
    if (color == RF_COLOR_WHITE) { return 0xA0A0A0; }
    return RF_MapStandardColorToRGB(color, 0x00,0xFF, 0x60,0xFF);
}

uint32_t st_map_border_color (uint32_t sys_id, uint32_t color) {
    return st_map_color(sys_id, color, RF_COLOR_WHITE | RF_COLOR_BRIGHT, RF_COLOR_WHITE | RF_COLOR_BRIGHT);
}

void st_render_cell (const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    if (!cmd || !cmd->cell) { return; }
    fg = st_map_color(cmd->ctx->system->sys_id, cmd->cell->fg, cmd->ctx->default_fg, RF_COLOR_BLACK);
    bg = st_map_color(cmd->ctx->system->sys_id, cmd->cell->bg, cmd->ctx->default_bg, RF_COLOR_WHITE | RF_COLOR_BRIGHT);
    RF_RenderCell(cmd, fg, bg, 0,0, 0,0, cmd->is_cursor, false, false, false);
}

const RF_SysClass stclass = {
    st_map_border_color,
    st_render_cell,
    NULL,  // check_font
};

#define STDEFAULT "`y12Memory Test:\nST RAM       `+r  1024 KB`0\nMemory Test Complete.\n\n"

//                                   sys_id,                       name,                 class,   scrn,       scrsz,  cellsz, fontsz,  b_ul,    b_lr,  aspect, blink, default_font_id
const RF_System RF_Sys_ST_LowRes = { RF_MAKE_ID('S','T','E','L'), "Atari ST LowRes",    &stclass, STDEFAULT, {40,25}, {8,8},  {8,8},  {32,30}, {32,40}, {1,1},     0, RF_MAKE_ID('S','T','0','8') };
const RF_System RF_Sys_ST_MedRes = { RF_MAKE_ID('S','T','E','M'), "Atari ST MediumRes", &stclass, STDEFAULT, {80,25}, {8,8},  {8,8},  {64,30}, {64,40}, {1,2},     0, RF_MAKE_ID('S','T','0','8') };
const RF_System RF_Sys_ST_HiRes  = { RF_MAKE_ID('S','T','E','H'), "Atari ST HighRes",   &stclass, STDEFAULT, {80,25}, {8,16}, {8,16}, {64,64}, {64,80}, {1,1},     0, RF_MAKE_ID('S','T','1','6') };
