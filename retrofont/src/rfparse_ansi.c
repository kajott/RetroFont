#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // DEBUG
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "retrofont.h"

static void SGR_ext(RF_Context* ctx, uint8_t* p_i, uint32_t* p_color) {
    if ((*p_i + 1) >= RF_MAX_NUMS) { return; }
    (*p_i)++;
    if (ctx->num[*p_i] == 2) {
        if ((*p_i + 3) >= RF_MAX_NUMS) { return; }
        *p_color = RF_COLOR_RGB(ctx->num[*p_i + 1], ctx->num[*p_i + 2], ctx->num[*p_i + 3]);
        (*p_i) += 3;
    } else if (ctx->num[*p_i] == 5) {
        uint8_t c;
        if ((*p_i + 1) >= RF_MAX_NUMS) { return; }
        (*p_i)++;
        c = (uint8_t) ctx->num[*p_i];
        if (c < 16) {  // IBGR -> IRGB
            *p_color = RF_COLOR_BLACK | (c & 0xA) | ((c >> 2) & 1) | ((c << 2) & 4);
        } else if (c < 232) {  // 6x6x6 color cube
            c -= 16;
            *p_color = RF_COLOR_RGB((c / 36) * 51, ((c / 6) % 6) * 51, (c % 6) * 51);
        } else { // grayscale ramp
            c -= 232;  c = (c * 11) + (c >> 3);
            *p_color = RF_COLOR_RGB(c, c, c);
        }
    }
}

static void handle_SGR(RF_Context* ctx) {
    uint8_t i;
    for (i = 0;  i <= ctx->num_idx;  ++i) {
        uint32_t n = ctx->num[i];
        switch (n) {
            case   0: ctx->attrib = RF_EmptyCell;        break;
            case   1: ctx->attrib.bold = 1;              break;
            case   2: ctx->attrib.dim = 1;               break;
            case  22: ctx->attrib.bold = ctx->attrib.dim = 0; break;
            case   4: ctx->attrib.underline = 1;         break;
            case  24: ctx->attrib.underline = 0;         break;
            case   5: ctx->attrib.blink = 1;             break;
            case  25: ctx->attrib.blink = 0;             break;
            case   7: ctx->attrib.reverse = 1;           break;
            case  27: ctx->attrib.reverse = 0;           break;
            case  30: ctx->attrib.fg = RF_COLOR_BLACK;   break;
            case  31: ctx->attrib.fg = RF_COLOR_RED;     break;
            case  32: ctx->attrib.fg = RF_COLOR_GREEN;   break;
            case  33: ctx->attrib.fg = RF_COLOR_YELLOW;  break;
            case  34: ctx->attrib.fg = RF_COLOR_BLUE;    break;
            case  35: ctx->attrib.fg = RF_COLOR_MAGENTA; break;
            case  36: ctx->attrib.fg = RF_COLOR_CYAN;    break;
            case  37: ctx->attrib.fg = RF_COLOR_WHITE;   break;
            case  38: SGR_ext(ctx, &i, &ctx->attrib.fg); break;
            case  39: ctx->attrib.bg = RF_COLOR_DEFAULT; break;
            case  40: ctx->attrib.bg = RF_COLOR_BLACK;   break;
            case  41: ctx->attrib.bg = RF_COLOR_RED;     break;
            case  42: ctx->attrib.bg = RF_COLOR_GREEN;   break;
            case  43: ctx->attrib.bg = RF_COLOR_YELLOW;  break;
            case  44: ctx->attrib.bg = RF_COLOR_BLUE;    break;
            case  45: ctx->attrib.bg = RF_COLOR_MAGENTA; break;
            case  46: ctx->attrib.bg = RF_COLOR_CYAN;    break;
            case  47: ctx->attrib.bg = RF_COLOR_WHITE;   break;
            case  48: SGR_ext(ctx, &i, &ctx->attrib.bg); break;
            case  49: ctx->attrib.bg = RF_COLOR_DEFAULT; break;
            case  90: ctx->attrib.fg = RF_COLOR_BLACK   | RF_COLOR_BRIGHT; break;
            case  91: ctx->attrib.fg = RF_COLOR_RED     | RF_COLOR_BRIGHT; break;
            case  92: ctx->attrib.fg = RF_COLOR_GREEN   | RF_COLOR_BRIGHT; break;
            case  93: ctx->attrib.fg = RF_COLOR_YELLOW  | RF_COLOR_BRIGHT; break;
            case  94: ctx->attrib.fg = RF_COLOR_BLUE    | RF_COLOR_BRIGHT; break;
            case  95: ctx->attrib.fg = RF_COLOR_MAGENTA | RF_COLOR_BRIGHT; break;
            case  96: ctx->attrib.fg = RF_COLOR_CYAN    | RF_COLOR_BRIGHT; break;
            case  97: ctx->attrib.fg = RF_COLOR_WHITE   | RF_COLOR_BRIGHT; break;
            case 100: ctx->attrib.fg = RF_COLOR_BLACK   | RF_COLOR_BRIGHT; break;
            case 101: ctx->attrib.fg = RF_COLOR_RED     | RF_COLOR_BRIGHT; break;
            case 102: ctx->attrib.fg = RF_COLOR_GREEN   | RF_COLOR_BRIGHT; break;
            case 103: ctx->attrib.fg = RF_COLOR_YELLOW  | RF_COLOR_BRIGHT; break;
            case 104: ctx->attrib.fg = RF_COLOR_BLUE    | RF_COLOR_BRIGHT; break;
            case 105: ctx->attrib.fg = RF_COLOR_MAGENTA | RF_COLOR_BRIGHT; break;
            case 106: ctx->attrib.fg = RF_COLOR_CYAN    | RF_COLOR_BRIGHT; break;
            case 107: ctx->attrib.fg = RF_COLOR_WHITE   | RF_COLOR_BRIGHT; break;
            default:
                #ifndef _NDEBUG
                    printf("unrecognized CSI SGR code %u\n", n);
                #endif
                break;
        }
    }
}


