#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "retrofont.h"


int main(int argc, char* argv[]) {
    srand(0x13375EED);

    uint32_t sys_id = 0;
    uint32_t font_id = 0;
    if ((argc > 1) && (strlen(argv[1]) == 4)) {  sys_id = RF_MAKE_ID_S(argv[1]); }
    if ((argc > 2) && (strlen(argv[2]) == 4)) { font_id = RF_MAKE_ID_S(argv[2]); }

    RF_Context* ctx = RF_CreateContext(sys_id);
    if (!ctx) {
        fprintf(stderr, "ERROR: no such system (sys_id=0x%08X)!\n", sys_id);
        return 1;
    }
    if (font_id) {
        if (!RF_SetFont(ctx, font_id)) {
            fprintf(stderr, "ERROR: no such font (font_id=0x%08X) or incompatible size!\n", font_id);
            return 1;
        }
    }
    RF_ResizeScreen(ctx, 0, 0, true);
    printf("sytem: %s [%dx%d]\n", ctx->system->name, ctx->screen_size.x, ctx->screen_size.y);
    printf("font: %s [%dx%d]\n", ctx->font->name, ctx->font->font_size.x, ctx->font->font_size.y);

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
            ++c;
        }
    }
    //RF_SetBackgroundColor(ctx, 0x123456);
    RF_Invalidate(ctx, true);

    RF_Render(ctx, 0);

    FILE *f = fopen("rftest.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", ctx->bitmap_size.x, ctx->bitmap_size.y);
    fwrite((void*)ctx->bitmap, ctx->stride, ctx->bitmap_size.y, f);
    fclose(f);

    RF_FreeContext(ctx);
    return 0;
}
