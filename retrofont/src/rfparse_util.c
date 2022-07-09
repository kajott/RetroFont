#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "retrofont.h"

RF_MarkupType RF_DetectMarkupType(const char* str) {
    bool backtick_found = false;
    bool valid_internal_markup = true;
    bool last_was_backtick = false;
    char c;
    if (!str) { return RF_MT_NONE; }
    while ((c = *str++)) {
        // a single escape character is enough for us to believe it's ANSI/VT-100
        if (c == 27) { return RF_MT_ANSI; }
        // check if a backtick was followed by a valid internal markup command
        if (last_was_backtick) {
            if (!isalnum(c) && (c != '-') && (c != '+')) {
                valid_internal_markup = false;
            }
            last_was_backtick = false;
        }
        // check for a backtick character
        if ((c == '`') && valid_internal_markup) {
            backtick_found = true;
            last_was_backtick = true;
        }
    }
    return (backtick_found && valid_internal_markup) ? RF_MT_INTERNAL : RF_MT_NONE;
}
