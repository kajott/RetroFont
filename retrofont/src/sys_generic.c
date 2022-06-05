#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

uint32_t gen_map_color(uint32_t color, uint32_t default1, uint32_t default2) {
    if (color == RF_COLOR_DEFAULT) { color = default1; }
    if (color == RF_COLOR_DEFAULT) { color = default2; }
    return RF_MapStandardColorToRGB(color, 0,160, 0,255);
}

uint32_t gen_map_border_color (uint32_t sys_id, uint32_t color) {
    return gen_map_color(color, 0, sys_id);
}

void gen_render_cell (const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    if (!cmd || !cmd->cell) { return; }
    fg = gen_map_color(cmd->cell->fg, cmd->ctx->default_fg, 0xEEDC82);
    bg = gen_map_color(cmd->cell->bg, cmd->ctx->default_bg, 0x000000);
    if (cmd->cell->dim) {
        fg = ((fg & 0xFEFEFE) + (bg & 0xFEFEFE)) >> 1;
    }
    RF_RenderCell(cmd, fg, bg, 0,0, 0,0, cmd->is_cursor, false, true, true);
}

const RF_SysClass genclass = {
    gen_map_border_color,
    gen_render_cell,
    NULL,  // check_font
};

//                                 sys_id,            name,      class,    scrsz,   cellsz, fontsz, b_ul,  b_lr, aspect, blink, default_font_id
const RF_System RF_Sys_Generic = { RF_COLOR_DEFAULT, "Generic", &genclass, {80,24}, {0,0},  {0,0},  {0,0}, {0,0}, {1,1},     0, RF_MAKE_ID('P','C','6','V') };
