#include <stdio.h>
#include <stdlib.h>

#include "retrofont.h"

int main(void) {
    srand(0x13375EED);

    RF_Context* ctx = RF_CreateContext(RF_MAKE_ID("ZX82"));
    RF_SetFont(ctx, RF_MAKE_ID("ZX80"));
    RF_ResizeScreen(ctx, 0, 0, true);

    RF_Cell *c = ctx->screen;
    for (size_t i = ctx->screen_size.x * ctx->screen_size.y;  i;  --i) {
        c->fg = RF_COLOR_BLACK | (rand() & 15);
        do {
            c->bg = RF_COLOR_BLACK | (rand() & 15);
        } while (c->bg == c->fg);
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
    RF_Invalidate(ctx);

    RF_Render(ctx, 0);

    FILE *f = fopen("rftest.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", ctx->bitmap_size.x, ctx->bitmap_size.y);
    fwrite((void*)ctx->bitmap, ctx->stride, ctx->bitmap_size.y, f);
    fclose(f);

    RF_FreeContext(ctx);
    return 0;
}
