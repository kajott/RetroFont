#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

uint32_t decvt_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx, (void)color;
    return 0;
}

void decvt_prepare_cell(RF_RenderCommand* cmd) {
    if (cmd->fg == RF_COLOR_DEFAULT) { cmd->fg = RF_COLOR_WHITE; }
    if (cmd->bg == RF_COLOR_DEFAULT) { cmd->bg = RF_COLOR_BLACK; }

    cmd->fg = RF_MapRGBToStandardColor(cmd->fg, 200);
    cmd->bg = RF_MapRGBToStandardColor(cmd->bg, 200);

    if ((cmd->fg & RF_COLOR_WHITE) != RF_COLOR_BLACK)
        { cmd->fg = RF_COLOR_WHITE | (cmd->fg & RF_COLOR_BRIGHT); }
    if ((cmd->bg & RF_COLOR_WHITE) != RF_COLOR_WHITE)
        { cmd->bg = RF_COLOR_BLACK | (cmd->bg & RF_COLOR_BRIGHT); }

    if (cmd->cell->blink) {
        if ( cmd->cell->bold && !(cmd->blink_phase & 4)) { cmd->fg |= RF_COLOR_BRIGHT; }
        if (!cmd->cell->bold &&  (cmd->blink_phase & 4)) { cmd->fg = cmd->bg; }
    } else if (cmd->cell->bold) {
        cmd->fg |= RF_COLOR_BRIGHT;
    }

    cmd->fg = RF_MapStandardColorToRGB(cmd->fg, 0,192, 0,255);
    cmd->bg = RF_MapStandardColorToRGB(cmd->bg, 0,192, 0,255);

    cmd->reverse_cursor = cmd->is_cursor && !!(cmd->blink_phase & 3);
    cmd->underline = !!cmd->cell->underline;
}


static const RF_SysClass decvtclass = {
    decvt_map_border_color,
    decvt_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char vt100default[] =
    "`+f`+bSET-UP B`-f`-b\n\n"
    "`+uTO EXIT PRESS \"SET-UP\"`-u"
    "`x00`Y011 `+r1101`-r  "
            "2 `+r0101`-r  "
            "3 `+r0000`-r  "
            "4 `+r0010`-r"
    "`x49T SPEED  9600"
     "   R SPEED  9600"
    "`x00`Y02";

static const char vt220default[] =
    "`C03`c20`u250c`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2510"
      "\n`c20`u2502     VT220 OK     `u2502"
      "\n`c20`u2514`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2500`u2518"
    "`Y03`c48Firmware and Set-Up Screens Copyright `u00a9 1983,85"
    "`Y02`c28Digital Equipment Corporation"
    "`x00`Y00";

//                                   sys_id,                       name,        class,       scrn,         scrsz,   cellsz,  fontsz,   b_ul,  b_lr,  aspect, blink, monitor,           default_font_id
const RF_System RF_Sys_DEC_VT100 = { RF_MAKE_ID('V','1','0','0'), "DEC VT100",  &decvtclass, vt100default, {80,24}, {10,10}, {10,10},  {5,2}, {3,2}, {1,2},    267, RF_MONITOR_BLUE,  RF_MAKE_ID('V','1','0','0') };
const RF_System RF_Sys_DEC_VT220 = { RF_MAKE_ID('V','2','2','0'), "DEC VT220",  &decvtclass, vt220default, {80,24}, {10,10}, {10,10},  {5,2}, {3,2}, {1,2},    267, RF_MONITOR_AMBER, RF_MAKE_ID('V','2','2','0') };
