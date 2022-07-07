#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

#define IS_SPECTRUM(sys_id) (sys_id == RF_MAKE_ID('Z','X','8','2'))

uint32_t zx_std_color(uint32_t color, bool is_fg) {
    if (color == RF_COLOR_DEFAULT) {
        return is_fg ? RF_COLOR_BLACK : RF_COLOR_WHITE;
    }
    return RF_MapRGBToStandardColor(color, 216);
}

#define zx_rgb_color(color) RF_MapStandardColorToRGB(color, 0,180, 0,255)

uint32_t zx_map_border_color(RF_Context* ctx, uint32_t color) {
    if (IS_SPECTRUM(ctx->system->sys_id)) {
        color = zx_std_color(color, false);
        color &= ~RF_COLOR_BRIGHT;  // border can't be bright
        return zx_rgb_color(color);
    } else {
        return 0xFFFFFF;  // always white border on ZX80/81
    }
}

void zx_prepare_cell(RF_RenderCommand* cmd) {
    if (IS_SPECTRUM(cmd->ctx->system->sys_id)) {
        cmd->fg = zx_std_color(cmd->fg, true);
        cmd->bg = zx_std_color(cmd->bg, false);
        // resolve the BRIGHT attribute -- can't have different BRIGHTness for FG and BG!
        #define IS_BRIGHT(c) (((c) != RF_COLOR_BLACK) && (((c) & RF_COLOR_BRIGHT) != 0))
        bool fg_bright = IS_BRIGHT(cmd->fg);
        bool bg_bright = IS_BRIGHT(cmd->bg);
        if (fg_bright != bg_bright) {
            // in case of disagreement, let the foreground win
            if (fg_bright) { cmd->bg |=  RF_COLOR_BRIGHT; }
            else           { cmd->bg &= ~RF_COLOR_BRIGHT; }
        }
        cmd->fg = zx_rgb_color(cmd->fg);
        cmd->bg = zx_rgb_color(cmd->bg);
    } else {
        // ZX80/ZX81 is strictly black-on-white
        cmd->fg = 0;
        cmd->bg = 0xFFFFFF;
    }
    cmd->reverse_cursor = cmd->is_cursor   && !(cmd->blink_phase & 1);
    cmd->reverse_blink  = cmd->cell->blink && !(cmd->blink_phase & 1);
}

static const RF_SysClass zxclass = {
    zx_map_border_color,
    zx_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char zx81default[] = "`x00`Y01K`x00";
static const char zx82default[] = "`c28`Y01`u00A9 1982 Sinclair Research Ltd`x00`Y00";

//                                  sys_id,                      name,                   class,    scrn,         scrsz,    cellsz,  fontsz,  b_ul,    b_lr,  aspect, blink, monitor,         default_font_id
const RF_System RF_Sys_ZX8x     = { RF_MAKE_ID('Z','X','8','1'), "Sinclair ZX80 / ZX81", &zxclass, zx81default, {32,24}, { 8, 8}, { 8, 8}, {48,44}, {48,52}, {1,1},     0, RF_MONITOR_WHITE, RF_MAKE_ID('Z','X','8','1') };
const RF_System RF_Sys_Spectrum = { RF_MAKE_ID('Z','X','8','2'), "Sinclair ZX Spectrum", &zxclass, zx82default, {32,24}, { 8, 8}, { 8, 8}, {48,44}, {48,52}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('Z','X','8','2') };
