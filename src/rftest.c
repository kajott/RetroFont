#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "retrofont.h"

int main(int argc, char* argv[]) {
    srand(0x13375EED);

    uint32_t sys_id = 0;
    uint32_t font_id = 0;
    if ((argc > 1) && (strlen(argv[1]) == 4)) {  sys_id = RF_MAKE_ID(argv[1]); }
    if ((argc > 2) && (strlen(argv[2]) == 4)) { font_id = RF_MAKE_ID(argv[2]); }

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

    RF_Cell *c = ctx->screen;
    for (size_t i = ctx->screen_size.x * ctx->screen_size.y;  i;  --i) {
        c->fg = RF_COLOR_BLACK | (rand() & 15);
        do { c->bg = RF_COLOR_BLACK | (rand() & 15); } while (c->bg == c->fg);
        uint16_t r = (uint16_t) rand();
        c->bold      = r >> 0;
        c->dim       = r >> 1;
        c->underline = r >> 2;
        c->blink     = r >> 3;
        c->reverse   = r >> 4;
        c->invisible = (r & 0x3F00) ? 0 : 1;  // make this very rare
        c->codepoint = 32 + (rand() % 95);
        ++c;
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
