#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "retrofont.h"

const RF_Cell RF_EmptyCell = { 32, 1, 0,0,0,0,0,0, RF_COLOR_DEFAULT, RF_COLOR_DEFAULT };

///////////////////////////////////////////////////////////////////////////////

RF_Context* RF_CreateContext(uint32_t sys_id) {
    RF_Context* ctx = (RF_Context*) calloc(1, sizeof(RF_Context));
    if (!ctx) { return NULL; }
    if (!sys_id) { sys_id = RF_SystemList[0]->sys_id; }
    if (!RF_SetSystem(ctx, sys_id)) { free((void*)ctx); return NULL; }
    return ctx;
}

bool RF_SetSystem(RF_Context* ctx, uint32_t sys_id) {
    if (!ctx) { return false; }
    for (const RF_System* const* p_sys = RF_SystemList;  *p_sys;  ++p_sys) {
        if ((*p_sys)->sys_id == sys_id) {
            ctx->system = *p_sys;
            ctx->default_fg = ctx->default_bg = ctx->border_color = RF_COLOR_DEFAULT;
            ctx->pixel_aspect = 1.0f;
            ctx->attrib = RF_EmptyCell;
            if (!RF_SetFont(ctx, 0)) { return false; }
            if (((*p_sys)->default_screen_size.x & (*p_sys)->default_screen_size.y) & RF_SIZE_PIXELS) {
                uint32_t sx = (*p_sys)->border_ul.x + ((*p_sys)->default_screen_size.x & RF_SIZE_MASK) + (*p_sys)->border_lr.x;
                uint32_t sy = (*p_sys)->border_ul.y + ((*p_sys)->default_screen_size.y & RF_SIZE_MASK) + (*p_sys)->border_lr.y;
                ctx->pixel_aspect = (4.0f * (float)sy) / (3.0f * (float)sx);
            } else if ((*p_sys)->font_size.x && (*p_sys)->font_size.y) {
                uint32_t sx = (*p_sys)->border_ul.x + (uint32_t)((*p_sys)->cell_size.x) * (uint32_t)((*p_sys)->default_screen_size.x) + (*p_sys)->border_lr.x;
                uint32_t sy = (*p_sys)->border_ul.y + (uint32_t)((*p_sys)->cell_size.y) * (uint32_t)((*p_sys)->default_screen_size.y) + (*p_sys)->border_lr.y;
                ctx->pixel_aspect = (4.0f * (float)sy) / (3.0f * (float)sx);
            }
            ctx->border_color_changed = true;
            RF_InvalidatePalette(ctx);
            return true;
        }
    }
    return false;
}

bool RF_SystemCanUseFont(const RF_System* sys, const RF_Font* font) {
    if (!sys || !font) { return false; }
    if (sys->font_size.x && (sys->font_size.x != font->font_size.x)) { return false; }
    if (sys->font_size.y && (sys->font_size.y != font->font_size.y)) { return false; }
    if (!sys->cls || !sys->cls->check_font) { return true; }
    return sys->cls->check_font(sys->sys_id, font);
}

bool RF_SetFont(RF_Context* ctx, uint32_t font_id) {
    bool result = false;
    if (!ctx || !ctx->system) { return false; }
    if (!font_id) { font_id = ctx->system->default_font_id; }
    for (const RF_Font* font = RF_FontList;  font->font_id;  ++font) {
        if (!RF_SystemCanUseFont(ctx->system, font)) {
            continue;  // font doesn't fit with current system
        }
        if (font->font_id == font_id) {  // font found
            ctx->font = font;
            result = true;
            break;
        }
        if (!ctx->font) {  // if no font set so far, fall back to any suitable
            ctx->font = font;
            result = true;
        }
    }
    if (result) {
        RF_SetFallbackMode(ctx, ctx->fallback);
    }
    return result;
}

void RF_SetFallbackMode(RF_Context* ctx, RF_FallbackMode mode) {
    if (!ctx) { return; }
    RF_Invalidate(ctx, false);
    ctx->fallback = mode;
    memset((void*)ctx->glyph_offset_cache, 0xFF, sizeof(ctx->glyph_offset_cache));
    ctx->fb_glyphs = NULL;
    if (ctx->font && ((mode == RF_FB_FONT) || (mode == RF_FB_FONT_CHAR) || (mode == RF_FB_CHAR_FONT))) {
        for (const RF_FallbackGlyphs* fb = RF_FallbackGlyphsList;  fb->font_size.x && fb->font_size.y && fb->glyph_map;  ++fb) {
            if ((ctx->font->font_size.x == fb->font_size.x)
            &&  (ctx->font->font_size.y == fb->font_size.y)) {
                ctx->fb_glyphs = fb;
                break;
            }
        }
    }
}

