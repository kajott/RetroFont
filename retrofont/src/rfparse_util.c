#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "retrofont.h"

////////////////////////////////////////////////////////////////////////////////

const RF_Charset* RF_DetectCharset(const char* str) {
    bool valid_utf8 = true;
    uint8_t utf8_cb_count = 0;  // expected number of continuation bytes
    uint32_t hist[256], max_run[256];  // statistics buffers
    uint32_t run = 0;  // current run length
    uint8_t c, prev = 0;  // current and previous character
    const RF_Charset* common_best = NULL;
    const RF_Charset* run_best = NULL;
    uint32_t score, common_score = 0, run_score = 0;
    const uint8_t* p;  // statistics position

    if (!str || !str[0]) { return RF_Charsets; }
    memset((void*)hist,    0, sizeof(hist));
    memset((void*)max_run, 0, sizeof(max_run));
    do {
        c = (uint8_t)(*str++);
        // update histogram and run-length statistics
        if (c) { hist[c]++; }
        if (c == prev) { ++run; }
        else {
            if (run > max_run[prev]) { max_run[prev] = run; }
            run = 1;
            prev = c;
        }
        // UTF-8 check
        if (valid_utf8) {
            if (utf8_cb_count) { --utf8_cb_count; valid_utf8 = ((c & 0xC0) == 0x80); }
            else if ((c & 0xE0) == 0xC0) { utf8_cb_count = 1; }
            else if ((c & 0xF0) == 0xE0) { utf8_cb_count = 2; }
            else if ((c & 0xF8) == 0xF0) { utf8_cb_count = 3; }
            else { valid_utf8 = (c < 0xF8); }
        }
    } while (c);
    // if it's valid UTF-8, use the first charset (which is always UTF-8 by convention)
    if (valid_utf8) { return RF_Charsets; }
    // evaluate statistics
    for (const RF_Charset* cs = RF_Charsets;  cs->charset_id;  ++cs) {
        // compute score for common characters
        if (cs->common_chars) {
            uint32_t n = 0;
            score = 0;
            p = cs->common_chars;
            while (*p) {
                uint8_t a = *p++;
                uint8_t b = *p++;
                for (c = a;  (c >= a) && (c <= b);  ++c) {
                    score += hist[c];
                    if (hist[c]) { n++; }
                }
            }
            score = n ? ((score << 8) / n) : 0;
            if (score > common_score) {
                common_best = cs;
                common_score = score;
            }
        }
        // compute score for runs
        if (cs->run_chars) {
            score = 0;
            for (p = cs->run_chars;  *p;  ++p) {
                if (max_run[*p] > score) {
                    score = max_run[*p];
                }
            }
            if (score > run_score) {
                run_best = cs;
                run_score = score;
            }
        }
    }
    // decide of final result
    if (run_best && (run_score >= 20)) {
        // trust the "run length" metric if the longest run was 20 characters or more
        return run_best;
    }
    // otherwise, trust the "common characters" metric or fall back to UTF-8
    return common_best ? common_best : RF_Charsets;
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////
