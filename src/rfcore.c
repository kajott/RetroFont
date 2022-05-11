#include <stdio.h>  // DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "retrofont.h"

RF_Context* RF_CreateContext(uint32_t sys_id) {
    RF_Context* ctx = (RF_Context*) calloc(1, sizeof(RF_Context));
    if (!ctx) { return NULL; }
    ctx->system = RF_SystemList[0];
//printf("searching for sys_id 0x%08X\n", sys_id);
    for (const RF_System** p_sys = RF_SystemList;  *p_sys;  ++p_sys) {
//printf("                  == 0x%08X? -> %s\n", (*p_sys)->sys_id, ((*p_sys)->sys_id == sys_id) ? "YES" : "no");
        if ((*p_sys)->sys_id == sys_id) {
            ctx->system = *p_sys;
            break;
        }
    }
    if (!ctx->system || !RF_SetFont(ctx, 0)) { free((void*)ctx); return NULL; }
    ctx->default_fg = ctx->default_bg = ctx->border_color = RF_COLOR_DEFAULT;
    return ctx;
}

bool RF_SetFont(RF_Context* ctx, uint32_t font_id) {
    bool result = false;
    if (!ctx || !ctx->system) { return false; }
    if (!font_id) { font_id = ctx->system->default_font_id; }
//printf("searching for font_id 0x%08X (%dx%d)\n", font_id, ctx->system->font_size.x, ctx->system->font_size.y);
    for (const RF_Font* font = RF_FontList;  font->font_id;  ++font) {
//printf("                   == 0x%08X? (%dx%d) -> ", font->font_id, font->font_size.x, font->font_size.y);
        if ((ctx->system->font_size.x && (font->font_size.x != ctx->system->font_size.x))
        ||  (ctx->system->font_size.y && (font->font_size.y != ctx->system->font_size.y))) {
//printf("size mismatch\n");
            continue;  // font size doesn't match with current system
        }
//printf("%s\n", (font->font_id == font_id) ? "YES" : "no");
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
    static const RF_Cell empty_cell = { 32, 1, 0,0,0,0,0,0, RF_COLOR_DEFAULT, RF_COLOR_DEFAULT };
    RF_Cell *new_screen, *c;
    RF_Coord bmpsize;
    uint8_t *new_bmp;

    if (!ctx || !ctx->system || (!ctx->system->font_size.x && !ctx->font)) { return false; }
    if (!new_width)  { new_width  = ctx->system->default_screen_size.x; }
    if (!new_height) { new_height = ctx->system->default_screen_size.y; }
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
        const RF_Cell* r = ctx->screen ? &ctx->screen[ctx->screen_size.x * y] : &empty_cell;
        for (uint16_t x = 0;  x < new_width;  ++x) {
            *c = *r;
            if ((x >= ctx->screen_size.x) || (y >= ctx->screen_size.y)) { c->codepoint = 32; }
            c->dirty = 1;
            ++c;
            if ((x + 1) < ctx->screen_size.x) { ++r; }
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
    bool allow_underline
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
        p = &cmd->pixel[cmd->stride * y];
        bits = 0;
        for (uint16_t x = offset_x;  x;  --x) {
            PUT_PIXEL(p, line_row ? fg : bg);
        }
        for (uint16_t x = 0;  x < (cmd->cell_size.x - offset_x);  ++x) {
            if (core_row && (x < cmd->font_size.x) && !(x & 7)) { bits = *g++; }
            color = (line_row || (bits & 1)) ? fg : bg;
            PUT_PIXEL(p, color);
            bits >>= 1;
        }
    }
}

void RF_DestroyContext(RF_Context* ctx) {
    if (!ctx) { return; }
    free((void*)ctx->screen);  ctx->screen = NULL;
    free((void*)ctx->bitmap);  ctx->bitmap = NULL;
    free((void*)ctx);
}

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