bool RF_ResizeScreen(RF_Context* ctx, uint16_t new_width, uint16_t new_height, bool with_border) {
    RF_Cell *new_screen, *c;
    RF_Coord bmpsize;
    uint8_t *new_bmp;

    if (!ctx || !ctx->system || (!ctx->system->font_size.x && !ctx->font)) { return false; }
    if (!new_width)  { new_width  = ctx->screen_size.x; }
    if (!new_height) { new_height = ctx->screen_size.y; }
    uint16_t dsx = ctx->system->default_screen_size.x & RF_SIZE_MASK;
    uint16_t dsy = ctx->system->default_screen_size.y & RF_SIZE_MASK;
    if (!new_width  || (new_width  == RF_SIZE_DEFAULT)) {
        new_width  = (ctx->system->default_screen_size.x & RF_SIZE_PIXELS)
                   ? (dsx / ctx->font->font_size.x)
                   : ctx->system->default_screen_size.x;
    }
    if (!new_height || (new_height == RF_SIZE_DEFAULT)) {
        new_height = (ctx->system->default_screen_size.y & RF_SIZE_PIXELS)
                   ? (dsy / ctx->font->font_size.y)
                   : ctx->system->default_screen_size.y;
    }
    c = new_screen = (RF_Cell*) malloc(sizeof(RF_Cell) * new_width * new_height);
    if (!new_screen) { return false; }

    uint16_t csx = ctx->system->cell_size.x + (ctx->system->font_size.x ? 0 : ctx->font->font_size.x);
    uint16_t csy = ctx->system->cell_size.y + (ctx->system->font_size.y ? 0 : ctx->font->font_size.y);
    uint16_t mainx = new_width  * csx;
    uint16_t mainy = new_height * csy;
    bmpsize.x = mainx;
    bmpsize.y = mainy;
    if (with_border) {
        if ((ctx->system->default_screen_size.x & RF_SIZE_PIXELS)
        && (mainx < dsx) && (mainx > (dsx - ctx->font->font_size.x)))
            { bmpsize.x = dsx; }
        if ((ctx->system->default_screen_size.y & RF_SIZE_PIXELS)
        && (mainx < dsy) && (mainx > (dsy - ctx->font->font_size.y)))
            { bmpsize.y = dsy; }
        bmpsize.x += ctx->system->border_ul.x + ctx->system->border_lr.x;
        bmpsize.y += ctx->system->border_ul.y + ctx->system->border_lr.y;
    }
    new_bmp = (uint8_t*) realloc((void*)ctx->bitmap, (size_t)bmpsize.x * (size_t)bmpsize.y * 3);
    if (!new_bmp) { free((void*)new_screen); return false; }

    if (!ctx->screen) { ctx->screen_size.x = ctx->screen_size.y = 0; }
    for (uint16_t y = 0;  y < new_height;  ++y) {
        const RF_Cell* r = (ctx->screen && (y < ctx->screen_size.y)) ? &ctx->screen[ctx->screen_size.x * y] : NULL;
        for (uint16_t x = 0;  x < new_width;  ++x) {
            *c = (r && (x < ctx->screen_size.x)) ? (*r++) : RF_EmptyCell;
            c->dirty = 1;
            ++c;
        }
    }

    free((void*)ctx->screen);
    ctx->screen = new_screen;
    ctx->screen_size.x = new_width;
    ctx->screen_size.y = new_height;
    ctx->bitmap = new_bmp;
    ctx->stride = bmpsize.x * 3;
    ctx->bitmap_size = bmpsize;
    ctx->has_border = with_border && ((ctx->system->border_lr.x | ctx->system->border_lr.y | ctx->system->border_ul.x | ctx->system->border_ul.y) != 0);
    if (ctx->has_border) {
        ctx->main_ul = ctx->system->border_ul;
        ctx->main_lr.x = ctx->system->border_ul.x + mainx;
        ctx->main_lr.y = ctx->system->border_ul.y + mainy;
    } else {
        ctx->main_ul.x = ctx->main_ul.y = 0;
        ctx->main_lr = bmpsize;
    }
    ctx->cell_size.x = csx;
    ctx->cell_size.y = csy;
    ctx->border_color_changed = true;
    return true;
}

void RF_MoveCursor(RF_Context* ctx, uint16_t new_col, uint16_t new_row) {
    if (!ctx || !ctx->screen) { return; }
    if ((ctx->cursor_pos.x < ctx->screen_size.x) && (ctx->cursor_pos.y < ctx->screen_size.y)) {
        ctx->screen[ctx->screen_size.x * ctx->cursor_pos.y + ctx->cursor_pos.x].dirty = 1;
    }
    if ((new_col < ctx->screen_size.x) && (new_row < ctx->screen_size.y)) {
        ctx->screen[ctx->screen_size.x * new_row + new_col].dirty = 1;
    }
    ctx->cursor_pos.x = new_col;
    ctx->cursor_pos.y = new_row;
}

void RF_ClearScreen(RF_Context* ctx, const RF_Cell* cell) {
    RF_Cell* c;
    if (!ctx || !ctx->screen) { return; }
    if (!cell) { cell = &RF_EmptyCell; }
    c = ctx->screen;
    for (size_t count = (size_t)ctx->screen_size.x * (size_t)ctx->screen_size.y;  count;  --count) {
        *c = *cell;
        c->dirty = 1;
        ++c;
    }
}

void RF_Invalidate(RF_Context* ctx, bool with_border) {
    RF_Cell *c;
    if (!ctx || !ctx->screen) { return; }
    c = ctx->screen;
    for (size_t count = (size_t)ctx->screen_size.x * (size_t)ctx->screen_size.y;  count;  --count) {
        c->dirty = 1;
        ++c;
    }
    if (with_border) { ctx->border_color_changed = true; }
}

void RF_DestroyContext(RF_Context* ctx) {
    if (!ctx) { return; }
    free((void*)ctx->screen);  ctx->screen = NULL;
    free((void*)ctx->bitmap);  ctx->bitmap = NULL;
    free((void*)ctx);
}

///////////////////////////////////////////////////////////////////////////////

#define PUT_PIXEL(p, color) do { \
    uint32_t color_ = color; \
    *p++ = RF_COLOR_R(color_); \
    *p++ = RF_COLOR_G(color_); \
    *p++ = RF_COLOR_B(color_); \
} while (0)

static uint8_t* fill_border(uint8_t* p, uint32_t color, size_t count) {
    while (count--) { PUT_PIXEL(p, color); }
    return p;
}

#define INVALID_GLYPH ((uint32_t)(-1))

static uint32_t glyph_map_lookup(const RF_GlyphMapEntry *map, uint32_t count, uint32_t codepoint) {
    // binary search for a glyph
    uint32_t a = 0, b = count - 1;
    while (b > a) {
//printf("U+%04X (#%d) <= U+%04X <= U+%04X (#%d)\n", ctx->font->glyph_map[a].codepoint, a, cmd.cell->codepoint, ctx->font->glyph_map[b].codepoint, b);
        uint32_t c = (a + b + 1) >> 1;
        if (codepoint < map[c].codepoint)
            { b = c - 1; } else { a = c; }
    }
    return (map[a].codepoint == codepoint) ? map[a].bitmap_offset : INVALID_GLYPH;
}

