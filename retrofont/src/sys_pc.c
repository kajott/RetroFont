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

uint32_t pc_map_border_color(RF_Context* ctx, uint32_t color) {
    if (IS_MDA(ctx->system->sys_id) || (color == RF_COLOR_DEFAULT)) {
        return 0;
    } else {
        return pc_rgb_color(pc_std_color(color));
    }
}

void pc_render_cell(const RF_RenderCommand* cmd) {
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
    pc_render_cell,
    NULL,  // check_font
};

static const char default_pc[] =
    "Current date is Tue  1-01-1980\n"
    "Enter new date (mm-dd-yy):\n"
    "Current time is 0:00:08.15\n"
    "Enter new time:\n\n\n"
    "Microsoft(R) MS-DOS(R)  Version 3.30\n"
    "             (C)Copyright Microsoft Corp 1981-1987\n\n\n"
    "A>";
static const char default_vga[] =
    "`u2554`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2557\n"
    "`u2551   AMIBIOS System Configuration (C) 1985-1992, American Megatrends Inc.,  `u2551\n"
    "`u2560`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2564`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2563\n"
    "`u2551 Main Processor     : 80486DX2       `u2502 Base Memory Size   : 640 KB        `u2551\n"
    "`u2551 Numeric Processor  : Present        `u2502 Ext. Memory Size   : 7168 KB       `u2551\n"
    "`u2551 Floppy Drive A:    : 1.44 MB, 3`u00bd\"   `u2502 Hard Disk C: Type  : 47            `u2551\n"
    "`u2551 Floppy Drive B:    : 1.2 MB, 5`u00bc\"    `u2502 Hard Disk D: Type  : None          `u2551\n"
    "`u2551 Display Type       : VGA/PGA/EGA    `u2502 Serial Port(s)     : 3F8,2F8       `u2551\n"
    "`u2551 AMIBIOS Date       : 08/15/92       `u2502 Parallel Port(s)   : 378,278       `u2551\n"
    "`u255A`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2567`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u2550`u255D\n"
    "256 KB CACHE MEMORY\n"
    "66MHz CPU Clock\n"
    "Starting MS-DOS...\n\n\n"
    "Microsoft(R) MS-DOS(R) Version 6.22\n"
    "             (C)Copyright Microsoft Corp 1981-1994.\n\n"
    "C:\\>";

//                                 sys_id,                      name,                      class,    scrn,         scrsz,    cellsz,  fontsz,  b_ul,  b_lr, aspect, blink, monitor,         default_font_id
const RF_System RF_Sys_MDA     = { RF_MAKE_ID('M','D','A','8'), "PC (MDA/Hercules)",       &pcclass, default_pc,  {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   228, RF_MONITOR_GREEN, RF_MAKE_ID('P','C','4','M') };
const RF_System RF_Sys_CGA40   = { RF_MAKE_ID('C','G','A','4'), "PC (CGA 40x25 text)",     &pcclass, default_pc,  {40,25}, { 8, 8}, { 8, 8}, {4,4}, {4, 4}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','C') };
const RF_System RF_Sys_CGA80   = { RF_MAKE_ID('C','G','A','8'), "PC (CGA 80x25 text)",     &pcclass, default_pc,  {80,25}, { 8, 8}, { 8, 8}, {8,4}, {8, 4}, {1,2},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','C') };
const RF_System RF_Sys_EGA25   = { RF_MAKE_ID('E','G','A','2'), "PC (EGA 80x25 text)",     &pcclass, default_pc,  {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_EGA43   = { RF_MAKE_ID('E','G','A','4'), "PC (EGA 80x43 text)",     &pcclass, default_pc,  {80,43}, { 9, 8}, { 8, 8}, {8,7}, {8, 9}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_EGA25_G = { RF_MAKE_ID('E','G','g','2'), "PC (EGA 80x25 graphics)", &pcclass, default_pc,  {80,25}, { 8,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_EGA43_G = { RF_MAKE_ID('E','G','g','4'), "PC (EGA 80x43 graphics)", &pcclass, default_pc,  {80,43}, { 8, 8}, { 8, 8}, {8,7}, {8, 9}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_VGA_low = { RF_MAKE_ID('V','G','A','4'), "PC (VGA 40x25 text)",     &pcclass, default_pc,  {40,25}, { 9,16}, { 8,16}, {4,8}, {4, 8}, {2,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA25   = { RF_MAKE_ID('V','G','A','8'), "PC (VGA 80x25 text)",     &pcclass, default_vga, {80,25}, { 9,16}, { 8,16}, {8,8}, {8, 8}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA28   = { RF_MAKE_ID('V','G','A','2'), "PC (VGA 80x28 text)",     &pcclass, default_vga, {80,28}, { 9,14}, { 8,14}, {8,8}, {8,14}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_VGA50   = { RF_MAKE_ID('V','G','A','5'), "PC (VGA 80x50 text)",     &pcclass, default_vga, {80,50}, { 9, 8}, { 8, 8}, {8,8}, {8, 8}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_VGA25_G = { RF_MAKE_ID('V','G','g','3'), "PC (VGA 80x30 graphics)", &pcclass, default_vga, {80,30}, { 8,16}, { 8,16}, {8,8}, {8, 8}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA28_G = { RF_MAKE_ID('V','G','g','4'), "PC (VGA 80x34 graphics)", &pcclass, default_vga, {80,34}, { 8,14}, { 8,14}, {8,8}, {8,18}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_VGA50_G = { RF_MAKE_ID('V','G','g','6'), "PC (VGA 80x60 graphics)", &pcclass, default_vga, {80,60}, { 8, 8}, { 8, 8}, {8,8}, {8, 8}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
