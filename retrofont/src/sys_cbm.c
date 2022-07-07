#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "retrofont.h"

const uint32_t cbm_palettes[] = {
    //                        ---00---  ---01---  ---02---  ---03---  ---04---  ---05---  ---06---  ---07---  ---08---  ---09---  ---10---  ---11---  ---12---  ---13---  ---14---  ---15---
    /* 0x000: VIC    PAL  */  0x000000, 0xFFFFFF, 0x950000, 0x46D1DA, 0x8900FB, 0x53CE00, 0x0000FF, 0xD7E700, 0xAB4500, 0xFAA222, 0xF7766B, 0x87FFFF, 0xE36CFF, 0x9AFF03, 0x7C6AFF, 0xFFFF17,
    /* 0x010: VIC    NTSC */  0x000000, 0xFFFFFF, 0x850000, 0x2DBEB9, 0x4E15A3, 0x629C0D, 0x000C97, 0xF7C137, 0xAF2900, 0xFF8852, 0xFA696E, 0x6CFDF8, 0xB67DFF, 0xACE657, 0x3D73FD, 0xFFFF76,
    /* 0x020: VIC-II PAL  */  0x000000, 0xFFFFFF, 0x924A40, 0x84C5CC, 0x9351B6, 0x72B14B, 0x483AAA, 0xD5DF7C, 0x99692D, 0x675200, 0xC18178, 0x606060, 0x8A8A8A, 0xB3EC91, 0x867ADE, 0xB3B3B3,
    /* 0x030: VIC-II NTSC */  0x000000, 0xFFFFFF, 0x7E352B, 0x6EB7C1, 0x7F3BA6, 0x5CA035, 0x332799, 0xCBD765, 0x85531C, 0x503C00, 0xB46B61, 0x4A4A4A, 0x757575, 0xA3E77C, 0x7064D6, 0xA3A3A3,
    /* 0x040: TED PAL  b0 */  0x000000, 0x101010, 0x5C0000, 0x00333B, 0x410091, 0x004500, 0x0000A7, 0x1F2F00, 0x510000, 0x3C1A00, 0x0D3600, 0x59004F, 0x004400, 0x000395, 0x1200A8, 0x003D00,
    /* 0x050:          b1 */  0x000000, 0x272727, 0x6C0000, 0x00454D, 0x5200A0, 0x005700, 0x1803B5, 0x344200, 0x621800, 0x4E2F00, 0x254900, 0x690060, 0x005600, 0x001EA3, 0x2900B6, 0x124F00,
    /* 0x060:          b2 */  0x000000, 0x313131, 0x740D00, 0x004E56, 0x5B00A7, 0x005F00, 0x2312BB, 0x3D4B00, 0x6A2300, 0x573900, 0x2F5100, 0x710068, 0x005E00, 0x0029AA, 0x3307BD, 0x1E5800,
    /* 0x070:          b3 */  0x000000, 0x444444, 0x83251B, 0x005F66, 0x6B0DB5, 0x146F00, 0x3729C9, 0x4F5C00, 0x7A3700, 0x674B00, 0x426200, 0x800F78, 0x006E00, 0x1A3CB8, 0x4521CB, 0x336800,
    /* 0x080:          b4 */  0x000000, 0x767676, 0xAF5C55, 0x338D94, 0x984BDD, 0x4F9C00, 0x6B5FE5, 0x7F8B00, 0xA66B17, 0x957C00, 0x749000, 0xAC4CA4, 0x369B41, 0x536FE0, 0x7759E5, 0x679600,
    /* 0x090:          b5 */  0x000000, 0x949494, 0xCA7C75, 0x57AAB0, 0xB56CE5, 0x70B814, 0x8A7EE5, 0x9DA800, 0xC18A40, 0xB1990D, 0x92AD00, 0xC76DC0, 0x5AB763, 0x748DE5, 0x9579E5, 0x86B200,
    /* 0x0A0:          b6 */  0x000000, 0xB0B0B0, 0xE49993, 0x77C6CC, 0xD08BE5, 0x8ED33D, 0xA79CE5, 0xB9C31F, 0xDCA762, 0xCCB539, 0xAFC81C, 0xE28CDA, 0x7AD282, 0x92AAE5, 0xB197E5, 0xA3CD21,
    /* 0x0B0:          b7 */  0x000000, 0xE5E5E5, 0xE5D1CB, 0xB1E5E5, 0xE5C3E5, 0xC7E57F, 0xDDD3E5, 0xE5E568, 0xE5DD9F, 0xE5E57C, 0xE4E566, 0xE5C4E5, 0xB4E5BB, 0xCAE0E5, 0xE5CEE5, 0xDAE569,
    /* 0x0C0: TED NTSC b0 */  0x000000, 0x080808, 0x490000, 0x001C19, 0x170049, 0x001B00, 0x000456, 0x2B0B00, 0x480000, 0x3D0200, 0x1F0F00, 0x380024, 0x001F00, 0x000D4B, 0x000057, 0x121400,
    /* 0x0D0:          b1 */  0x000000, 0x181818, 0x590306, 0x002C29, 0x270459, 0x082B00, 0x001466, 0x3B1B00, 0x580900, 0x4D1200, 0x2F1F00, 0x480034, 0x002F00, 0x001D5B, 0x001067, 0x222400,
    /* 0x0E0:          b2 */  0x000000, 0x202020, 0x610B0E, 0x003431, 0x2F0C61, 0x103300, 0x001C6E, 0x432300, 0x601100, 0x551A00, 0x372700, 0x50083C, 0x003703, 0x002563, 0x08186F, 0x2A2C00,
    /* 0x0F0:          b3 */  0x000000, 0x303030, 0x711B1E, 0x004441, 0x3F1C71, 0x204300, 0x0C2C7E, 0x533300, 0x702101, 0x652A00, 0x473700, 0x60184C, 0x004713, 0x003573, 0x18287F, 0x3A3C00,
    /* 0x100:          b4 */  0x000000, 0x606060, 0xA14B4E, 0x1E7471, 0x6F4CA1, 0x50731E, 0x3C5CAE, 0x836311, 0xA05131, 0x955A1C, 0x776710, 0x90487C, 0x2F7743, 0x2A65A3, 0x4858AF, 0x6A6C11,
    /* 0x110:          b5 */  0x000000, 0x808080, 0xC16B6E, 0x3E9491, 0x8F6CC1, 0x70933E, 0x5C7CCE, 0xA38331, 0xC07151, 0xB57A3C, 0x978730, 0xB0689C, 0x4F9763, 0x4A85C3, 0x6878CF, 0x8A8C31,
    /* 0x120:          b6 */  0x000000, 0xA0A0A0, 0xDF8B8E, 0x5EB4B1, 0xAF8CDF, 0x90B35E, 0x7C9CDF, 0xC3A351, 0xDF9171, 0xD59A5C, 0xB7A750, 0xD088BC, 0x6FB783, 0x6AA5DF, 0x8898DF, 0xAAAC51,
    /* 0x130:          b7 */  0x000000, 0xDFDFDF, 0xDFCBCE, 0x9EDFDF, 0xDFCCDF, 0xD0DF9E, 0xBCDCDF, 0xDFDF91, 0xDFD1B1, 0xDFDA9C, 0xDFDF90, 0xDFC8DF, 0xAFDFC3, 0xAADFDF, 0xC8D8DF, 0xDFDF91,
    //                        ---00---  ---01---  ---02---  ---03---  ---04---  ---05---  ---06---  ---07---  ---08---  ---09---  ---10---  ---11---  ---12---  ---13---  ---14---  ---15---
};

