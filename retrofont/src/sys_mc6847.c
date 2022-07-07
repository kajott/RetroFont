#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "retrofont.h"

static const uint32_t mc6847_palette[11] = {
    // source: http://hcvgm.org/VDG_Colours.html
    0x28E028,  //  0: green
    0xF0F070,  //  1: yellow
    0x2020D8,  //  2: blue
    0xA82020,  //  3: red
    0xF0F0F0,  //  4: buff
    0x28A8A8,  //  5: cyan
    0xD361FA,  //  6: magenta
    0xF08828,  //  7: orange
    0x101010,  //  8: black
    0x106010,  //  9: dark green
    0x803010,  // 10: dark orange
};

uint32_t mc6847_black_border(RF_Context* ctx, uint32_t color) {
    (void)ctx, (void)color;
    return mc6847_palette[8];
}

uint32_t mc6847_map_color(RF_Context* ctx, uint32_t color, uint8_t default_index) {
    return mc6847_palette[
        (color == RF_COLOR_DEFAULT) ? default_index :
        RF_PaletteLookup(ctx, mc6847_palette, 8, RF_MapStandardColorToRGB(color, 0,160, 0,255))
    ];
}

////////////////////////////////////////////////////////////////////////////////

void atom_prepare_cell(RF_RenderCommand* cmd) {
    uint32_t color = RF_MapStandardColorToRGB((cmd->ctx->default_fg == RF_COLOR_DEFAULT) ? RF_COLOR_GREEN : cmd->ctx->default_fg, 0,255, 0,255);
    bool is_orange = (RF_COLOR_R(color) > RF_COLOR_G(color)) || ((RF_COLOR_B(color) < RF_COLOR_G(color)) && (RF_COLOR_B(color) < RF_COLOR_R(color)));
    cmd->fg = mc6847_palette[is_orange ?  7 : 0];
    cmd->bg = mc6847_palette[is_orange ? 10 : 9];
    cmd->reverse_cursor = cmd->is_cursor;
}

static const RF_SysClass atomclass = {
    mc6847_black_border,
    atom_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char defaultatom[] = "ACORN ATOM\n\n>";

////////////////////////////////////////////////////////////////////////////////

void coco_prepare_cell(RF_RenderCommand* cmd) {
    if (cmd->is_cursor && (cmd->ctx->system->sys_id == RF_MAKE_ID('C','o','C','o'))) {
        // CoCo colorful cursor
        cmd->bg = cmd->fg = mc6847_palette[cmd->blink_phase & 7];
    } else if ((cmd->codepoint & (~0x1F)) == 0x2580) {
        // block graphics character -> allow the basic 8 colors
        cmd->fg = mc6847_map_color(cmd->ctx, cmd->fg, 9);
        cmd->bg = mc6847_map_color(cmd->ctx, cmd->bg, 0);
        // force one of the colors to black
        if ((RF_COLOR_R(cmd->fg) + RF_COLOR_G(cmd->fg) + RF_COLOR_B(cmd->fg))
        >=  (RF_COLOR_R(cmd->bg) + RF_COLOR_G(cmd->bg) + RF_COLOR_B(cmd->bg)))
             { cmd->bg = mc6847_palette[8]; }
        else { cmd->fg = mc6847_palette[8]; }
    } else {
        // normal ASCII character -> hardwire to black-on-green
        cmd->fg = mc6847_palette[9];
        cmd->bg = mc6847_palette[0];
        cmd->reverse_cursor = cmd->is_cursor && !(cmd->blink_phase & 1);
    }
}

static const RF_SysClass cococlass = {
    mc6847_black_border,
    coco_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const char defaultcoco[] =
    "EXTENDED COLOR BASIC 1.1\n"
    "COPYRIGHT (C) 1982 BY TANDY\n"
    "UNDER LICENSE FROM MICROSOFT\n"
    "\nOK\n";
static const char defaultdragon[] =
    "(C) 1982 DRAGON DATA LTD\n"
    "16K BASIC INTERPRETER 1.0\n"
    "(C) 1982 BY MICROSOFT\n"
    "\nOK\n";

////////////////////////////////////////////////////////////////////////////////

//                                sys_id,                       name,                          class,     scrn,           scrsz,  cellsz, fontsz , b_ul,    b_lr,  aspect, blink, monitor,          default_font_id
const RF_System RF_Sys_CoCo   = { RF_MAKE_ID('C','o','C','o'), "Tandy TRS-80 Color Computer", &cococlass, defaultcoco,   {32,16}, {8,12}, {8,12}, {56,24}, {56,24}, {1,1},   100, RF_MONITOR_COLOR, RF_MAKE_ID('M','T','0','4') };
const RF_System RF_Sys_Dragon = { RF_MAKE_ID('D','R','3','2'), "Dragon 32/64",                &cococlass, defaultdragon, {32,16}, {8,12}, {8,12}, {56,24}, {56,24}, {1,1},   533, RF_MONITOR_COLOR, RF_MAKE_ID('M','T','0','4') };
const RF_System RF_Sys_Atom   = { RF_MAKE_ID('A','T','O','M'), "Acorn Atom",                  &atomclass, defaultatom,   {32,16}, {8,12}, {8,12}, {56,24}, {56,24}, {1,1},     0, RF_MONITOR_COLOR, RF_MAKE_ID('M','T','0','6') };
