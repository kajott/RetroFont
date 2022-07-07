#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "retrofont.h"

// CPC ROM default palette
const uint32_t cpc_palette[] = {
    0x000080,  //  0: dark blue
    0xFFFF00,  //  1: yellow
    0x00FFFF,  //  2: cyan
    0xFF0000,  //  3: red
    0xFFFFFF,  //  4: white
    0x000000,  //  5: black
    0x0000FF,  //  6: blue
    0xFF00FF,  //  7: magenta
    0x008080,  //  8: dark cyan
    0x808000,  //  9: dark yellow
    0x8080FF,  // 10: pastel blue
    0xFF8080,  // 11: pastel red
    0x00FF00,  // 12: green
    0x80FF80,  // 13: pastel green
    // 14 and 15 are blinking combinations (and we generally don't use 15)
    0x000080, 0x0080FF
};

const uint8_t cpc_default_color_maps[2][16] = {
    //                          0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
    //                          K     B     G     C     R     M     Y    LG    DG    LB    LG    LC    LR    LM    LY     W
    /* mode 0           */ {    5,    0,    8,    8,    9,    9,    9,    4,    5,    6,   12,    2,    3,    7,    1,    4 },
    /* mode 1 (fg only) */ {    0,    2,    2,    2,    3,    3,    1,    1,    1,    2,    2,    2,    3,    3,    1,    1 },
};

uint32_t cpc_map_color(RF_Context* ctx, uint32_t color, bool is_fg) {
    uint8_t mode = RF_EXTRACT_ID(ctx->system->sys_id, 3) - '0';
    if ((mode == 2) || ((mode == 1) && !is_fg)) {
        // mode 2 shall act like a monochrome mode, everything else doesn't make sense;
        // same goes for the background of mode 1
        color = RF_COLOR_DEFAULT;
    }
    if (color == RF_COLOR_DEFAULT) {
        return is_fg ? 1 : 0;
    } else if (RF_IS_RGB_COLOR(color)) {
        static const uint8_t mode2ncol[3] = { 14, 4, 2};
        return RF_PaletteLookup(ctx, cpc_palette, mode2ncol[mode], color);
    } else if (RF_IS_STD_COLOR(color)) {
        return cpc_default_color_maps[mode][color & 15];
    } else {  // native color?
        return (color & 15);
    }
}

static inline uint8_t cpc_map_rgb_component(uint8_t c) {
    return (c <= 0x55) ? 0x00
         : (c >  0xAA) ? 0x80
         :               0xFF;
}

uint32_t cpc_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx;
    if (color == RF_COLOR_DEFAULT) { return 0x000080; }
    color = RF_MapStandardColorToRGB(color, 0,128, 0,255);
    return RF_COLOR_RGB(cpc_map_rgb_component(RF_COLOR_R(color)),
                        cpc_map_rgb_component(RF_COLOR_G(color)),
                        cpc_map_rgb_component(RF_COLOR_B(color)));
}

void cpc_prepare_cell(RF_RenderCommand* cmd) {
    if (cmd->cell->blink) {
        // there are just two color combinations with default blinking in
        // the ROM palette, and one of them is super-weird (bright red/blue),
        // so let's force everything that blinks to the default blue/yellow
        cmd->fg = cmd->blink_phase & 1;
        cmd->bg = cmd->fg ^ 1;
    } else {
        cmd->fg = cpc_map_color(cmd->ctx, cmd->fg, true);
        cmd->bg = cpc_map_color(cmd->ctx, cmd->bg, false);
    }
    cmd->fg = cpc_palette[cmd->fg];
    cmd->bg = cpc_palette[cmd->bg];
    cmd->reverse_cursor = cmd->is_cursor && !(cmd->blink_phase & 1);
}

static const RF_SysClass cpcclass = {
    cpc_map_border_color,
    cpc_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char cpcdefault[] =
    "\n Amstrad 64K Microcomputer  (v1)\n"
    "\n `u00A91984 Amstrad Consumer Electronics plc"
    "\n           and Locomotive Software Ltd.\n"
    "\n BASIC 1.0\n"
    "\nReady\n";
static const char cpcmini[] = "Ready\n";

//                                     sys_id,                     name,                                  class,     scrn,       scrsz,  cellsz,  fontsz,   b_ul,    b_lr,  aspect, blink, monitor,          default_font_id
const RF_System RF_Sys_CPC_Mode0 = { RF_MAKE_ID('C','P','C','0'), "Amstrad CPC (Mode 0: 20x25 16-color)", &cpcclass, cpcmini,    {20,24}, {8,8},  { 8, 8}, {24,48}, {24,40}, {2,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('C','P','C','8') };
const RF_System RF_Sys_CPC_Mode1 = { RF_MAKE_ID('C','P','C','1'), "Amstrad CPC (Mode 1: 40x25 4-color)",  &cpcclass, cpcdefault, {40,24}, {8,8},  { 8, 8}, {48,48}, {48,40}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('C','P','C','8') };
const RF_System RF_Sys_CPC_Mode2 = { RF_MAKE_ID('C','P','C','2'), "Amstrad CPC (Mode 2: 80x25 2-color)",  &cpcclass, cpcdefault, {80,24}, {8,8},  { 8, 8}, {96,48}, {96,40}, {1,2},   320, RF_MONITOR_COLOR, RF_MAKE_ID('C','P','C','8') };