const uint8_t cbm_default_color_maps[3][16] = {
    //                   0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
    //                   K     B     G     C     R     M     Y    LG    DG    LB    LG    LC    LR    LM    LY     W
    /* 0: VIC    */ {    0,    6,    5,    3,    2,    4,    8,    1,    0,   14,   13,   11,   10,   12,   15,    1 },
    /* 1: VIC-II */ {    0,    6,    5,    3,    2,    4,    8,   15,   11,   14,   13,    3,   10,    4,    7,    1 },
    /* 2: TED    */ { 0x00, 0x16, 0x15, 0x13, 0x12, 0x14, 0x17, 0x61, 0x11, 0x66, 0x65, 0x63, 0x62, 0x64, 0x67, 0x71 },
};

uint32_t cbm_map_rgbi_color(uint32_t color) {
    // C128 80-column RGBI mode -> same color logic as for PC RGBI
    color = RF_MapRGBToStandardColor(color, 200);
    if (color == RF_COLOR_YELLOW) {
        return 0xAA5500;
    } else {
        return RF_MapStandardColorToRGB(color, 0,170, 85,255);
    }
}

uint32_t cbm_map_color(RF_Context* ctx, uint32_t color, bool is_fg, bool is_border) {
    uint32_t sys_id = ctx->system->sys_id;
    bool is_vic = (RF_EXTRACT_ID(sys_id, 1) == '2');
    bool is_ted = (RF_EXTRACT_ID(sys_id, 1) == 'P');
    bool is_pal = (RF_EXTRACT_ID(sys_id, 3) == 'P');
    const uint32_t *pal = &cbm_palettes[
        is_ted ?  (is_pal ? 0x040 : 0x0C0)
               : ((is_pal ? 0x000 : 0x010) + (is_vic ? 0x000 : 0x020))
    ];
    const uint8_t *cmap = cbm_default_color_maps[is_ted ? 2 : (is_vic ? 0 : 1)];
    if (color == RF_COLOR_DEFAULT) {
        switch (RF_EXTRACT_ID(sys_id, 1)) {
            case 'P': return is_border ? pal[0x6E] : is_fg ? pal[0] : pal[0x71];  // TED default colors
            case '2': return is_border ? pal[3] : is_fg ? pal[6] : pal[1];  // VIC-20 default colors
            case '8': return is_fg ? pal[13] : pal[11];  // C128 40-column default colors
            case '1': return is_fg ? (RF_COLOR_CYAN | RF_COLOR_BRIGHT) : RF_COLOR_BLACK;  // C128 80-column default colors
            case 'S': return is_border ? pal[3] : is_fg ? pal[6] : pal[1];  // SX-64 default colors
            default:  return is_fg ? pal[14] : pal[6];  // C64 default colors
        }
    }
    if (sys_id == RF_MAKE_ID('C','1','2','8')) {
        return cbm_map_rgbi_color(color);
    } else if (RF_IS_RGB_COLOR(color)) {
        return pal[RF_PaletteLookup(ctx, pal, is_ted ? 128 : 16, color)];
    } else if (RF_IS_STD_COLOR(color)) {
        return pal[cmap[color & 15]];
    } else {  // native color
        return pal[color & (is_ted ? 127 : 15)];
    }
}