static uint32_t glyph_map_lookup_with_fallback(const RF_GlyphMapEntry *map, uint32_t count, uint32_t codepoint, bool allow_fallback) {
    uint32_t header, multi_fb_count;
    uint32_t offset = glyph_map_lookup(map, count, codepoint);
    if ((offset != INVALID_GLYPH) || !allow_fallback) { return offset; }
    // if we arrived here, we need to look for fallback characters -> fetch the header
    header = glyph_map_lookup(RF_FallbackMap, RF_FallbackMapSize, codepoint);
    if (header == INVALID_GLYPH) { return header; }
    // decode header; if it's a single fallback entry, look that up
    multi_fb_count = header >> 24;
    if (!multi_fb_count) { return glyph_map_lookup(map, count, header); }
    // if we arrived here, there's multiple possible fallback characters -> iterate over them
    header &= 0xFFFFFF;
    do {
        offset = glyph_map_lookup(map, count, RF_MultiFallbackData[header++]);
    } while (--multi_fb_count && (offset == INVALID_GLYPH));
    return offset;
}

bool RF_Render(RF_Context* ctx, uint32_t time_msec) {
    bool result = false;
    RF_RenderCommand cmd;
    if (!ctx || !ctx->system || !ctx->font || !ctx->screen || !ctx->bitmap) { return false; }
    if (ctx->border_color_changed) {
        uint32_t color = ctx->system->cls->map_border_color(ctx, ctx->border_color);
        if (ctx->has_border) {
            uint8_t* p = fill_border(ctx->bitmap, color, ctx->bitmap_size.x * ctx->main_ul.y + ctx->main_ul.x);
            for (uint16_t y = ctx->screen_size.y * ctx->cell_size.y;  y;  --y) {
                p += ctx->screen_size.x * ctx->cell_size.x * 3;
                if (y > 1) { p = fill_border(p, color, ctx->bitmap_size.x - ctx->main_lr.x + ctx->main_ul.x); }
            }
            fill_border(p, color, ctx->bitmap_size.x - ctx->main_lr.x + ctx->bitmap_size.x * (ctx->bitmap_size.y - ctx->main_lr.y));
        }
        result = true;
        ctx->border_rgb = color;
        ctx->border_color_changed = false;
    }
    cmd.ctx = ctx;
    cmd.cell = ctx->screen;
    cmd.blink_phase = ctx->system->blink_interval_msec ? (uint8_t)(time_msec / ctx->system->blink_interval_msec) : 0;
    for (uint16_t y = 0;  y < ctx->screen_size.y;  ++y) {
        uint8_t* pixel_ptr = &ctx->bitmap[((ctx->has_border ? ctx->system->border_ul.y : 0) + y * ctx->cell_size.y) * ctx->stride
                                         + (ctx->has_border ? ctx->system->border_ul.x : 0) * 3];
        for (uint16_t x = 0;  x < ctx->screen_size.x;  ++x) {
            cmd.is_cursor = (y == ctx->cursor_pos.y) && (x == ctx->cursor_pos.x);
            if (cmd.cell->dirty || ((cmd.blink_phase != ctx->last_blink_phase) && (cmd.cell->blink || cmd.is_cursor))) {
                // prepare the RenderCommand
                cmd.glyph_data = NULL;
                cmd.pixel = pixel_ptr;
                cmd.codepoint = cmd.cell->codepoint;
                cmd.fg = cmd.cell->fg;  if (cmd.fg == RF_COLOR_DEFAULT) { cmd.fg = ctx->default_fg; }
                cmd.bg = cmd.cell->bg;  if (cmd.bg == RF_COLOR_DEFAULT) { cmd.bg = ctx->default_bg; }
                cmd.offset.x = cmd.offset.y = 0;
                cmd.line_start = cmd.line_end = 0;
                cmd.line_xor = true;
                cmd.underline = cmd.bold = false;
                cmd.invisible = !!cmd.cell->invisible;
                cmd.reverse_attr = !!cmd.cell->reverse;
                cmd.reverse_cursor = cmd.reverse_blink = false;
                if (ctx->system->cls->prepare_cell) {
                    ctx->system->cls->prepare_cell(&cmd);
                }

                // try to retrieve the offset from the cache
                uint32_t cache_index = cmd.codepoint - RF_GLYPH_CACHE_MIN;
                uint32_t offset = (cache_index < RF_GLYPH_CACHE_SIZE) ? ctx->glyph_offset_cache[cache_index] : INVALID_GLYPH;
                // if the glyph is not cached (or not cacheable), look it up the hard way
                if (offset == INVALID_GLYPH) {
                    // not cached (or not cacheable) -> look up the glyph the hard way
                    {
                        // try the font's native glyph map first
                        offset = glyph_map_lookup_with_fallback(
                                    ctx->font->glyph_map, ctx->font->glyph_count, cmd.codepoint,
                                    (ctx->fallback == RF_FB_CHAR) || (ctx->fallback == RF_FB_CHAR_FONT));
                    }
                    if ((offset == INVALID_GLYPH) && ctx->fb_glyphs) {
                        // look for fallback glyphs in other fonts of the same size (if allowed to)
                        offset = glyph_map_lookup_with_fallback(
                                    ctx->fb_glyphs->glyph_map, ctx->fb_glyphs->glyph_count, cmd.codepoint,
                                    (ctx->fallback == RF_FB_CHAR_FONT) || (ctx->fallback == RF_FB_FONT_CHAR));
                    }
                    if (offset == INVALID_GLYPH) {
                        // last resort: use font's built-in fallback glyph
                        offset = ctx->font->fallback_offset;
                    }
                    if (cache_index < RF_GLYPH_CACHE_SIZE) {
                        // store in cache
                        ctx->glyph_offset_cache[cache_index] = offset;
                    }
                }

                // render the glyph
                cmd.glyph_data = &RF_GlyphBitmaps[offset];
                if (ctx->system->cls->render_cell) {
                    ctx->system->cls->render_cell(&cmd);
                } else {
                    RF_RenderCell(&cmd);
                }
                cmd.cell->dirty = 0;
                result = true;
            }
            ++cmd.cell;
            pixel_ptr += ctx->cell_size.x * 3;
        }
    }
    ctx->last_blink_phase = cmd.blink_phase;
    return result;
}

