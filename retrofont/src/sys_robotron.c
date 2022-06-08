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

uint32_t kc85_pre_map_color(uint32_t color, uint32_t default1, uint32_t default2) {
    if (color == RF_COLOR_DEFAULT) { color = default1; }
    if (color == RF_COLOR_DEFAULT) { color = default2; }
    return RF_MapRGBToStandardColor(color, 200);
}

uint32_t kc85_map_border_color(uint32_t sys_id, uint32_t color) {
    (void)sys_id, (void)color;
    return 0;  // border is always black
}

void kc85_render_cell(const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    uint16_t line = 999;
    if (!cmd || !cmd->cell) { return; }
    fg = kc85_pre_map_color(cmd->cell->fg, cmd->ctx->default_fg, RF_COLOR_WHITE);
    bg = kc85_pre_map_color(cmd->cell->bg, cmd->ctx->default_bg, RF_COLOR_BLUE);
    if (cmd->ctx->insert && cmd->is_cursor) { line = 6; }
    RF_RenderCell(cmd,
        kc85_fg_color_map[fg & 15],
        RF_MapStandardColorToRGB(bg & (~RF_COLOR_BRIGHT), 0,160, 0,160),
        0,0,
        line, line+1,
        !cmd->ctx->insert && cmd->is_cursor && !cmd->blink_phase,
        cmd->cell->blink && cmd->blink_phase,
        false, false);
}

const RF_SysClass kc85class = {
    kc85_map_border_color,
    kc85_render_cell,
    NULL,  // check_font
};


////////////////////////////////////////////////////////////////////////////////

uint32_t kc87_map_color(uint32_t color, uint32_t default1, uint32_t default2) {
    if (color == RF_COLOR_DEFAULT) { color = default1; }
    if (color == RF_COLOR_DEFAULT) { color = default2; }
    color = RF_MapRGBToStandardColor(color, 200);
    return RF_MapStandardColorToRGB(color, 0,255, 0,255);
}

uint32_t kc87_map_border_color(uint32_t sys_id, uint32_t color) {
    (void)sys_id;
    return kc87_map_color(color, RF_COLOR_BLACK, 0);
}

void kc87_render_cell(const RF_RenderCommand* cmd) {
    uint32_t fg, bg;
    if (!cmd || !cmd->cell) { return; }
    fg = kc87_map_color(cmd->cell->fg, cmd->ctx->default_fg, RF_COLOR_WHITE);
    bg = kc87_map_color(cmd->cell->bg, cmd->ctx->default_bg, RF_COLOR_BLACK);
    RF_RenderCell(cmd, fg, bg, 0,0, 0,0, cmd->is_cursor ? !cmd->blink_phase : (cmd->cell->blink && cmd->blink_phase), false, false, false);
}

const RF_SysClass kc87class = {
    kc87_map_border_color,
    kc87_render_cell,
    NULL,  // check_font
};

////////////////////////////////////////////////////////////////////////////////

//                               sys_id,                       name,                            class,      scrsz,  cellsz, fontsz,  b_ul,    b_lr,   aspect, blink, default_font_id
const RF_System RF_Sys_KC85  = { RF_MAKE_ID('K','C','8','5'), "robotron HC900, KC85/2, /3, /4", &kc85class, {40,32}, {8,8},  {8,8}, {24,16}, {24,16}, {1,1},    320, RF_MAKE_ID('K','C','4','1') };
const RF_System RF_Sys_KC87  = { RF_MAKE_ID('K','C','8','7'), "robotron Z 9001, KC85/1, KC87",  &kc87class, {40,24}, {8,8},  {8,8}, {32,44}, {32,44}, {1,1},    320, RF_MAKE_ID('K','C','8','7') };
