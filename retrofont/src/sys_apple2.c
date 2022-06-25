#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

uint32_t a2_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx, (void)color;
    return 0;
}

void a2_prepare_cell(RF_RenderCommand* cmd) {
    uint8_t model = RF_EXTRACT_ID(cmd->ctx->system->sys_id, 3);
    uint32_t cursor_char = 0;
    cmd->fg = 0xFFFFFF;
    cmd->bg = 0x000000;
    switch (model) {
        case '1': // Apple I: "@"-cursor, bottom-aligned, no inverse video
            cursor_char = '@';
            cmd->offset.x = cmd->offset.y = 1;
            cmd->reverse_attr = false;
            break;
        case '2': // Apple II: inverse cursor, bottom-aligned, blinking supported
            cmd->offset.x = cmd->offset.y = 1;
            cmd->reverse_cursor = cmd->is_cursor && !cmd->blink_phase;
            cmd->reverse_blink = cmd->cell->blink && cmd->blink_phase && !cmd->reverse_cursor;
            break;
        default:  // Apple IIe: 0x7F special character cursor
            cursor_char = 0x2425;  // 0x7F maps to U+2425 SYMBOL FOR DELETE FORM TWO
            break;
    }
    if (cmd->is_cursor && cursor_char && !cmd->blink_phase) {
        cmd->codepoint = cursor_char;
    }
}

static const RF_SysClass a2class = {
    a2_map_border_color,
    a2_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char defaulta1[]  = "\\\n";
static const char defaulta2[]  = "`c08APPLE ][`Y01`x00>";
static const char defaulta2e[] = "`c08Apple ][`Y01`x00]";

//                                  sys_id,                       name,            class,   scrn,        scrsz,  cellsz, fontsz,  b_ul,    b_lr,   aspect, blink, monitor,          default_font_id
const RF_System RF_Sys_AppleI   = { RF_MAKE_ID('A','P','L','1'), "Apple I",       &a2class, defaulta1,  {40,24}, {7,8},  {6,8},  {72,24}, {70,24}, {1,1},    266, RF_MONITOR_GREEN, RF_MAKE_ID('2','5','1','3') };
const RF_System RF_Sys_AppleII  = { RF_MAKE_ID('A','P','L','2'), "Apple II/II+",  &a2class, defaulta2,  {40,24}, {7,8},  {6,8},  {72,24}, {70,24}, {1,1},    266, RF_MONITOR_GREEN, RF_MAKE_ID('2','5','1','3') };
const RF_System RF_Sys_AppleIIe = { RF_MAKE_ID('A','P','2','e'), "Apple IIe/IIc", &a2class, defaulta2e, {40,24}, {7,8},  {8,8},  {72,24}, {70,24}, {1,1},    266, RF_MONITOR_GREEN, RF_MAKE_ID('A','P','2','e') };
