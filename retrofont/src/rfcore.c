#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
            if ((*p_sys)->font_size.x && (*p_sys)->font_size.y) {
                uint32_t sx = (*p_sys)->border_ul.x + (uint32_t)((*p_sys)->cell_size.x) * (uint32_t)((*p_sys)->default_screen_size.x) + (*p_sys)->border_lr.x;
                uint32_t sy = (*p_sys)->border_ul.y + (uint32_t)((*p_sys)->cell_size.y) * (uint32_t)((*p_sys)->default_screen_size.y) + (*p_sys)->border_lr.y;
                ctx->pixel_aspect = (4.0f * (float)sy) / (3.0f * (float)sx);
            }
            return true;
        }
    }
    return false;
}

bool RF_SetFont(RF_Context* ctx, uint32_t font_id) {
    bool result = false;
    if (!ctx || !ctx->system) { return false; }
    if (!font_id) { font_id = ctx->system->default_font_id; }
    for (const RF_Font* font = RF_FontList;  font->font_id;  ++font) {
        if ((ctx->system->font_size.x && (font->font_size.x != ctx->system->font_size.x))
        ||  (ctx->system->font_size.y && (font->font_size.y != ctx->system->font_size.y))) {
            continue;  // font size doesn't match with current system
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
        RF_Invalidate(ctx, false);
        memset((void*)ctx->glyph_offset_cache, 0xFF, sizeof(ctx->glyph_offset_cache));
    }
    return result;
}

bool RF_ResizeScreen(RF_Context* ctx, uint16_t new_width, uint16_t new_height, bool with_border) {
    RF_Cell *new_screen, *c;
    RF_Coord bmpsize;
    uint8_t *new_bmp;

    if (!ctx || !ctx->system || (!ctx->system->font_size.x && !ctx->font)) { return false; }
    if (!new_width)  { new_width  = ctx->screen_size.x; }
    if (!new_height) { new_height = ctx->screen_size.y; }
    if (!new_width  || (new_width  == RF_SIZE_DEFAULT)) { new_width  = ctx->system->default_screen_size.x; }
    if (!new_height || (new_height == RF_SIZE_DEFAULT)) { new_height = ctx->system->default_screen_size.y; }
    c = new_screen = (RF_Cell*) malloc(sizeof(RF_Cell) * new_width * new_height);
    if (!new_screen) { return false; }

    bmpsize.x = new_width  * (ctx->system->cell_size.x + (ctx->system->font_size.x ? 0 : ctx->font->font_size.x));
    bmpsize.y = new_height * (ctx->system->cell_size.y + (ctx->system->font_size.y ? 0 : ctx->font->font_size.y));
    if (with_border) {
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
        ctx->main_lr.x = bmpsize.x - ctx->system->border_lr.x;
        ctx->main_lr.y = bmpsize.y - ctx->system->border_lr.y;
    } else {
        ctx->main_ul.x = ctx->main_ul.y = 0;
        ctx->main_lr = bmpsize;
    }
    ctx->border_color_changed = true;
    return true;
}

void RF_MoveCursor(RF_Context* ctx, uint16_t new_col, uint16_t new_row) {
    if (!ctx || !ctx->screen) { return; }
    ctx->screen[ctx->screen_size.x * ctx->cursor_pos.y + ctx->cursor_pos.x].dirty = 1;
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
    *p++ = RF_COLOR_R(color); \
    *p++ = RF_COLOR_G(color); \
    *p++ = RF_COLOR_B(color); \
} while (0)

static uint8_t* fill_border(uint8_t* p, uint32_t color, size_t count) {
    while (count--) { PUT_PIXEL(p, color); }
    return p;
}

bool RF_Render(RF_Context* ctx, uint32_t time_msec) {
    bool result = false;
    uint16_t csx, csy;
    RF_RenderCommand cmd;
    if (!ctx || !ctx->system || !ctx->font || !ctx->screen || !ctx->bitmap) { return false; }
    csx = ctx->system->cell_size.x + (ctx->system->font_size.x ? 0 : ctx->font->font_size.x);
    csy = ctx->system->cell_size.y + (ctx->system->font_size.y ? 0 : ctx->font->font_size.y);
    if (ctx->border_color_changed) {
        uint32_t color = ctx->system->cls->map_border_color(ctx->system->sys_id, ctx->border_color);
        if (ctx->has_border) {
            uint8_t* p = fill_border(ctx->bitmap, color, ctx->bitmap_size.x * ctx->system->border_ul.y + ctx->system->border_ul.x);
            for (uint16_t y = ctx->screen_size.y * csy;  y;  --y) {
                p += ctx->screen_size.x * csx * 3;
                if (y > 1) { p = fill_border(p, color, ctx->system->border_lr.x + ctx->system->border_ul.x); }
            }
            fill_border(p, color, ctx->system->border_lr.x + ctx->bitmap_size.x * ctx->system->border_lr.y);
        }
        result = true;
        ctx->border_rgb = color;
        ctx->border_color_changed = false;
    }
    cmd.cell = ctx->screen;
    cmd.stride = ctx->stride;
    cmd.sys_id = ctx->system->sys_id;
    cmd.cell_size.x = csx;
    cmd.cell_size.y = csy;
    cmd.font_size = ctx->font->font_size;
    cmd.underline_row = ctx->font->underline_row;
    cmd.default_fg = ctx->default_fg;
    cmd.default_bg = ctx->default_bg;
    cmd.blink_phase = ctx->system->blink_interval_msec ? (((time_msec / ctx->system->blink_interval_msec) & 1) != 0) : false;
    for (uint16_t y = 0;  y < ctx->screen_size.y;  ++y) {
        cmd.pixel = &ctx->bitmap[((ctx->has_border ? ctx->system->border_ul.y : 0) + y * csy) * ctx->stride
                                + (ctx->has_border ? ctx->system->border_ul.x : 0) * 3];
        for (uint16_t x = 0;  x < ctx->screen_size.x;  ++x) {
            cmd.is_cursor = (y == ctx->cursor_pos.y) && (x == ctx->cursor_pos.x);
            if (cmd.cell->dirty || ((cmd.blink_phase != ctx->last_blink_phase) && (cmd.cell->blink || cmd.is_cursor))) {
                // try to retrieve the offset from the cache
                #define CACHE_INVALID ((uint32_t)(-1))
                uint32_t cache_index = cmd.cell->codepoint - RF_GLYPH_CACHE_MIN;
                uint32_t offset = (cache_index < RF_GLYPH_CACHE_SIZE) ? ctx->glyph_offset_cache[cache_index] : CACHE_INVALID;
                if (offset == CACHE_INVALID) {
                    // not cached (or not cacheable) -> binary search for the correct glyph
                    uint32_t a = 0, b = ctx->font->glyph_count - 1;
                    while (b > a) {
//printf("U+%04X (#%d) <= U+%04X <= U+%04X (#%d)\n", ctx->font->glyph_map[a].codepoint, a, cmd.cell->codepoint, ctx->font->glyph_map[b].codepoint, b);
                        uint32_t c = (a + b + 1) >> 1;
                        if (cmd.cell->codepoint < ctx->font->glyph_map[c].codepoint)
                            { b = c - 1; } else { a = c; }
                    }
                    offset = (ctx->font->glyph_map[a].codepoint == cmd.cell->codepoint)
                        ?  ctx->font->glyph_map[a].bitmap_offset
                        :  ctx->font->fallback_offset;
//printf("DONE\n");
                    if (cache_index < RF_GLYPH_CACHE_SIZE) {
                        // store in cache
                        ctx->glyph_offset_cache[cache_index] = offset;
                    }
                }

                // render the glyph
                cmd.glyph_data = &RF_GlyphBitmaps[offset];
                ctx->system->cls->render_cell(&cmd);
                cmd.cell->dirty = 0;
                result = true;
            }
            ++cmd.cell;
            cmd.pixel += csx * 3;
        }
    }
    ctx->last_blink_phase = cmd.blink_phase;
    return result;
}

void RF_RenderCell(
    const RF_RenderCommand* cmd,
    uint32_t fg,         uint32_t bg,
    uint16_t offset_x,   uint16_t offset_y,
    uint16_t line_start, uint16_t line_end,
    bool extra_reverse,
    bool force_invisible,
    bool allow_underline,
    bool allow_bold
) {
    uint8_t *p, bits = 0;
    const uint8_t *g;
    uint32_t color;
    if (!cmd || !cmd->cell || !cmd->pixel || !cmd->glyph_data) { return; }
    if (cmd->cell->reverse) { extra_reverse = !extra_reverse; }
    if (extra_reverse) { uint32_t t = fg;  fg = bg;  bg = t; }
    if (cmd->cell->invisible || force_invisible) { fg = bg; }
    if (line_start >= line_end) { line_start = cmd->cell_size.y; line_end = 0; }
    if (allow_underline && cmd->underline_row && cmd->cell->underline) {
        if (line_start >  cmd->underline_row)      { line_start = cmd->underline_row; }
        if (line_end   < (cmd->underline_row + 1)) { line_end   = cmd->underline_row + 1; }
    }
    g = cmd->glyph_data;
    for (uint16_t y = 0;  y < cmd->cell_size.y;  ++y) {
        bool core_row = (y >= offset_y) && (y < (offset_y + cmd->font_size.y));
        bool line_row = (y >= line_start) && (y < line_end);
        bool prev = false;
        p = &cmd->pixel[cmd->stride * y];
        bits = 0;
        for (uint16_t x = offset_x;  x;  --x) {
            PUT_PIXEL(p, line_row ? fg : bg);
        }
        for (uint16_t x = 0;  x < (cmd->cell_size.x - offset_x);  ++x) {
            if (core_row && (x < cmd->font_size.x) && !(x & 7)) { bits = *g++; }
            color = (line_row || (bits & 1) || prev) ? fg : bg;
            PUT_PIXEL(p, color);
            if (cmd->cell->bold && allow_bold) { prev = (bits & 1); }
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
        0x1FB70, 0x1FB90,                // (some) Symbols for Legacy Computing
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
            if (y >= attribute_start_row) {
                uint16_t r = (uint16_t) rand();
                c->fg = RF_COLOR_BLACK | (rand() & 15);
                do { c->bg = RF_COLOR_BLACK | (rand() & 15); } while (c->bg == c->fg);
                c->bold      = r >> 0;
                c->dim       = r >> 1;
                c->underline = r >> 2;
                c->blink     = r >> 3;
                c->reverse   = r >> 4;
                c->invisible = (r & 0x3F00) ? 0 : 1;  // make this very rare
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
            c->codepoint = (x & 31) + cp_offsets[y % row_mod];
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
                RF_ScrollRegion(ctx, 0, 0, 0, 0, -1, &ctx->attrib);
            } else {
                RF_ScrollRegion(ctx, 0, ctx->cursor_pos.y, ctx->screen_size.x, ctx->screen_size.y, 1, &ctx->attrib);
            }
            // copy remainder of current line onto next one
            pos = &ctx->screen[ctx->cursor_pos.y * ctx->screen_size.x];
            for (uint16_t i = ctx->screen_size.x - ctx->cursor_pos.x;  i;  --i) {
                *pos = pos[(int)ctx->cursor_pos.x - (int)ctx->screen_size.x];
                pos->dirty = 1;
                ++pos;
            }
            // clear what remains
            RF_ClearRegion(ctx, ctx->cursor_pos.x, ctx->cursor_pos.y - 1, ctx->screen_size.x, ctx->cursor_pos.y, &ctx->attrib);
            RF_ClearRegion(ctx, ctx->screen_size.x - ctx->cursor_pos.x, ctx->cursor_pos.y, ctx->screen_size.x, ctx->cursor_pos.y + 1, &ctx->attrib);
            ctx->cursor_pos.x = 0;
        } else {
            // clear the remainder of the current line
            RF_ClearRegion(ctx, ctx->cursor_pos.x, ctx->cursor_pos.y, ctx->screen_size.x, ctx->cursor_pos.y + 1, &ctx->attrib);
            ctx->cursor_pos.x = 0;
            if ((++(ctx->cursor_pos.y)) >= ctx->screen_size.y) {
                ctx->cursor_pos.y = ctx->screen_size.y - 1;
            }
        }
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
                    RF_ScrollRegion(ctx, 0, 0, 0, 0, -1, &ctx->attrib);
                } else {
                    RF_ScrollRegion(ctx, 0, ctx->cursor_pos.y, ctx->screen_size.x, ctx->screen_size.y, 1, &ctx->attrib);
                }
            } else {
                if ((++(ctx->cursor_pos.y)) >= ctx->screen_size.y) {
                    ctx->cursor_pos.y = 0;
                }
            }
        }
    }
    // regardless of what just happened, mark the new cursor position as dirty
    if ((ctx->cursor_pos.x < ctx->screen_size.x) && (ctx->cursor_pos.y < ctx->screen_size.y)) {
        ctx->screen[ctx->cursor_pos.y * ctx->screen_size.x + ctx->cursor_pos.x].dirty = 1;
    }
}

