#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

#define CINT 180
#define IS_SPECTRUM(sys_id) (sys_id == RF_MAKE_ID('Z','X','8','2'))

uint32_t zx_std_color(uint32_t color, uint32_t default_color, bool is_fg) {
    if (color == RF_COLOR_DEFAULT) {
        color = default_color;
    }
    if (color == RF_COLOR_DEFAULT) {
        return is_fg ? RF_COLOR_BLACK : RF_COLOR_WHITE;
    }
    return RF_MapRGBToStandardColor(color, 216);
}

#define zx_rgb_color(color) RF_MapStandardColorToRGB(color, 0, CINT, 0, 255)

uint32_t zx_map_border_color (uint32_t sys_id, uint32_t color) {
    if (IS_SPECTRUM(sys_id)) {
        color = zx_std_color(color, RF_COLOR_DEFAULT, false);
        color &= ~RF_COLOR_BRIGHT;  // border can not be bright
        return zx_rgb_color(color);
    } else {
        return 0xFFFFFF;  // always white border on ZX80/81
    }
}

void zx_render_cell (const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    uint8_t rev;
    if (!cmd || !cmd->cell) { return; }

    if (IS_SPECTRUM(cmd->ctx->system->sys_id)) {
        fg = zx_std_color(cmd->cell->fg, cmd->ctx->default_fg, true);
        bg = zx_std_color(cmd->cell->bg, cmd->ctx->default_bg, false);
        // resolve the BRIGHT attribute -- can't have different BRIGHTness for FG and BG!
        #define IS_BRIGHT(c) (((c) != RF_COLOR_BLACK) && (((c) & RF_COLOR_BRIGHT) != 0))
        bool fg_bright = IS_BRIGHT(fg);
        bool bg_bright = IS_BRIGHT(bg);
        if (fg_bright != bg_bright) {
            // in case of disagreement, let the foreground win
            if (fg_bright) { bg |=  RF_COLOR_BRIGHT; }
            else           { bg &= ~RF_COLOR_BRIGHT; }
        }
        fg = zx_rgb_color(fg);
        bg = zx_rgb_color(bg);
    } else {
        fg = 0;
        bg = 0xFFFFFF;
    }

    rev  = (cmd->cell->blink &&  cmd->blink_phase) ? 1 : 0;
    rev ^= (cmd->is_cursor   && !cmd->blink_phase) ? 1 : 0;
    RF_RenderCell(cmd, fg, bg, 0,0, 0,0, (bool)rev, false, false, false);
}

const RF_SysClass zxclass = {
    zx_map_border_color,
    zx_render_cell,
    NULL,  // check_font
};

//                                  sys_id,                      name,                   class,    scrn, scrsz,    cellsz,  fontsz,  b_ul,    b_lr,  aspect, blink, default_font_id
const RF_System RF_Sys_ZX8x     = { RF_MAKE_ID('Z','X','8','1'), "Sinclair ZX80 / ZX81", &zxclass, NULL, {32,24}, { 8, 8}, { 8, 8}, {48,44}, {48,52}, {1,1},     0, RF_MAKE_ID('Z','X','8','1') };
const RF_System RF_Sys_Spectrum = { RF_MAKE_ID('Z','X','8','2'), "Sinclair ZX Spectrum", &zxclass, NULL, {32,24}, { 8, 8}, { 8, 8}, {48,44}, {48,52}, {1,1},   320, RF_MAKE_ID('Z','X','8','2') };