uint32_t cbm_map_border_color(RF_Context* ctx, uint32_t color) {
    if (ctx->system->sys_id == RF_MAKE_ID('C','1','2','8')) {
        if (ctx->default_bg == RF_COLOR_DEFAULT) { return 0; }
        return cbm_map_rgbi_color(ctx->default_bg);
    }
    return cbm_map_color(ctx, color, true, true);
}

void cbm_prepare_cell(RF_RenderCommand* cmd) {
    cmd->fg = cbm_map_color(cmd->ctx, cmd->fg, true, false);
    cmd->bg = cbm_map_color(cmd->ctx, cmd->ctx->default_bg, false, false);
    cmd->reverse_cursor = cmd->is_cursor && !(cmd->blink_phase & 1);
}

uint32_t pet_map_border_color(RF_Context* ctx, uint32_t color) {
    (void)ctx, (void)color;
    return 0;  // always black
}

void pet_render_cell(RF_RenderCommand* cmd) {
    cmd->fg = 0xFFFFFF;
    cmd->bg = 0x000000;
    cmd->reverse_cursor = cmd->is_cursor && !(cmd->blink_phase & 1);
    RF_RenderCell(cmd);
    if (cmd->ctx->system->cell_size.y > 8) {
        // PET 8032's ninth row is *always* black
        memset(&cmd->pixel[cmd->ctx->stride * 8], 0, 24);
    }
}

static const RF_SysClass cbmclass = {
    cbm_map_border_color,
    cbm_prepare_cell,
    NULL,  // render_cell = default
    NULL,  // check_font = default
};

static const RF_SysClass petclass = {
    pet_map_border_color,
    NULL,  // prepare_cell = default (unused)
    pet_render_cell,
    NULL,  // check_font = default
};

static const char pet40default[] =
    "*** COMMODORE BASIC ***\n\n"
    " 7167 BYTES FREE\n\n"
    "READY.\n";
static const char pet80default[] =
    "*** commodore basic 4.0 ***\n\n"
    " 31743 bytes free\n\n"
    "ready.\n";
static const char vicdefault[] =
    "**** CBM BASIC V2 ****\n"
    "`y023583 BYTES FREE\n\n"
    "READY.\n";
static const char c64default[] =
    "\n`c31**** COMMODORE 64 BASIC V2 ****\n\n"
    " 64K RAM SYSTEM`X2338911 BASIC BYTES FREE\n\n"
    "READY.\n";
static const char sx64default[] =
    "\n`c30*****  SX-64 BASIC V2.0  *****\n\n"
    " 64K RAM SYSTEM`X2338911 BASIC BYTES FREE\n\n"
    "READY.\n";
static const char c128default[] =
    "\n COMMODORE BASIC V7.0`X18122365 BYTES FREE\n"
    "`c34(C)1986 COMMODORE ELECTRONICS, LTD.\n"
    "`c22(C)1977 MICROSOFT CORP.\n"
    "`c18ALL RIGHTS RESERVED\n\n"
    "READY.\n";
