#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

uint32_t gen_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx;
    return RF_MapStandardColorToRGB(color, 0,160, 0,255);
}

void gen_prepare_cell(RF_RenderCommand* cmd) {
    if (cmd->fg == RF_COLOR_DEFAULT) { cmd->fg = 0xEEDC82; }
    if (cmd->bg == RF_COLOR_DEFAULT) { cmd->bg = 0x000000; }
    if (cmd->cell->dim) {
        cmd->fg = ((cmd->fg & 0xFEFEFE) + (cmd->bg & 0xFEFEFE)) >> 1;
    }
    cmd->reverse_cursor = cmd->is_cursor;
    cmd->bold = !!(cmd->cell->bold);
    cmd->underline = !!(cmd->cell->underline);
}

static const RF_SysClass genclass = {
    gen_map_border_color,
    gen_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

//                                 sys_id,            name,      class,    scrn, scrsz,   cellsz, fontsz, b_ul,  b_lr, aspect, blink, monitor,          default_font_id
const RF_System RF_Sys_Generic = { RF_COLOR_DEFAULT, "Generic", &genclass, NULL, {80,30}, {0,0},  {0,0},  {0,0}, {0,0}, {1,1},     0, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
