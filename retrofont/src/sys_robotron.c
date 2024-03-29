#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

////////////////////////////////////////////////////////////////////////////////

// using floooh's palette here; kc85emu's is slightly different
static const uint32_t kc85_fg_color_map[16] = {
    0x000000, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF,
    0x000000, 0xA000FF, 0x00FFA0, 0x00A0FF, 0xFFA000, 0xFF00A0, 0xA0FF00, 0xFFFFFF,
};

#define kc85_pre_map_color(color, default) \
    RF_MapRGBToStandardColor((color == RF_COLOR_DEFAULT) ? (default) : (color), 200)

uint32_t kc85_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx, (void)color;
    return 0;  // border is always black
}

void kc85_prepare_cell(RF_RenderCommand* cmd) {
    bool line_cursor = cmd->is_cursor && (cmd->codepoint == 32);
    cmd->fg = kc85_pre_map_color(cmd->fg, RF_COLOR_WHITE);
    cmd->bg = kc85_pre_map_color(cmd->bg, RF_COLOR_BLUE);
    cmd->fg = kc85_fg_color_map[cmd->fg & 15];
    cmd->bg = RF_MapStandardColorToRGB(cmd->bg, 0,160, 0,160);
    if (line_cursor) {
        cmd->line_start = 6;
        cmd->line_end = 7;
    }
    cmd->reverse_cursor = cmd->is_cursor && !line_cursor && !(cmd->blink_phase & 1);
    if (cmd->cell->blink && (cmd->blink_phase & 1)) { cmd->invisible = true; }
}

static const RF_SysClass kc85class = {
    kc85_map_border_color,
    kc85_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char kc85default[] =
    "\n* KC-CAOS 4.2 *"
    "\n%BASIC"
    "\n%REBASIC"
    "\n%SWITCH"
    "\n%JUMP"
    "\n%MENU"
    "\n%SAVE"
    "\n%VERIFY"
    "\n%LOAD"
    "\n%COLOR"
    "\n%DISPLAY"
    "\n%MODIFY"
    "\n%WINDOW"
    "\n%KEY"
    "\n%KEYLIST"
    "\n%MODUL"
    "\n%SYSTEM"
    "\n%V24OUT"
    "\n%V24DUP"
    "\n%";

////////////////////////////////////////////////////////////////////////////////

uint32_t kc87_map_color(uint32_t color, uint32_t default1) {
    if (color == RF_COLOR_DEFAULT) { color = default1; }
    color = RF_MapRGBToStandardColor(color, 200);
    return RF_MapStandardColorToRGB(color, 0,255, 0,255);
}

uint32_t kc87_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx;
    return kc87_map_color(color, RF_COLOR_BLACK);
}

void kc87_prepare_cell(RF_RenderCommand* cmd) {
    cmd->fg = kc87_map_color(cmd->fg, RF_COLOR_WHITE);
    cmd->bg = kc87_map_color(cmd->bg, RF_COLOR_BLACK);
    cmd->reverse_cursor = cmd->is_cursor && !(cmd->blink_phase & 1);
    cmd->reverse_blink = !cmd->is_cursor && cmd->cell->blink && (cmd->blink_phase & 1);
}

static const RF_SysClass kc87class = {
    kc87_map_border_color,
    kc87_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char kc87default[] = "`f4robotron  Z 9001\n`0\n`F2OS\n>";

////////////////////////////////////////////////////////////////////////////////

uint32_t z1013_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx, (void)color;
    return 0;
}

void z1013_prepare_cell(RF_RenderCommand* cmd) {
    cmd->fg = 0xFFFFFF;
    cmd->bg = cmd->is_cursor ? 0xFFFFFF : 0x000000;
}

static const RF_SysClass z1013class = {
    z1013_map_border_color,
    z1013_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char default1013[] = "robotron Z 1013/A.2\n # ";

////////////////////////////////////////////////////////////////////////////////

//                               sys_id,                       name,                            class,       scrn,         scrsz,  cellsz, fontsz,  b_ul,    b_lr,   aspect, blink, monitor,         default_font_id
const RF_System RF_Sys_KC85  = { RF_MAKE_ID('K','C','8','5'), "robotron HC900, KC85/2, /3, /4", &kc85class,  kc85default, {40,32}, {8,8},  {8,8}, {24,16}, {24,16}, {1,1},    320, RF_MONITOR_COLOR, RF_MAKE_ID('K','C','4','1') };
const RF_System RF_Sys_KC87  = { RF_MAKE_ID('K','C','8','7'), "robotron Z 9001, KC85/1, KC87",  &kc87class,  kc87default, {40,24}, {8,8},  {8,8}, {32,44}, {32,44}, {1,1},    320, RF_MONITOR_COLOR, RF_MAKE_ID('K','C','8','7') };
const RF_System RF_Sys_Z1013 = { RF_MAKE_ID('1','0','1','3'), "robotron Z 1013",                &z1013class, default1013, {32,32}, {8,8},  {8,8}, {80,16}, {80,16}, {1,1},      0, RF_MONITOR_WHITE, RF_MAKE_ID('1','0','1','3') };