bool RF_ParseANSIMarkup(RF_Context* ctx, uint8_t c) {
    // handle initial character
    if (ctx->esc_count == 1) {
        memset((void*)ctx->num, 0, sizeof(ctx->num));
        ctx->num_idx = ctx->esc_type = 0;
        switch (c) {
            case '[':
                // start sequence capture
                ctx->esc_type = c;
                return false;
            default:
                return true;  // ignore invalid/unrecognized Escape sequence
        }
    }
    // switch from CSI to extended CSI
    if ((ctx->esc_type == '[') && (c == '?') && (ctx->esc_count == 2)) {
        ctx->esc_type = '?';
        return false;
    }

    // handle CSI sequence
    if ((ctx->esc_type == '[') || (ctx->esc_type == ']')) {
        if (isdigit(c)) {
            uint32_t n = ctx->num[ctx->num_idx];
            ctx->num[ctx->num_idx] = n * 10 + c - '0';
            return (n > 429496729);
        } else if (c == ';') {
            return (ctx->num_idx++) >= RF_MAX_NUMS;
        }
    }
    if (ctx->esc_type == '[') {
        switch (c) {
            case 'A': RF_MoveCursor(ctx, ctx->cursor_pos.x, (ctx->cursor_pos.y > ctx->num[0]) ? (uint16_t)(ctx->cursor_pos.y - ctx->num[0]) : 0); return true;
            case 'B': RF_MoveCursor(ctx, ctx->cursor_pos.x, ((ctx->cursor_pos.y + ctx->num[0]) < ctx->screen_size.y) ? (uint16_t)(ctx->cursor_pos.y + ctx->num[0]) : (ctx->screen_size.y - 1)); return true;
            case 'C': RF_MoveCursor(ctx, ((ctx->cursor_pos.x + ctx->num[0]) < ctx->screen_size.x) ? (uint16_t)(ctx->cursor_pos.x + ctx->num[0]) : (ctx->screen_size.x - 1), ctx->cursor_pos.y); return true;
            case 'D': RF_MoveCursor(ctx, (ctx->cursor_pos.x > ctx->num[0]) ? (uint16_t)(ctx->cursor_pos.x - ctx->num[0]) : 0, ctx->cursor_pos.y); return true;
            case 'm': handle_SGR(ctx); return true;
            default: break;  // "unrecognized CSI sequence" error will be handled below
        }
    }
    if ((ctx->esc_type == '[') || (ctx->esc_type == ']')) {
        #ifndef _NDEBUG
            printf("unrecognized CSI sequence: ESC [%s", (ctx->esc_type == '?') ? " ?" : "");
            for (uint8_t i = 0;  i <= ctx->num_idx;  ++i) { printf("%s %d", i ? " ;" : "", ctx->num[i]); }
            printf(" %c\n", c);
        #endif
        return true;
    }

    return true;
}
