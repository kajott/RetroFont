#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

#define IS_MDA(sys_id)  ((sys_id) == RF_MAKE_ID('M','D','A','8'))

#define pc_std_color(color) RF_MapRGBToStandardColor(color, 200)

uint32_t pc_map_color(uint32_t color, uint32_t default1, uint32_t default2) {
    if (color == RF_COLOR_DEFAULT) { color = default1; }
    if (color == RF_COLOR_DEFAULT) { color = default2; }
    return pc_std_color(color);
}

uint32_t pc_rgb_color(uint32_t color) {
    if (color == RF_COLOR_YELLOW) {
        return 0xAA5500;
    } else {
        return RF_MapStandardColorToRGB(color, 0,170, 85,255);
    }
}

uint32_t pc_map_border_color (uint32_t sys_id, uint32_t color) {
    if (IS_MDA(sys_id) || (color == RF_COLOR_DEFAULT)) {
        return 0;
    } else {
        return pc_rgb_color(pc_std_color(color));
    }
}

void pc_render_cell (const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    uint16_t cs = 0, ce = 0;
    bool is_gfx;
    if (!cmd || !cmd->cell) { return; }

    // resolve to standard color
    fg = pc_map_color(cmd->cell->fg, cmd->ctx->default_fg, RF_COLOR_WHITE);
    bg = pc_map_color(cmd->cell->bg, cmd->ctx->default_bg, RF_COLOR_BLACK);

    // map MDA color: any FG color becomes white, only white BG is white
    if (IS_MDA(cmd->ctx->system->sys_id)) {
        if ((fg & RF_COLOR_WHITE) != RF_COLOR_BLACK)
            { fg = RF_COLOR_WHITE | (fg & RF_COLOR_BRIGHT); }
        if ((bg & RF_COLOR_WHITE) != RF_COLOR_WHITE)
            { bg = RF_COLOR_BLACK | (bg & RF_COLOR_BRIGHT); }
    }

    // map color back to RGB
    fg = pc_rgb_color(fg);
    bg = pc_rgb_color(bg);

    // set cursor
    is_gfx = (((cmd->ctx->system->sys_id >> 16) & 0xFF) == 'g');
    if (cmd->is_cursor && !cmd->blink_phase && !is_gfx) {
        ce = cmd->ctx->cell_size.y;
        if (ce > 8) { --ce; }
        cs = cmd->ctx->insert ? (ce - 2) : (cmd->ctx->cell_size.y >> 1);
    }

    // render the main cell
    RF_RenderCell(cmd, fg,bg, 0,0, cs,ce,
        is_gfx && cmd->is_cursor,
        IS_MDA(cmd->ctx->system->sys_id) && cmd->cell->blink && cmd->blink_phase,
        IS_MDA(cmd->ctx->system->sys_id) || is_gfx,
        false);

    // replicate the 9th column if required
    // - on actual VGA, this is done for characters 0xC0 to 0xDF;
    // on codepage 437 (which is the only one we're interested about there),
    // this maps to the Unicode block "Box Drawing";
    // in fact, a few additional characters map there (0xB3 to 0xBF,
    // to be specific), but none of those extends to the right (of course,
    // because otherwise it would've been broken on real MDA/EGA/VGA too),
    // so it's fine to treat the whole block as eligible
    if ((cmd->ctx->cell_size.x == 9) && ((cmd->cell->codepoint & (~0x7F)) == 0x2500)) {
        uint8_t *p = &cmd->pixel[7 * 3];
        for (uint16_t i = cmd->ctx->cell_size.y;  i;  --i) {
            p[3] = p[0];
            p[4] = p[1];
            p[5] = p[2];
            p += cmd->ctx->stride;
        }
    }
}

const RF_SysClass pcclass = {
    pc_map_border_color,
    pc_render_cell
};

//                                 sys_id,                      name,                      class,    scrsz,    cellsz,  fontsz,  b_ul,  b_lr, aspect, blink, default_font_id
const RF_System RF_Sys_MDA     = { RF_MAKE_ID('M','D','A','8'), "PC (MDA)",                &pcclass, {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   228, RF_MAKE_ID('P','C','4','M') };
const RF_System RF_Sys_CGA40   = { RF_MAKE_ID('C','G','A','4'), "PC (CGA 40x25 text)",     &pcclass, {40,25}, { 8, 8}, { 8, 8}, {4,4}, {4, 4}, {1,1},   266, RF_MAKE_ID('P','C','8','C') };
const RF_System RF_Sys_CGA80   = { RF_MAKE_ID('C','G','A','8'), "PC (CGA 80x25 text)",     &pcclass, {80,25}, { 8, 8}, { 8, 8}, {8,4}, {8, 4}, {1,2},   266, RF_MAKE_ID('P','C','8','C') };
const RF_System RF_Sys_EGA25   = { RF_MAKE_ID('E','G','A','2'), "PC (EGA 80x25 text)",     &pcclass, {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   266, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_EGA43   = { RF_MAKE_ID('E','G','A','4'), "PC (EGA 80x43 text)",     &pcclass, {80,43}, { 9, 8}, { 8, 8}, {8,7}, {8, 9}, {1,1},   266, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_EGA25_G = { RF_MAKE_ID('E','G','g','2'), "PC (EGA 80x25 graphics)", &pcclass, {80,25}, { 8,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   266, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_EGA43_G = { RF_MAKE_ID('E','G','g','4'), "PC (EGA 80x43 graphics)", &pcclass, {80,43}, { 8, 8}, { 8, 8}, {8,7}, {8, 9}, {1,1},   266, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_VGA_low = { RF_MAKE_ID('V','G','A','4'), "PC (VGA 40x25 text)",     &pcclass, {40,25}, { 9,16}, { 8,16}, {4,8}, {4, 8}, {2,1},   228, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA25   = { RF_MAKE_ID('V','G','A','8'), "PC (VGA 80x25 text)",     &pcclass, {80,25}, { 9,16}, { 8,16}, {8,8}, {8, 8}, {1,1},   228, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA28   = { RF_MAKE_ID('V','G','A','2'), "PC (VGA 80x28 text)",     &pcclass, {80,28}, { 9,14}, { 8,14}, {8,8}, {8,14}, {1,1},   228, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_VGA50   = { RF_MAKE_ID('V','G','A','5'), "PC (VGA 80x50 text)",     &pcclass, {80,50}, { 9, 8}, { 8, 8}, {8,8}, {8, 8}, {1,1},   228, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_VGA25_G = { RF_MAKE_ID('V','G','g','3'), "PC (VGA 80x30 graphics)", &pcclass, {80,30}, { 8,16}, { 8,16}, {8,8}, {8, 8}, {1,1},   266, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA28_G = { RF_MAKE_ID('V','G','g','4'), "PC (VGA 80x34 graphics)", &pcclass, {80,34}, { 8,14}, { 8,14}, {8,8}, {8,18}, {1,1},   266, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_VGA50_G = { RF_MAKE_ID('V','G','g','6'), "PC (VGA 80x60 graphics)", &pcclass, {80,60}, { 8, 8}, { 8, 8}, {8,8}, {8, 8}, {1,1},   266, RF_MAKE_ID('P','C','8','V') };
