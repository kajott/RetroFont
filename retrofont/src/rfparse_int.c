#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "retrofont.h"

typedef struct s_RF_InternalMarkupCommand {
    uint8_t letter;  //!< first character of the command
    bool is_color;   //!< enable special processing for color values
    uint8_t digits;  //!< number of digits to follow
    uint8_t base;    //!< numberic base of the digits (10 or 16; 0 = use ASCII code directly)
    void (*finalize_func) (RF_Context*);  //!< function to call at the end of the sequence
} RF_InternalMarkupCommand;

static void cmd_setxpos(RF_Context* ctx) {
    RF_MoveCursor(ctx, (uint16_t)ctx->num[0], ctx->cursor_pos.y);
}
static void cmd_setxneg(RF_Context* ctx) {
    int x = (int)ctx->screen_size.x - (int)ctx->num[0];
    RF_MoveCursor(ctx, (x > 0) ? (uint16_t)x : 0, ctx->cursor_pos.y);
}
static void cmd_centerx(RF_Context* ctx) {
    int x = ((int)ctx->screen_size.x - (int)ctx->num[0]) >> 1;
    RF_MoveCursor(ctx, (x > 0) ? (uint16_t)x : 0, ctx->cursor_pos.y);
}
static void cmd_setypos(RF_Context* ctx) {
    RF_MoveCursor(ctx, ctx->cursor_pos.x, (uint16_t)ctx->num[0]);
}
static void cmd_setyneg(RF_Context* ctx) {
    int y = (int)ctx->screen_size.y - (int)ctx->num[0];
    RF_MoveCursor(ctx, ctx->cursor_pos.x, (y > 0) ? (uint16_t)y : 0);
}
static void cmd_centery(RF_Context* ctx) {
    int y = ((int)ctx->screen_size.y - (int)ctx->num[0]) >> 1;
    RF_MoveCursor(ctx, ctx->cursor_pos.x, (y > 0) ? (uint16_t)y : 0);
}

static void cmd_setfg(RF_Context* ctx) {
    ctx->attrib.fg = ctx->num[0];
}
static void cmd_setbg(RF_Context* ctx) {
    ctx->attrib.bg = ctx->num[0];
}
static void cmd_setdfg(RF_Context* ctx) {
    RF_SetForegroundColor(ctx, ctx->num[0]);
}
static void cmd_setdbg(RF_Context* ctx) {
    RF_SetBackgroundColor(ctx, ctx->num[0]);
}
static void cmd_setborder(RF_Context* ctx) {
    RF_SetBorderColor(ctx, ctx->num[0]);
}

static void cmd_reset(RF_Context* ctx) {
    ctx->attrib = RF_EmptyCell;
}

static void cmd_attrib(RF_Context* ctx) {
    uint32_t value = (((const RF_InternalMarkupCommand*)(ctx->esc_class))->letter == '+') ? 1 : 0;
    switch (ctx->num[0]) {
        case 'B': case 'b': ctx->attrib.bold      = value; break;
        case 'D': case 'd': ctx->attrib.dim       = value; break;
        case 'U': case 'u': ctx->attrib.underline = value; break;
        case 'F': case 'f': ctx->attrib.blink     = value; break;
        case 'R': case 'r': ctx->attrib.reverse   = value; break;
        case 'I': case 'i': ctx->attrib.invisible = value; break;
        default:  // invalid attribute
            RF_AddChar(ctx, 0xFFFD);
            break;
    }
}

static void cmd_clreol(RF_Context* ctx) {
    RF_Cell* c;
    uint16_t i;
    if ((ctx->cursor_pos.x >= ctx->screen_size.x) || (ctx->cursor_pos.y >= ctx->screen_size.y)) { return; }
    ctx->attrib.codepoint = 32;
    ctx->attrib.dirty = 1;
    c = &ctx->screen[ctx->cursor_pos.y * ctx->screen_size.x + ctx->cursor_pos.y];
    for (i = ctx->screen_size.x - ctx->cursor_pos.x;  i;  --i) {
        *c++ = ctx->attrib;
    }
}