void RF_ClearRegion(RF_Context* ctx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const RF_Cell* attrib) {
    RF_Cell* c;
    if (!x0 && !x1 && !y0 && !y1) { x1 = y1 = (uint16_t)(-1); }
    if (!ctx || !ctx->screen
    || (x0 >= ctx->screen_size.x) || (x1 <= x0)
    || (y0 >= ctx->screen_size.y) || (y1 <= y0))
        { return; }
    if (x1 > ctx->screen_size.x) { x1 = ctx->screen_size.x; }
    if (y1 > ctx->screen_size.y) { y1 = ctx->screen_size.y; }
    if (!attrib) { attrib = &RF_EmptyCell; }
    c = &ctx->screen[x0 + y0 * ctx->screen_size.x];
    x1 -= x0;
    for (uint16_t y = y0;  y < y1;  ++y) {
        for (uint16_t x = x1;  x;  --x) {
            *c = *attrib;
            c->codepoint = 32;
            c->dirty = 1;
            ++c;
        }
        c += ctx->screen_size.x - x1 + x0;
    }
}

void RF_ScrollRegion(RF_Context* ctx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int16_t dy, const RF_Cell* attrib) {
    uint16_t height, amount, stride, dx;
    RF_Cell *dest;
    const RF_Cell *src;
    if (!x0 && !x1 && !y0 && !y1) { x1 = y1 = (uint16_t)(-1); }
    if (!ctx || !ctx->screen || !dy
    || (x0 >= ctx->screen_size.x) || (x1 <= x0)
    || (y0 >= ctx->screen_size.y) || (y1 <= y0))
        { return; }
    if (x1 > ctx->screen_size.x) { x1 = ctx->screen_size.x; }
    if (y1 > ctx->screen_size.y) { y1 = ctx->screen_size.y; }
    height = y1 - y0;
    amount = (uint16_t)((dy < 0) ? -dy : dy);
    if (amount > height) { amount = height; }
    stride = ctx->screen_size.x - x1 + x0;
    dx = x1 - x0;
    if (dy > 0) {
        // scroll down
        dest = &ctx->screen[(x1 - 1) + (y1 - 1) * ctx->screen_size.x];
        src = &dest[-(int)(amount * ctx->screen_size.x)];
        for (uint16_t y = height - amount;  y;  --y) {
            for (uint16_t x = dx;  x;  --x) {
                *dest = *src--;
                dest->dirty = 1;
                dest--;
            }
            dest -= stride;
            src -= stride;
        }
        RF_ClearRegion(ctx, x0, y0, x1, y0 + amount, attrib ? attrib : &ctx->screen[x0 + y0 * ctx->screen_size.x]);
    } else {
        // scroll up
        dest = &ctx->screen[x0 + y0 * ctx->screen_size.x];
        src = &dest[amount * ctx->screen_size.x];
        for (uint16_t y = height - amount;  y;  --y) {
            for (uint16_t x = dx;  x;  --x) {
                *dest = *src++;
                dest->dirty = 1;
                dest++;
            }
            dest += stride;
            src += stride;
        }
        RF_ClearRegion(ctx, x0, y1 - amount, x1, y1, attrib ? attrib : &ctx->screen[(x1 - 1) + (y1 - amount) * ctx->screen_size.x]);
    }
}

///////////////////////////////////////////////////////////////////////////////

uint32_t RF_MapRGBToStandardColor(uint32_t color, uint8_t bright_threshold) {
    if (!RF_IS_RGB_COLOR(color)) { return color; }
    uint8_t r = RF_COLOR_R(color);
    uint8_t g = RF_COLOR_G(color);
    uint8_t b = RF_COLOR_B(color);
    uint8_t max = (r > g) ? r : g;
    if (b > max) { max = b; }
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