static const char plus4default[] =
    "\n COMMODORE BASIC V3.5`X1860671 BYTES FREE\n"
    " 3-PLUS-1 ON KEY F1\n\n"
    "READY.\n";

//                                  sys_id,                         name,                             class,    scrn,         scrsz,   cellsz,  fontsz,   b_ul,    b_lr,  aspect, blink, monitor,          default_font_id
const RF_System RF_Sys_PET40      = { RF_MAKE_ID('C','0','4','N'), "Commodore PET 2001",             &petclass, pet40default, {40,24}, { 8, 8}, { 8, 8}, {32,33}, {32,41}, {1,1},   333, RF_MONITOR_GREEN, RF_MAKE_ID('C','0','8','s') };
const RF_System RF_Sys_PET80      = { RF_MAKE_ID('C','0','8','N'), "Commodore PET 8032",             &petclass, pet80default, {80,25}, { 8, 9}, { 8, 8}, {32,20}, {32,22}, {1,2},   333, RF_MONITOR_GREEN, RF_MAKE_ID('C','0','8','s') };
const RF_System RF_Sys_VIC20_NTSC = { RF_MAKE_ID('C','2','0','N'), "Commodore VIC-20 (NTSC)",        &cbmclass, vicdefault,   {22,23}, { 8, 8}, { 8, 8}, {14,28}, {16,28}, {2,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('C','2','0','s') };
const RF_System RF_Sys_VIC20_PAL  = { RF_MAKE_ID('C','2','0','P'), "Commodore VIC-20 (PAL)",         &cbmclass, vicdefault,   {22,23}, { 8, 8}, { 8, 8}, {12,52}, {14,52}, {2,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('C','2','0','s') };
const RF_System RF_Sys_C64_NTSC   = { RF_MAKE_ID('C','6','4','N'), "Commodore 64 (NTSC)",            &cbmclass, c64default,   {40,25}, { 8, 8}, { 8, 8}, {46,20}, {46,20}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('C','6','4','s') };
const RF_System RF_Sys_C64_PAL    = { RF_MAKE_ID('C','6','4','P'), "Commodore 64 (PAL)",             &cbmclass, c64default,   {40,25}, { 8, 8}, { 8, 8}, {42,44}, {42,44}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('C','6','4','s') };
const RF_System RF_Sys_SX64_NTSC  = { RF_MAKE_ID('C','S','X','N'), "Commodore SX-64 (NTSC)",         &cbmclass, sx64default,  {40,25}, { 8, 8}, { 8, 8}, {46,20}, {46,20}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('C','6','4','s') };
const RF_System RF_Sys_SX64_PAL   = { RF_MAKE_ID('C','S','X','P'), "Commodore SX-64 (PAL)",          &cbmclass, sx64default,  {40,25}, { 8, 8}, { 8, 8}, {42,44}, {42,44}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('C','6','4','s') };
const RF_System RF_Sys_C128_NTSC  = { RF_MAKE_ID('C','8','0','N'), "Commodore 128 (40-column NTSC)", &cbmclass, c128default,  {40,25}, { 8, 8}, { 8, 8}, {46,20}, {46,20}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('C','8','0','s') };
const RF_System RF_Sys_C128_PAL   = { RF_MAKE_ID('C','8','0','P'), "Commodore 128 (40-column PAL)",  &cbmclass, c128default,  {40,25}, { 8, 8}, { 8, 8}, {42,44}, {42,44}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('C','8','0','s') };
const RF_System RF_Sys_C128_80Col = { RF_MAKE_ID('C','1','2','8'), "Commodore 128 (80-column)",      &cbmclass, c128default,  {80,25}, { 8, 8}, { 8, 8}, {94,20}, {92,20}, {1,2},   266, RF_MONITOR_COLOR, RF_MAKE_ID('C','8','0','s') };
const RF_System RF_Sys_Plus4_NTSC = { RF_MAKE_ID('C','P','4','N'), "Commodore 16/116/Plus4 (NTSC)",  &cbmclass, plus4default, {40,25}, { 8, 8}, { 8, 8}, {46,20}, {46,20}, {1,1},   266, RF_MONITOR_COLOR, RF_MAKE_ID('C','P','4','s') };
const RF_System RF_Sys_Plus4_PAL  = { RF_MAKE_ID('C','P','4','P'), "Commodore 16/116/Plus4 (PAL)",   &cbmclass, plus4default, {40,25}, { 8, 8}, { 8, 8}, {42,44}, {42,44}, {1,1},   320, RF_MONITOR_COLOR, RF_MAKE_ID('C','P','4','s') };
