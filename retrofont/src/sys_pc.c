#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

#define SYSTEM_IS_MDA(sys_id)  ((sys_id) == RF_MAKE_ID('P','M','D','A'))

#define pc_std_color(color) RF_MapRGBToStandardColor(color, 200)

uint32_t pc_rgb_color(uint32_t color) {
    if (color == RF_COLOR_YELLOW) {
        return 0xAA5500;
    } else {
        return RF_MapStandardColorToRGB(color, 0,170, 85,255);
    }
}

uint32_t pc_map_border_color(RF_Context* ctx, uint32_t color) {
    if (SYSTEM_IS_MDA(ctx->system->sys_id) || (color == RF_COLOR_DEFAULT)) {
        return 0;
    } else {
        return pc_rgb_color(pc_std_color(color));
    }
}

void pc_render_cell(RF_RenderCommand* cmd) {
    bool is_gfx = (RF_EXTRACT_ID(cmd->ctx->system->sys_id, 3) == 'g');
    bool is_mda = SYSTEM_IS_MDA(cmd->ctx->system->sys_id);

    // resolve to standard color
    cmd->fg = pc_std_color((cmd->fg == RF_COLOR_DEFAULT) ? RF_COLOR_WHITE : cmd->fg);
    cmd->bg = pc_std_color((cmd->bg == RF_COLOR_DEFAULT) ? RF_COLOR_BLACK : cmd->bg);

    // map MDA color: any FG color becomes white, only white BG is white
    if (is_mda) {
        if ((cmd->fg & RF_COLOR_WHITE) != RF_COLOR_BLACK)
            { cmd->fg = RF_COLOR_WHITE | (cmd->fg & RF_COLOR_BRIGHT); }
        if ((cmd->bg & RF_COLOR_WHITE) != RF_COLOR_WHITE)
            { cmd->bg = RF_COLOR_BLACK | (cmd->bg & RF_COLOR_BRIGHT); }
    }

    // text mode blink handling
    if (!is_gfx && (RF_EXTRACT_ID(cmd->ctx->system->sys_id, 3) != 't')) {
        cmd->bg &= ~RF_COLOR_BRIGHT;  // no bright background
        if (cmd->cell->blink && !cmd->blink_phase) {
            cmd->invisible = true;
        }
    }

    // map color back to RGB
    cmd->fg = pc_rgb_color(cmd->fg);
    cmd->bg = pc_rgb_color(cmd->bg);

    // set cursor
    if (cmd->is_cursor && !cmd->blink_phase && !is_gfx) {
        cmd->line_end = cmd->ctx->cell_size.y;
        if (cmd->line_end > 8) { --cmd->line_end; }
        cmd->line_start = cmd->ctx->insert ? (cmd->ctx->cell_size.y >> 1) : (cmd->line_end - 2);
    }

    // render the main cell
    cmd->reverse_cursor = is_gfx && cmd->is_cursor;
    cmd->underline = is_mda && !!cmd->cell->underline;
    RF_RenderCell(cmd);

    // replicate the 9th column if required
    // - on actual VGA, this is done for characters 0xC0 to 0xDF;
    // on codepage 437 (which is the only one we're interested about there),
    // this maps to the Unicode block "Box Drawing" and a few select others;
    // in fact, a few additional characters map there (0xB3 to 0xBF,
    // to be specific), but none of those extends to the right (of course,
    // because otherwise it would've been broken on real MDA/EGA/VGA too),
    // so it's fine to treat the whole block as eligible
    if ((cmd->ctx->cell_size.x == 9) && (
        ((cmd->codepoint & (~0x7F)) == 0x2500)
    ||  ((cmd->codepoint & (~0x0F)) == 0x2580)
    ||  ( cmd->codepoint            == 0x2590)
    )) {
        uint8_t *p = &cmd->pixel[7 * 3];
        for (uint16_t i = cmd->ctx->cell_size.y;  i;  --i) {
            p[3] = p[0];
            p[4] = p[1];
            p[5] = p[2];
            p += cmd->ctx->stride;
        }
    }
}