static void cmd_clrscr(RF_Context* ctx) {
    RF_Cell* c;
    uint32_t i;
    ctx->attrib.codepoint = 32;
    ctx->attrib.dirty = 1;
    c = ctx->screen;
    for (i = (uint32_t)ctx->screen_size.x * (uint32_t) ctx->screen_size.y;  i;  --i) {
        *c++ = ctx->attrib;
    }
}

static void cmd_unicode(RF_Context* ctx) {
    RF_AddChar(ctx, ctx->num[0]);
}

const RF_InternalMarkupCommand RF_InternalMarkupCommands[] = {
    { 'x', false, 2, 10, cmd_setxpos },
    { 'X', false, 2, 10, cmd_setxneg },
    { 'c', false, 2, 10, cmd_centerx },
    { 'y', false, 2, 10, cmd_setypos },
    { 'Y', false, 2, 10, cmd_setyneg },
    { 'C', false, 2, 10, cmd_centery },
    { 'f', true,  1, 16, cmd_setfg },
    { 'b', true,  1, 16, cmd_setbg },
    { 'F', true,  1, 16, cmd_setdfg },
    { 'B', true,  1, 16, cmd_setdbg },
    { 'r', true,  1, 16, cmd_setborder },
    { '-', false, 1,  0, cmd_attrib },
    { '+', false, 1,  0, cmd_attrib },
    { '0', false, 0,  0, cmd_reset },
    { 'z', false, 0,  0, cmd_clreol },
    { 'Z', false, 0,  0, cmd_clrscr },
    { 'u', false, 4, 16, cmd_unicode },
    { 'U', false, 6, 16, cmd_unicode },
    { 0 }
};

bool RF_ParseInternalMarkup(RF_Context* ctx, uint8_t c) {
    const RF_InternalMarkupCommand* cmd;

    // first byte of sequence?
    if (ctx->esc_count == 1) {
        if (c == '`') {
            // escape for backtick character
            RF_AddChar(ctx, c);
            return true;
        }
        // find command
        for (cmd = RF_InternalMarkupCommands;  cmd->letter && (cmd->letter != c);  ++cmd);
        if (!cmd->letter) {
            // invalid sequence
            RF_AddChar(ctx, 0xFFFD);
            return true;
        }
        // set up parser
        ctx->esc_class = (const void*)cmd;
        ctx->esc_remain = cmd->digits;
        ctx->num[0] = 0;
        if (cmd->digits) {
            return false;
        }
    } else {
        // not first byte -> load command from context
        cmd = (const RF_InternalMarkupCommand*) ctx->esc_class;
    }

    // handle special color syntax in second byte of sequence
    if (cmd->is_color && (ctx->esc_count == 2)) {
        if (c == '-') {
            // handle default color
            ctx->num[0] = RF_COLOR_DEFAULT;
            cmd->finalize_func(ctx);
            return true;
        } else if (c == '#') {
            // switch to hexadecimal RGB mode
            ctx->esc_remain = 6;
            return false;
        }
    }

    // parse digit
    if (ctx->esc_remain) {
        if (cmd->base) {
            if      ((c >= '0') && (c <= '9')) { c -= '0'; }
            else if ((c >= 'A') && (c <= 'Z')) { c -= 'A' - 10; }
            else if ((c >= 'a') && (c <= 'z')) { c -= 'a' - 10; }
            else                               { c = 0xFF; /* force invalid value */ }
            if (c >= cmd->base) { RF_AddChar(ctx, 0xFFFD); return true; }
            ctx->num[0] = (ctx->num[0] * cmd->base) + c;
        } else {
            ctx->num[0] = (ctx->num[0] << 8) | c;
        }
        ctx->esc_remain--;
    }

    // end of sequence?
    if (ctx->esc_remain) {
        // no, more bytes follow
        return false;
    }

    // if the value is a color, and it was just a single digit (i.e. no
    // hexadecimal RGB value), convert it into a standard16 color code
    if (cmd->is_color && (ctx->esc_count == 2)) {
        ctx->num[0] |= RF_COLOR_BLACK;
    }

    // run finalizer function
    cmd->finalize_func(ctx);
    return true;
}
