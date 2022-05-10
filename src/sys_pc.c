#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

#define IS_MDA(sys_id)  ((sys_id) == RF_MAKE_ID("MDA9"))

#define pc_std_color(color) RF_MapRGBToStandardColor(color, 200)
#define pc_rgb_color(color) RF_MapStandardColorToRGB(color, 0,170, 85,255)

uint32_t pc_map_border_color (uint32_t sys_id, uint32_t color) {
    if (IS_MDA(sys_id) || (color == RF_COLOR_DEFAULT)) {
        return 0;  // MDA always has a black border
    } else {
        return pc_rgb_color(pc_std_color(color));
    }
}

void pc_render_cell (const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    if (!cmd || !cmd->cell) { return; }

    // resolve standard color
    fg = cmd->cell->fg;
    bg = cmd->cell->bg;
    if (fg == RF_COLOR_DEFAULT) { fg = cmd->default_fg; }
    if (bg == RF_COLOR_DEFAULT) { bg = cmd->default_bg; }
    if (fg == RF_COLOR_DEFAULT) { fg = RF_COLOR_WHITE; }
    if (bg == RF_COLOR_DEFAULT) { bg = RF_COLOR_BLACK; }
    fg = pc_std_color(fg);
    bg = pc_std_color(bg);

    // map MDA color: any FG color becomes white, only white BG is white
    if (IS_MDA(cmd->sys_id)) {
        if ((fg & RF_COLOR_WHITE) != RF_COLOR_BLACK)
            { fg = RF_COLOR_WHITE | (fg & RF_COLOR_BRIGHT); }
        if ((bg & RF_COLOR_WHITE) != RF_COLOR_WHITE)
            { bg = RF_COLOR_BLACK | (bg & RF_COLOR_BRIGHT); }
    }

    // map color back to RGB
    fg = pc_rgb_color(fg);
    bg = pc_rgb_color(bg);

    RF_RenderCell(cmd, fg, bg, 0,0, false, false);
    // TODO: render 9th column
    // TODO: render cursor
}

RF_SysClass pcclass = {
    pc_map_border_color,
    pc_render_cell
};

//                              sys_id,           name,                      class,    scrsz,    cellsz,  fontsz,  b_ul, b_lr,  double, blink, default_font_id
RF_System RF_Sys_MDA     = { RF_MAKE_ID("MDA9"), "PC (MDA)",                &pcclass, {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, false,    228, RF_MAKE_ID("PC4M") };
RF_System RF_Sys_CGA40   = { RF_MAKE_ID("CGA4"), "PC (CGA 40x25 text)",     &pcclass, {40,25}, { 8, 8}, { 8, 8}, {4,4}, {4, 4}, false,    266, RF_MAKE_ID("PC8C") };
RF_System RF_Sys_CGA80   = { RF_MAKE_ID("CGA8"), "PC (CGA 80x25 text)",     &pcclass, {80,25}, { 8, 8}, { 8, 8}, {8,4}, {8, 4}, false,    266, RF_MAKE_ID("PC8C") };
RF_System RF_Sys_EGA25   = { RF_MAKE_ID("EG49"), "PC (EGA 80x25 text)",     &pcclass, {80,25}, { 9,14}, { 8,14}, {8,7}, {8, 7}, false,    266, RF_MAKE_ID("PC4V") };
RF_System RF_Sys_EGA43   = { RF_MAKE_ID("EG89"), "PC (EGA 80x43 text)",     &pcclass, {80,43}, { 9, 8}, { 8, 8}, {8,7}, {8, 9}, false,    266, RF_MAKE_ID("PC8V") };
RF_System RF_Sys_EGA25_G = { RF_MAKE_ID("EG48"), "PC (EGA 80x25 graphics)", &pcclass, {80,25}, { 8,14}, { 8,14}, {8,7}, {8, 7}, false,    266, RF_MAKE_ID("PC4V") };
RF_System RF_Sys_EGA43_G = { RF_MAKE_ID("EG88"), "PC (EGA 80x43 graphics)", &pcclass, {80,43}, { 8, 8}, { 8, 8}, {8,7}, {8, 9}, false,    266, RF_MAKE_ID("PC8V") };
RF_System RF_Sys_VGA_low = { RF_MAKE_ID("VG09"), "PC (VGA 40x25 text)",     &pcclass, {40,25}, { 9,16}, { 8,16}, {4,8}, {4, 8}, false,    228, RF_MAKE_ID("PC6V") };
RF_System RF_Sys_VGA25   = { RF_MAKE_ID("VG69"), "PC (VGA 80x25 text)",     &pcclass, {80,25}, { 9,16}, { 8,16}, {8,8}, {8, 8}, false,    228, RF_MAKE_ID("PC6V") };
RF_System RF_Sys_VGA28   = { RF_MAKE_ID("VG49"), "PC (VGA 80x28 text)",     &pcclass, {80,28}, { 9,14}, { 8,14}, {8,8}, {8,14}, false,    228, RF_MAKE_ID("PC4V") };
RF_System RF_Sys_VGA50   = { RF_MAKE_ID("VG89"), "PC (VGA 80x50 text)",     &pcclass, {80,50}, { 9, 8}, { 8, 8}, {8,8}, {8, 8}, false,    228, RF_MAKE_ID("PC8V") };
RF_System RF_Sys_VGA25_G = { RF_MAKE_ID("VG68"), "PC (VGA 80x30 graphics)", &pcclass, {80,30}, { 8,16}, { 8,16}, {8,8}, {8, 8}, false,    266, RF_MAKE_ID("PC6V") };
RF_System RF_Sys_VGA28_G = { RF_MAKE_ID("VG48"), "PC (VGA 80x34 graphics)", &pcclass, {80,34}, { 8,14}, { 8,14}, {8,8}, {8,18}, false,    266, RF_MAKE_ID("PC4V") };
RF_System RF_Sys_VGA50_G = { RF_MAKE_ID("VG88"), "PC (VGA 80x60 graphics)", &pcclass, {80,60}, { 8, 8}, { 8, 8}, {8,8}, {8, 8}, false,    266, RF_MAKE_ID("PC8V") };
