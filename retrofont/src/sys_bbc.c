#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "retrofont.h"

uint32_t bbc_map_color(uint32_t color, uint32_t colors, bool blink, uint32_t default1) {
    // default color or monochrome mode -> force standard colors
    if ((colors == 2) || (color == RF_COLOR_DEFAULT)) {
        color = default1;
    }
    color = RF_MapRGBToStandardColor(color, 128);
    // reduce colors for 4-color mode
    if (colors == 4) {
        static const uint8_t bbc_4color_map[8] = { 0, 4, 4, 6, 4, 6, 6, 7 };
        color = (color & (~7)) | bbc_4color_map[color & 7];
    }
    // handle blinking in 8-color mode
    if ((colors == 8) && blink) {
        color ^= 7;
    }
    return RF_MapStandardColorToRGB(color, 0,255, 0,255);
}

uint32_t bbc_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx, (void)color;
    return 0;
}

void bbc_render_cell(RF_RenderCommand* cmd) {
    static const uint8_t bbc_color_count[8] = { 2, 4, 8, 2, 2, 4, 2, 0 };
    uint8_t colors = bbc_color_count[RF_EXTRACT_ID(cmd->ctx->system->sys_id, 3) & 7];
    bool cursor = cmd->is_cursor && !cmd->blink_phase;
    bool blink = cmd->cell->blink && cmd->blink_phase;
    cmd->fg = bbc_map_color(cmd->fg, colors, blink, RF_COLOR_WHITE);
    cmd->bg = bbc_map_color(cmd->bg, colors, blink, RF_COLOR_BLACK);

    // alternate blink and cursor handling in mode 7 (teletext)
    if (colors == 0) {
        if (blink) { cmd->invisible = true; }
        if (cursor) {
            cmd->line_start = 18;
            cmd->line_end = 20;
        }
    }

    // draw the main cell
    RF_RenderCell(cmd);
    if (colors == 0) {
        return;  // teletext mode -> we're done here
    }

    // draw the inverted cursor
    if (cursor) {
        uint8_t* p = &cmd->pixel[cmd->ctx->stride * 7];
        for (uint8_t c = 24;  c;  --c) {
            *p ^= 0xFF;
            ++p;
        }
    }

    // fill spacer lines in text mode with black (or white if the cursor is here)
    for (size_t y = 8;  y < cmd->ctx->system->cell_size.y;  ++y) {
        memset(&cmd->pixel[cmd->ctx->stride * y], cursor ? 0xFF : 0, 24);
    }
}

static const RF_SysClass bbcclass = {
    bbc_map_border_color,
    NULL,  // prepare_cell = default
    bbc_render_cell,
    NULL,  // check_font = default
};

static const char defaultbbc[] =
    "\nBBC Computer 32K\n"
    "\nAcorn DFS\n"
    "\nBASIC\n"
    "\n>";

//                                   sys_id,                       name,                                            class,    scrn,         scrsz, cellsz, fontsz,  b_ul,    b_lr,  aspect, blink, monitor,          default_font_id
const RF_System RF_Sys_BBC_Mode0 = { RF_MAKE_ID('B','B','C','0'), "BBC Micro (Mode 0: 80x32 monochrome graphics)", &bbcclass, defaultbbc, {80,32}, {8,8},  {8,8}, {96,16}, {96,16}, {1,2},   320, RF_MONITOR_COLOR, RF_MAKE_ID('B','B','C','M') };
const RF_System RF_Sys_BBC_Mode1 = { RF_MAKE_ID('B','B','C','1'), "BBC Micro (Mode 1: 40x32 4-color graphics)",    &bbcclass, defaultbbc, {40,32}, {8,8},  {8,8}, {48,16}, {48,16}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('B','B','C','M') };
const RF_System RF_Sys_BBC_Mode2 = { RF_MAKE_ID('B','B','C','2'), "BBC Micro (Mode 2: 20x32 8-color graphics)",    &bbcclass, defaultbbc, {20,32}, {8,8},  {8,8}, {24,16}, {24,16}, {2,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('B','B','C','M') };
const RF_System RF_Sys_BBC_Mode3 = { RF_MAKE_ID('B','B','C','3'), "BBC Micro (Mode 3: 80x25 monochrome text)",     &bbcclass, defaultbbc, {80,25}, {8,10}, {8,8}, {96,16}, {96,22}, {1,2},   320, RF_MONITOR_COLOR, RF_MAKE_ID('B','B','C','M') };
const RF_System RF_Sys_BBC_Mode4 = { RF_MAKE_ID('B','B','C','4'), "BBC Micro (Mode 4: 40x32 monochrome graphics)", &bbcclass, defaultbbc, {40,32}, {8,8},  {8,8}, {48,16}, {48,16}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('B','B','C','M') };
const RF_System RF_Sys_BBC_Mode5 = { RF_MAKE_ID('B','B','C','5'), "BBC Micro (Mode 5: 20x32 4-color graphics)",    &bbcclass, defaultbbc, {20,32}, {8,8},  {8,8}, {24,16}, {24,16}, {2,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('B','B','C','M') };
const RF_System RF_Sys_BBC_Mode6 = { RF_MAKE_ID('B','B','C','6'), "BBC Micro (Mode 6: 40x25 monochrome text)",     &bbcclass, defaultbbc, {40,25}, {8,10}, {8,8}, {48,16}, {48,22}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('B','B','C','M') };
const RF_System RF_Sys_BBC_Mode7 = { RF_MAKE_ID('B','B','C','7'), "BBC Micro (Mode 7: 40x25 Teletext)",            &bbcclass, defaultbbc, {40,25},{12,20},{12,20},{72,38}, {72,38}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('5','0','5','0') };