void RF_RenderCell(RF_RenderCommand* cmd) {
    uint8_t *p, bits, mask, lsb, lsb_mask;
    const uint8_t *g;
    uint16_t uline_pos;
    if (!cmd || !cmd->cell || !cmd->pixel || !cmd->glyph_data) { return; }
    if (!RF_IS_RGB_COLOR(cmd->fg)) { cmd->fg = (cmd->fg == RF_COLOR_DEFAULT) ? 0xFFFFFF : RF_MapStandardColorToRGB(cmd->fg, 0,160, 0,255); }
    if (!RF_IS_RGB_COLOR(cmd->bg)) { cmd->bg = (cmd->bg == RF_COLOR_DEFAULT) ? 0xFFFFFF : RF_MapStandardColorToRGB(cmd->bg, 0,160, 0,255); }
    if ((cmd->reverse_attr ? 1 : 0) ^ (cmd->reverse_cursor ? 1 : 0) ^ (cmd->reverse_blink ? 1 : 0)) {
        uint32_t t = cmd->fg;  cmd->fg = cmd->bg;  cmd->bg = t;
    }
    mask = cmd->invisible ? 0 : 0xFF;
    lsb_mask = cmd->bold ? 1 : 0;
    uline_pos = (cmd->underline && cmd->ctx->font->underline_row) ? (cmd->offset.y + cmd->ctx->font->underline_row) : cmd->ctx->cell_size.y;
    g = cmd->glyph_data;
    for (uint16_t y = 0;  y < cmd->ctx->cell_size.y;  ++y) {
        const bool core_row = (y >= cmd->offset.y) && (y < (cmd->offset.y + cmd->ctx->font->font_size.y));
        const bool uline_row = (y == uline_pos);
        const bool xline_row = (y >= cmd->line_start) && (y < cmd->line_end);
        const uint8_t line_or  = (uline_row || (xline_row && !cmd->line_xor)) ? 1 : 0;
        const uint8_t line_xor = xline_row && cmd->line_xor ? 1 : 0;
        p = &cmd->pixel[cmd->ctx->stride * y];
        for (uint16_t x = cmd->offset.x;  x;  --x) {
            PUT_PIXEL(p, (line_or ^ line_xor) ? cmd->fg : cmd->bg);
        }
        bits = lsb = 0;
        for (uint16_t x = 0;  x < (cmd->ctx->cell_size.x - cmd->offset.x);  ++x) {
            if (core_row && (x < cmd->ctx->font->font_size.x) && !(x & 7)) {
                bits = (*g++) & mask;
            }
            PUT_PIXEL(p, (((bits & 1) | lsb | line_or) ^ line_xor) ? cmd->fg : cmd->bg);
            lsb = bits & lsb_mask;
            bits >>= 1;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void RF_DemoScreen(RF_Context* ctx) {
    if (!ctx || !ctx->screen) { return; }
    static const uint32_t cp_offsets[] = {
        0x0020, 0x0040, 0x0060,          // Basic Latin (a.k.a. standard ASCII)
        0x00A0, 0x00C0, 0x00E0,          // Latin-1 Supplement
        0x2580,                          // Block Elements
        0x1FB70, 0x1FB90, 0x1FBB0,       // (some) Symbols for Legacy Computing
        0x2500, 0x2520, 0x2540, 0x2560,  // Box Drawing
        0x25A0, 0x25C0, 0x25E0,          // Geometric Shapes
        0x2190,                          // Arrows (most basic ones only)
    };
    uint16_t demo_row_count = (uint16_t)(sizeof(cp_offsets) / sizeof(*cp_offsets));
    uint16_t attribute_start_row = (ctx->screen_size.y > demo_row_count) || (ctx->screen_size.x > 32) ? demo_row_count : 9;
    RF_Cell *c = ctx->screen;
    for (uint16_t y = 0;  y < ctx->screen_size.y;  ++y) {
        for (uint16_t x = 0;  x < ctx->screen_size.x;  ++x) {
            uint32_t row_mod = 3;
            *c = RF_EmptyCell;
            c->codepoint = 0;
            if (y >= (attribute_start_row + 3)) {
                uint16_t r = (uint16_t) rand();
                c->fg = RF_COLOR_BLACK | (rand() & 15);
                do { c->bg = RF_COLOR_BLACK | (rand() & 15); } while (c->bg == c->fg);
                c->bold      = r >> 0;
                c->dim       = r >> 1;
                c->underline = r >> 2;
                c->blink     = r >> 3;
                c->reverse   = r >> 4;
                c->invisible = (r & 0x3F00) ? 0 : 1;  // make this very rare
            } else if (y >= attribute_start_row) {
                uint32_t full_phase = ((((uint32_t)(2 * x + 1)) << 10) + ctx->screen_size.x) / (ctx->screen_size.x * 2);
                uint8_t p = (uint8_t) full_phase, cA, cB;
                switch (full_phase >> 8) {
                    case 0:  cA = p;         cB = 0;         break;
                    case 1:  cA =     0xFF;  cB = p;         break;
                    case 2:  cA = p ^ 0xFF;  cB =     0xFF;  break;
                    default: cA = 0;         cB = p ^ 0xFF;  break;
                }
                switch (y - attribute_start_row) {
                    case 0:  c->fg = RF_COLOR_RGB(cA, cB, cB); break;
                    case 1:  c->fg = RF_COLOR_RGB(cB, cA, cB); break;
                    default: c->fg = RF_COLOR_RGB(cB, cB, cA); break;
                }
                c->codepoint = 64;
            } else if (x >= 64) {
                c->bold      = y >> 0;
                c->dim       = x >> 0;
                c->underline = y >> 1;
                c->blink     = x >> 1;
                c->reverse   = y >> 2;
                c->invisible = x >> 2;
            } else if (x >= 32) {
                c->fg = RF_COLOR_BLACK | (x & 15);
                c->bg = RF_COLOR_BLACK | (y & 15);
                c->reverse = x >> 4;
            } else {
                row_mod = demo_row_count;
            }
            if (!c->codepoint) {
                c->codepoint = (x & 31) + cp_offsets[y % row_mod];
                if ((c->codepoint >= 0x1FBC6) && (c->codepoint <= 0x1FBCF)) {
                    c->codepoint += 0x1FBF0 - 0x1FBC6;  // make ST's LED digits visible
                }
            }
            c->dirty = 1;
            ++c;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void RF_AddChar(RF_Context* ctx, uint32_t codepoint) {
    RF_Cell* pos;
    if (!codepoint) { return; }
    if (!ctx || !ctx->screen
    || (ctx->cursor_pos.x >= ctx->screen_size.x)
    || (ctx->cursor_pos.y >= ctx->screen_size.y))
        { return; }
    pos = &ctx->screen[ctx->cursor_pos.y * ctx->screen_size.x + ctx->cursor_pos.x];
    if (codepoint == RF_CP_TAB) {
        if (ctx->insert) {
            for (uint16_t i = 8 - (ctx->cursor_pos.x & 7);  i;  --i) {
                RF_AddChar(ctx, 32);
                if (!ctx->cursor_pos.x) { break; }
            }
        } else {
            RF_Coord c = ctx->cursor_pos;
            c.x = (c.x + 8) & (~7);
            if (c.x >= ctx->screen_size.x) {
                c.x = 0;
                if ((c.y + 1) < ctx->screen_size.y) { ++c.y; }
            }
            RF_MoveCursor(ctx, c.x, c.y);
        }
    } else if (codepoint == RF_CP_ENTER) {
        if (ctx->insert) {
            // scroll down first
            if ((++(ctx->cursor_pos.y)) >= ctx->screen_size.y) {
                ctx->cursor_pos.y = ctx->screen_size.y - 1;
                RF_ScrollRegionAB(ctx, 0,0, ctx->screen_size.x,ctx->screen_size.y, 0,-1, &ctx->attrib);
            } else {
                RF_ScrollRegionAB(ctx, 0,ctx->cursor_pos.y, ctx->screen_size.x,ctx->screen_size.y, 0,1, &ctx->attrib);
            }
            // copy remainder of current line onto next one
            RF_CopyRegionAB(ctx, ctx->cursor_pos.x,ctx->cursor_pos.y-1, ctx->screen_size.x,ctx->cursor_pos.y, 0,ctx->cursor_pos.y, &ctx->attrib);
            // clear what remains
            RF_ClearRegionPS(ctx, ctx->cursor_pos.x, ctx->cursor_pos.y - 1, ctx->screen_size.x, 1, &ctx->attrib);
            RF_ClearRegionPS(ctx, ctx->screen_size.x - ctx->cursor_pos.x, ctx->cursor_pos.y, ctx->screen_size.x, 1,
                             &ctx->screen[ctx->cursor_pos.y * ctx->screen_size.x + ctx->screen_size.x - ctx->cursor_pos.x - 1]);
            ctx->cursor_pos.x = 0;
        } else if ((ctx->cursor_pos.y + 1) == ctx->screen_size.y) {
            // overwrite mode, at end of screen -> insert new line
            RF_ScrollRegionAB(ctx, 0,0, ctx->screen_size.x,ctx->screen_size.y, 0,-1, &ctx->screen[ctx->screen_size.x * ctx->screen_size.y - 1]);
            ctx->cursor_pos.x = 0;
        } else {
            // otherwise, only move cursor in overwrite mode
            pos->dirty = 1;
            ctx->cursor_pos.x = 0;
            if ((++(ctx->cursor_pos.y)) >= ctx->screen_size.y) {
                ctx->cursor_pos.y = ctx->screen_size.y - 1;
            }
        }
    } else if (codepoint == 13) {
        // ignore carriage return
    } else if (codepoint == RF_CP_BACKSPACE) {
        if (ctx->cursor_pos.x) {
            if (ctx->insert) {
                --pos;
                for (uint16_t i = ctx->cursor_pos.x;  i < ctx->screen_size.x;  ++i) {
                    *pos = pos[1];
                    pos->dirty = 1;
                    ++pos;
                }
            } else {
                pos->dirty = 1;
                --pos;
            }
            --ctx->cursor_pos.x;
            pos->codepoint = 32;
        }
    } else if (codepoint == RF_CP_DELETE) {
        for (uint16_t i = ctx->cursor_pos.x + 1;  i < ctx->screen_size.x;  ++i) {
            *pos = pos[1];
            pos->dirty = 1;
            ++pos;
        }
        pos->codepoint = 32;
    } else {
        if (ctx->insert) {
            for (uint16_t i = ctx->screen_size.x - 1 - ctx->cursor_pos.x;  i;  --i) {
                pos[i] = pos[i-1];
                pos[i].dirty = true;
            }
        }
        *pos = ctx->attrib;
        pos->codepoint = codepoint;
        pos->dirty = 1;
        if ((++(ctx->cursor_pos.x)) >= ctx->screen_size.x) {
            ctx->cursor_pos.x = 0;
            if (ctx->insert) {
                if ((++(ctx->cursor_pos.y)) >= ctx->screen_size.y) {
                    ctx->cursor_pos.y = ctx->screen_size.y - 1;
                    RF_ScrollRegionAB(ctx, 0,0, ctx->screen_size.x,ctx->screen_size.y, 0,-1, &ctx->attrib);
                } else {
                    RF_ScrollRegionAB(ctx, 0,ctx->cursor_pos.y, ctx->screen_size.x,ctx->screen_size.y, 0,1, &ctx->attrib);
                }
            } else {
                if ((++(ctx->cursor_pos.y)) >= ctx->screen_size.y) {
                    RF_ScrollRegionAB(ctx, 0,0, ctx->screen_size.x,ctx->screen_size.y, 0,-1, &ctx->attrib);
                    --ctx->cursor_pos.y;
                }
            }
        }
    }
    // regardless of what just happened, mark the new cursor position as dirty
    if ((ctx->cursor_pos.x < ctx->screen_size.x) && (ctx->cursor_pos.y < ctx->screen_size.y)) {
        ctx->screen[ctx->cursor_pos.y * ctx->screen_size.x + ctx->cursor_pos.x].dirty = 1;
    }
}

RF_Cell* RF_GetCell(RF_Context* ctx, int x, int y) {
    return (ctx && ctx->screen
         && (x >= 0) && (x < ctx->screen_size.x)
         && (y >= 0) && (y < ctx->screen_size.y))
         ? &ctx->screen[x + y * ctx->screen_size.x]
         : NULL;
}

void RF_ClearRegionAB(RF_Context* ctx, int x0, int y0, int x1, int y1, const RF_Cell* attrib) {
    RF_Cell* c;
    if (!ctx || !ctx->screen) { return; }
    if (x0 < 0) { x0 = 0; }
    if (y0 < 0) { y0 = 0; }
    if (x1 >= ctx->screen_size.x) { x1 = ctx->screen_size.x; }
    if (y1 >= ctx->screen_size.y) { y1 = ctx->screen_size.y; }
    if ((x1 <= x0) || (y1 <= y0)) { return; }
    if (!attrib) { attrib = &RF_EmptyCell; }
    c = &ctx->screen[x0 + y0 * ctx->screen_size.x];
    x1 -= x0;
    for (y1 -= y0;  y1;  --y1) {
        for (int x = x1;  x;  --x) {
            *c = *attrib;
            c->codepoint = 32;
            c->dirty = 1;
            ++c;
        }
        c += ctx->screen_size.x - x1;
    }
}
void RF_ClearRegionPS(RF_Context* ctx, int x0, int y0, int w, int h, const RF_Cell* attrib) {
    RF_ClearRegionAB(ctx, x0,y0, x0+w,y0+h, attrib);
}

void RF_CopyRegionInt(RF_Context* ctx, int src_x0, int src_y0, int src_x1, int src_y1, int dest_x0, int dest_y0, int dest_x1, int dest_y1, const RF_Cell* attrib) {
    int pad_l = 0, pad_r = 0, pad_t = 0, pad_b = 0;
    int w, t, h;
//printf("CR src=%d,%d~%d,%d(%dx%d) dest=%d,%d~%d,%d(%dx%d) pad=%d,%d~%d,%d\n", src_x0,src_y0,src_x1,src_y1,src_x1-src_x0,src_y1-src_y0, dest_x0,dest_y0,dest_x1,dest_y1,dest_x1-dest_x0,dest_y1-dest_y0, pad_l,pad_t,pad_r,pad_b);
    if (!ctx || !ctx->screen) { return; }
    if (dest_x0 < 0) { src_x0 -= dest_x0; dest_x0 = 0; }
    if (dest_y0 < 0) { src_y0 -= dest_y0; dest_y0 = 0; }
    t = dest_x1 - ctx->screen_size.x;  if (t > 0) { src_x1 -= t; dest_x1 = ctx->screen_size.x; }
    t = dest_y1 - ctx->screen_size.y;  if (t > 0) { src_y1 -= t; dest_y1 = ctx->screen_size.y; }
    if (src_x0 < 0) { pad_l = -src_x0; dest_x0 -= src_x0; src_x0 = 0; }
    if (src_y0 < 0) { pad_t = -src_y0; dest_y0 -= src_y0; src_y0 = 0; }
    t = src_x1 - ctx->screen_size.x;  if (t > 0) { pad_r = t; dest_x1 -= t; src_x1 = ctx->screen_size.x; }
    t = src_y1 - ctx->screen_size.y;  if (t > 0) { pad_b = t; dest_y1 -= t; src_y1 = ctx->screen_size.y; }
    w = dest_x1 - dest_x0;
    h = dest_y1 - dest_y0;
    assert((src_x1 - src_x0) == w);
    assert((src_y1 - src_y0) == h);
    if ((w > 0) && (h > 0) && ((src_x0 != dest_x0) || (src_y0 != dest_y0))) {
        const RF_Cell* p_src  = &ctx->screen[src_x0  + src_y0  * ctx->screen_size.x];
              RF_Cell* p_dest = &ctx->screen[dest_x0 + dest_y0 * ctx->screen_size.x];
        int xdir = 1, stride = ctx->screen_size.x;
        if (src_x0 < dest_x0) { p_src +=  w - 1;           p_dest +=  w - 1;           xdir = -1; }
        if (src_y0 < dest_y0) { p_src += (h - 1) * stride; p_dest += (h - 1) * stride; stride = -stride; }
        stride -= xdir * w;
        for (int y = h;  y;  --y) {
            for (int x = w;  x;  --x) {
                *p_dest = *p_src;
                p_dest->dirty = 1;
                p_src  += xdir;
                p_dest += xdir;
            }
            p_src  += stride;
            p_dest += stride;
        }
    }
    if (pad_t) { RF_ClearRegionPS(ctx, dest_x0 - pad_l, dest_y0 - pad_t, pad_l + w + pad_r, pad_t, attrib); }
    if (pad_b) { RF_ClearRegionPS(ctx, dest_x0 - pad_l, dest_y1,         pad_l + w + pad_r, pad_b, attrib); }
    if (pad_l) { RF_ClearRegionPS(ctx, dest_x0 - pad_l, dest_y0, pad_l, h, attrib); }
    if (pad_r) { RF_ClearRegionPS(ctx, dest_x1,         dest_y0, pad_r, h, attrib); }
}
void RF_CopyRegionAB(RF_Context* ctx, int src_x0, int src_y0, int src_x1, int src_y1, int dest_x0, int dest_y0, const RF_Cell* attrib) {
    RF_CopyRegionInt(ctx, src_x0,src_y0, src_x1,src_y1, dest_x0,dest_y0, dest_x0+src_x1-src_x0,dest_y0+src_y1-src_y0, attrib);
}
void RF_CopyRegionPS(RF_Context* ctx, int src_x0, int src_y0, int dest_x0, int dest_y0, int w, int h, const RF_Cell* attrib) {
    RF_CopyRegionInt(ctx, src_x0,src_y0, src_x0+w,src_y0+h, dest_x0,dest_y0, dest_x0+w,dest_y0+h, attrib);
}

void RF_ScrollRegionAB(RF_Context* ctx, int x0, int y0, int x1, int y1, int dx, int dy, const RF_Cell* attrib) {
    int sx0 = x0, sy0 = y0, tx0 = x0, ty0 = y0, w = x1 - x0, h = y1 - y0;
//printf("SR %d,%d~%d,%d(%dx%d) by %d,%d\n", x0,y0, x1,y1, w,h, dx,dy);
    if (!ctx || !ctx->screen || (!dx && !dy) || (w <= 0) || (h <= 0)) { return; }
    if (dx > 0) { tx0 += dx; w -= dx; }
    if (dx < 0) { sx0 -= dx; w += dx; }
    if (dy > 0) { ty0 += dy; h -= dy; }
    if (dy < 0) { sy0 -= dy; h += dy; }
    RF_CopyRegionPS(ctx, sx0,sy0, tx0,ty0, w,h, attrib);
    RF_ClearRegionAB(ctx, x0,ty0,  tx0,ty0+h, attrib);  // left
    RF_ClearRegionAB(ctx, tx0+w,y0, x1,ty0+h, attrib);  // right
    RF_ClearRegionAB(ctx, x0,y0,    x1,ty0, attrib);  // top
    RF_ClearRegionAB(ctx, x0,ty0+h, x1,y1,  attrib);  // bottom
}
void RF_ScrollRegionPS(RF_Context* ctx, int x0, int y0, int w, int h, int dx, int dy, const RF_Cell* attrib) {
    RF_ScrollRegionAB(ctx, x0,y0, x0+w,y0+h, dx,dy, attrib);
}

///////////////////////////////////////////////////////////////////////////////

extern bool RF_ParseInternalMarkup(RF_Context* ctx, uint8_t c);
extern bool RF_ParseANSIMarkup(RF_Context* ctx, uint8_t c);

void RF_AddText(RF_Context* ctx, const char* str, const RF_Charset* charset, RF_MarkupType mt) {
    const uint32_t* charmap = charset ? charset->charmap : NULL;
    if (!ctx || !ctx->screen || !ctx->system || !str || !str[0]) { return; }
    if (mt == RF_MT_AUTO) { mt = RF_DetectMarkupType(str); }
    for (;;) {
        uint8_t c = (uint8_t) *str++;
        if (!c) { return; }  // end of string

        if (ctx->esc_count) {
            bool res;
            switch (mt) {
                case RF_MT_INTERNAL: res = RF_ParseInternalMarkup(ctx, c); break;
                case RF_MT_ANSI:     res = RF_ParseANSIMarkup(ctx, c);     break;
                default:             res = true;  // unknown markup type -> exit markup mode
            }
            if (res) {
                ctx->esc_count = 0;  // parser returned false -> end of sequence
            } else if (ctx->esc_count < 255) {
                ctx->esc_count++;  // otherwise, count escape byte
            }
            continue;
        }

        // process UTF-8 continuation byte
        if (ctx->utf8_cb_count) {
            if ((c & 0xC0) == 0x80) {
                ctx->num[0] = (ctx->num[0] << 6) | (c & 0x3F);
                ctx->utf8_cb_count--;
                if (!ctx->utf8_cb_count) {
                    // end of UTF-8 sequence
                    RF_AddChar(ctx, ctx->num[0]);
                }
                continue;
            } else {
                // invalid continuation byte -> emit error glyph and try parsing as normal byte
                RF_AddChar(ctx, 0xFFFD);
                ctx->utf8_cb_count = 0;
            }
        }

        // handle normal byte
        if (c == (uint8_t)mt) {
            // begin of escape sequence
            ctx->esc_count = 1;
        } else if ((c == 8) || (c == 9) || (c == 10) || (c == 13) || (c == 127)) {
            // pass control character through, regardless of character set
            RF_AddChar(ctx, c);
        } else if (charmap) {
            // translate character by character map
            RF_AddChar(ctx, charmap[c]);
        } else if (c < 0x80) {
            // normal 7-bit ASCII character
            RF_AddChar(ctx, c);
        } else if (c < 0xC0) {
            // unexpected UTF-8 continuation byte
            RF_AddChar(ctx, 0xFFFD);
        } else if (c < 0xE0) {
            // leading bytes for 2-byte UTF-8 sequence
            ctx->utf8_cb_count = 1;
            ctx->num[0] = c & 0x1F;
        } else if (c < 0xF0) {
            // leading bytes for 3-byte UTF-8 sequence
            ctx->utf8_cb_count = 2;
            ctx->num[0] = c & 0x0F;
        } else if (c < 0xF8) {
            // leading bytes for 4-byte UTF-8 sequence
            ctx->utf8_cb_count = 3;
            ctx->num[0] = c & 0x07;
        } else {
            // invalid UTF-8 byte
            RF_AddChar(ctx, 0xFFFD);
        }
    }
}

void RF_ResetParser(RF_Context* ctx) {
    if (!ctx) { return; }
    ctx->utf8_cb_count = ctx->esc_count = ctx->esc_remain = 0;
    memset((void*)ctx->num, 0, sizeof(ctx->num));
    ctx->num_idx = ctx->esc_type = 0;
    ctx->esc_class = NULL;
}

///////////////////////////////////////////////////////////////////////////////

uint32_t RF_MapRGBToStandardColor(uint32_t color, uint8_t bright_threshold) {
    if (!RF_IS_RGB_COLOR(color)) { return color; }
    uint8_t r = RF_COLOR_R(color);
    uint8_t g = RF_COLOR_G(color);
    uint8_t b = RF_COLOR_B(color);
    uint8_t max = (r > g) ? r : g;
    if (b > max) { max = b; }
    if (max < 72) { max = 72; }  // make very dark colors register as black
    color = RF_COLOR_BLACK;
    if (max > bright_threshold) { color |= RF_COLOR_BRIGHT; }
    max >>= 1;
    if (r >= max) { color |= RF_COLOR_RED;   }
    if (g >= max) { color |= RF_COLOR_GREEN; }
    if (b >= max) { color |= RF_COLOR_BLUE;  }
    return color;
}

uint32_t RF_MapStandardColorToRGB(uint32_t color, uint8_t std0, uint8_t std1, uint8_t bright0, uint8_t bright1) {
    switch (color) {
        case RF_COLOR_BLACK:                     return RF_COLOR_RGB(   std0,    std0,    std0);
        case RF_COLOR_BLUE:                      return RF_COLOR_RGB(   std0,    std0,    std1);
        case RF_COLOR_GREEN:                     return RF_COLOR_RGB(   std0,    std1,    std0);
        case RF_COLOR_CYAN:                      return RF_COLOR_RGB(   std0,    std1,    std1);
        case RF_COLOR_RED:                       return RF_COLOR_RGB(   std1,    std0,    std0);
        case RF_COLOR_MAGENTA:                   return RF_COLOR_RGB(   std1,    std0,    std1);
        case RF_COLOR_YELLOW:                    return RF_COLOR_RGB(   std1,    std1,    std0);
        case RF_COLOR_WHITE:                     return RF_COLOR_RGB(   std1,    std1,    std1);
        case RF_COLOR_BLACK   | RF_COLOR_BRIGHT: return RF_COLOR_RGB(bright0, bright0, bright0);
        case RF_COLOR_BLUE    | RF_COLOR_BRIGHT: return RF_COLOR_RGB(bright0, bright0, bright1);
        case RF_COLOR_GREEN   | RF_COLOR_BRIGHT: return RF_COLOR_RGB(bright0, bright1, bright0);
        case RF_COLOR_CYAN    | RF_COLOR_BRIGHT: return RF_COLOR_RGB(bright0, bright1, bright1);
        case RF_COLOR_RED     | RF_COLOR_BRIGHT: return RF_COLOR_RGB(bright1, bright0, bright0);
        case RF_COLOR_MAGENTA | RF_COLOR_BRIGHT: return RF_COLOR_RGB(bright1, bright0, bright1);
        case RF_COLOR_YELLOW  | RF_COLOR_BRIGHT: return RF_COLOR_RGB(bright1, bright1, bright0);
        case RF_COLOR_WHITE   | RF_COLOR_BRIGHT: return RF_COLOR_RGB(bright1, bright1, bright1);
        default: return color;
    }
}

void RF_InvalidatePalette(RF_Context* ctx) {
    if (ctx) {
        memset(ctx->pal_cache, 0xFF, sizeof(ctx->pal_cache));
    }
}

uint32_t RF_PaletteLookup(RF_Context* ctx, const uint32_t* pal, uint32_t pal_size, uint32_t color) {
    uint32_t best_index = 0;
    uint32_t best_dist = 0x3FFFF;
    uint32_t cache_idx = 0;
    uint32_t mask = 0xFFFFFF;
    if (!pal || !pal_size) { return 0; }

    // cache initialization and check
    if (ctx) {
        cache_idx = ((RF_COLOR_R(color) >> (8 - RF_PAL_CACHE_BITS)) << (RF_PAL_CACHE_BITS * 2))
                  | ((RF_COLOR_G(color) >> (8 - RF_PAL_CACHE_BITS)) <<  RF_PAL_CACHE_BITS)
                  |  (RF_COLOR_B(color) >> (8 - RF_PAL_CACHE_BITS));
        best_index = ctx->pal_cache[cache_idx];
        if (best_index < 0xFF) { return best_index; }
        mask = (0xFF << (8 - RF_PAL_CACHE_BITS)) & 0xFF;
        mask |= (mask << 8) | (mask << 16);
        color &= mask;
    }

    // main search loop
    for (uint32_t index = 0;  index < pal_size;  ++index) {
        uint32_t check = pal[index] & mask;
        int32_t cdist = (check & 0xFF) - (color & 0xFF);
        uint32_t dist = (uint32_t)(cdist * cdist);
        cdist = ((check >> 8) & 0xFF) - ((color >> 8) & 0xFF);
        dist += (uint32_t)(cdist * cdist);
        cdist = ((check >> 16) & 0xFF) - ((color >> 16) & 0xFF);
        dist += (uint32_t)(cdist * cdist);
        if (dist < best_dist) {
            best_dist = dist;
            best_index = index;
        }
    }

    // cache finalization
    if (ctx && (best_index < 0xFF)) {
        ctx->pal_cache[cache_idx] = (uint8_t)best_index;
    }
    return best_index;
}