static const RF_SysClass pcclass = {
    pc_map_border_color,
    NULL,  // prepare_cell = default (unused)
    pc_render_cell,
    NULL,  // check_font = default
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

//                                 sys_id,                      name,                                class,    scrn,         scrsz,    cellsz,  fontsz,  b_ul,  b_lr, aspect, blink, monitor,         default_font_id
const RF_System RF_Sys_MDA     = { RF_MAKE_ID('P','M','D','A'), "PC (MDA/Hercules)",                 &pcclass, default_pc,  {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   228, RF_MONITOR_GREEN, RF_MAKE_ID('P','C','4','M') };
const RF_System RF_Sys_CGA40   = { RF_MAKE_ID('P','C','4','0'), "PC (CGA 40x25 text)",               &pcclass, default_pc,  {40,25}, { 8, 8}, { 8, 8}, {4,4}, {4, 4}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','C') };
const RF_System RF_Sys_CGA80   = { RF_MAKE_ID('P','C','8','0'), "PC (CGA 80x25 text)",               &pcclass, default_pc,  {80,25}, { 8, 8}, { 8, 8}, {8,4}, {8, 4}, {1,2},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','C') };
const RF_System RF_Sys_EGA25_B = { RF_MAKE_ID('P','E','2','b'), "PC (EGA 80x25 text, blinking)",     &pcclass, default_pc,  {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_EGA25   = { RF_MAKE_ID('P','E','2','t'), "PC (EGA 80x25 text, non-blinking)", &pcclass, default_pc,  {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_EGA43_B = { RF_MAKE_ID('P','E','4','b'), "PC (EGA 80x43 text, blinking)",     &pcclass, default_pc,  {80,43}, { 9, 8}, { 8, 8}, {8,7}, {8, 9}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_EGA43   = { RF_MAKE_ID('P','E','4','t'), "PC (EGA 80x43 text, non-blinking)", &pcclass, default_pc,  {80,43}, { 9, 8}, { 8, 8}, {8,7}, {8, 9}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_EGA25_G = { RF_MAKE_ID('P','E','2','g'), "PC (EGA 80x25 graphics)",           &pcclass, default_pc,  {80,25}, { 8,14}, { 8,14}, {8,7}, {8, 7}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_EGA43_G = { RF_MAKE_ID('P','E','4','g'), "PC (EGA 80x43 graphics)",           &pcclass, default_pc,  {80,43}, { 8, 8}, { 8, 8}, {8,7}, {8, 9}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_VGA_low = { RF_MAKE_ID('P','V','0','b'), "PC (VGA 40x25 text, blinking)",     &pcclass, default_pc,  {40,25}, { 9,16}, { 8,16}, {4,8}, {4, 8}, {2,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA_loB = { RF_MAKE_ID('P','V','0','t'), "PC (VGA 40x25 text, non-blinking)", &pcclass, default_pc,  {40,25}, { 9,16}, { 8,16}, {4,8}, {4, 8}, {2,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA25_B = { RF_MAKE_ID('P','V','2','b'), "PC (VGA 80x25 text, blinking)",     &pcclass, default_vga, {80,25}, { 9,16}, { 8,16}, {8,8}, {8, 8}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA25   = { RF_MAKE_ID('P','V','2','t'), "PC (VGA 80x25 text, non-blinking)", &pcclass, default_vga, {80,25}, { 9,16}, { 8,16}, {8,8}, {8, 8}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA28_B = { RF_MAKE_ID('P','V','3','b'), "PC (VGA 80x28 text, blinking)",     &pcclass, default_vga, {80,28}, { 9,14}, { 8,14}, {8,8}, {8,14}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_VGA28   = { RF_MAKE_ID('P','V','3','t'), "PC (VGA 80x28 text, non-blinking)", &pcclass, default_vga, {80,28}, { 9,14}, { 8,14}, {8,8}, {8,14}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_VGA50_B = { RF_MAKE_ID('P','V','5','b'), "PC (VGA 80x50 text, blinking)",     &pcclass, default_vga, {80,50}, { 9, 8}, { 8, 8}, {8,8}, {8, 8}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_VGA50   = { RF_MAKE_ID('P','V','5','t'), "PC (VGA 80x50 text, non-blinking)", &pcclass, default_vga, {80,50}, { 9, 8}, { 8, 8}, {8,8}, {8, 8}, {1,1},   228, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
const RF_System RF_Sys_VGA25_G = { RF_MAKE_ID('P','V','3','g'), "PC (VGA 80x30 graphics)",           &pcclass, default_vga, {80,30}, { 8,16}, { 8,16}, {8,8}, {8, 8}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','6','V') };
const RF_System RF_Sys_VGA28_G = { RF_MAKE_ID('P','V','4','g'), "PC (VGA 80x34 graphics)",           &pcclass, default_vga, {80,34}, { 8,14}, { 8,14}, {8,8}, {8,18}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','4','V') };
const RF_System RF_Sys_VGA50_G = { RF_MAKE_ID('P','V','6','g'), "PC (VGA 80x60 graphics)",           &pcclass, default_vga, {80,60}, { 8, 8}, { 8, 8}, {8,8}, {8, 8}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('P','C','8','V') };
