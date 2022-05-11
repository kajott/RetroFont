#ifndef RETROFONT_H_
#define RETROFONT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define RF_GLYPH_CACHE_MIN  32
#define RF_GLYPH_CACHE_MAX 255
#define RF_GLYPH_CACHE_SIZE (RF_GLYPH_CACHE_MAX - RF_GLYPH_CACHE_MIN + 1)

//! create an ID from a string
//! \note the 's' parameter *must* be at least 4 characters long!
#define RF_MAKE_ID(s) ((((uint32_t)((s)[3])) << 24) \
                     | (((uint32_t)((s)[2])) << 16) \
                     | (((uint32_t)((s)[1])) <<  8) \
                     |  ((uint32_t)((s)[0])))

typedef struct s_RF_Coord         RF_Coord;
typedef struct s_RF_Cell          RF_Cell;
typedef struct s_RF_RenderCommand RF_RenderCommand;
typedef struct s_RF_SysClass      RF_SysClass;
typedef struct s_RF_System        RF_System;
typedef struct s_RF_Font          RF_Font;
typedef struct s_RF_GlyphMapEntry RF_GlyphMapEntry;
typedef struct s_RF_Context       RF_Context;

#define RF_COLOR_DEFAULT ((uint32_t)(-1))  //!< platform default FG/BG color
#define RF_COLOR_BLACK   0x1000000ul  //!< standard black color
#define RF_COLOR_BLUE    0x1000001ul  //!< standard blue color
#define RF_COLOR_GREEN   0x1000002ul  //!< standard green color
#define RF_COLOR_CYAN    0x1000003ul  //!< standard cyan color
#define RF_COLOR_RED     0x1000004ul  //!< standard red color
#define RF_COLOR_MAGENTA 0x1000005ul  //!< standard magenta color
#define RF_COLOR_YELLOW  0x1000006ul  //!< standard yellow color
#define RF_COLOR_WHITE   0x1000007ul  //!< standard white (or light gray) color
#define RF_COLOR_BRIGHT  0x0000008ul  //!< OR'ed into the RF_COLOR_* codes
#define RF_IS_RGB_COLOR(c) (((c) & (~0xFFFFFFul)) == 0ul)
#define RF_IS_STD_COLOR(c) (((c) & (~(RF_COLOR_WHITE | RF_COLOR_BRIGHT))) == 0ul)
#define RF_COLOR_RGB(r,g,b) (((r) << 16) | ((g) << 8) | (b))
#define RF_COLOR_R(c) ((uint8_t)((c) >> 16))
#define RF_COLOR_G(c) ((uint8_t)((c) >>  8))
#define RF_COLOR_B(c) ((uint8_t) (c))

//! map any RGB color to one of the standard 16 colors (RF_COLOR_DEFAULT is left untouched)
//! \param bright_threshold  if the brightest component is brighter than this, the bright flag is set
uint32_t RF_MapRGBToStandardColor(uint32_t color, uint8_t bright_threshold);

//! map one of the standard 16 colors to RGB
uint32_t RF_MapStandardColorToRGB(uint32_t color, uint8_t std0, uint8_t std1, uint8_t bright0, uint8_t bright1);

struct s_RF_Coord {
    uint16_t x;
    uint16_t y;
};

struct s_RF_Cell {
    uint32_t codepoint;    //!< unicode codepoint of glyph to render
    uint32_t dirty:1;      //!< "needs update" flag
    uint32_t bold:1;       //!< bold attribute flag
    uint32_t dim:1;        //!< dim attribute flag
    uint32_t underline:1;  //!< underline attribute flag
    uint32_t blink:1;      //!< blink attribute flag
    uint32_t reverse:1;    //!< reverse attribute flag
    uint32_t invisible:1;  //!< invisible attribute flag
    uint32_t fg;           //!< foreground color (0xRRGGBB or RF_COLOR_*)
    uint32_t bg;           //!< background color (0xRRGGBB or RF_COLOR_*)
};

struct s_RF_RenderCommand {
    RF_Cell *cell;
    const uint8_t *glyph_data;
    uint8_t *pixel;
    size_t stride;
    uint32_t sys_id;
    RF_Coord cell_size;
    RF_Coord font_size;
    uint32_t underline_row;
    uint32_t default_fg;
    uint32_t default_bg;
    bool is_cursor;
    bool blink_phase;
};

//! render a cell with specific RGB colors; honors the reverse and invisible flags,
//! but can perform an extra reverse or force invisibility if needed for blinking
void RF_RenderCell(
    const RF_RenderCommand* cmd,
    uint32_t fg,         uint32_t bg,
    uint16_t offset_x,   uint16_t offset_y,
    uint16_t line_start, uint16_t line_end,
    bool extra_reverse,
    bool force_invisible,
    bool allow_underline
);

struct s_RF_SysClass {
    uint32_t (*map_border_color) (uint32_t sys_id, uint32_t color);
    void (*render_cell) (const RF_RenderCommand* cmd);
};

struct s_RF_System {
    uint32_t sys_id;
    const char* name;
    const RF_SysClass *cls;
    RF_Coord default_screen_size;
    RF_Coord cell_size;
    RF_Coord font_size;
    RF_Coord border_ul;
    RF_Coord border_lr;
    bool doublescan;
    uint32_t blink_interval_msec;
    uint32_t default_font_id;
};

struct s_RF_GlyphMapEntry {
    uint32_t codepoint;
    uint32_t bitmap_offset;
};

struct s_RF_Font {
    uint32_t font_id;
    const char* name;
    RF_Coord font_size;
    const RF_GlyphMapEntry *glyph_map;
    uint32_t glyph_count;
    uint32_t fallback_offset;
    uint32_t underline_row;
};

struct s_RF_Context {
    RF_Cell *screen;
    RF_Coord screen_size;
    const RF_System *system;
    const RF_Font *font;
    uint8_t *bitmap;
    size_t stride;
    RF_Coord bitmap_size;
    RF_Coord cursor_pos;
    uint32_t default_fg;
    uint32_t default_bg;
    uint32_t border_color;
    uint32_t border_rgb;
    bool border_color_changed;
    bool has_border;
    bool last_blink_phase;
    uint32_t glyph_offset_cache[RF_GLYPH_CACHE_SIZE];
};

extern const RF_System* RF_SystemList[];
extern const RF_Font    RF_FontList[];
extern const uint8_t    RF_GlyphBitmaps[];

//! create empty context with specified system ID
//! \note before the context can be used, RF_ResizeScreen() must be called
RF_Context* RF_CreateContext(uint32_t sys_id);

//! set the font to be used
//! \param font_id  ID for the font to set; 0 = system default
//! \returns true if the font has been set, false if the font is unknown or invalid
bool RF_SetFont(RF_Context* ctx, uint32_t font_id);

//! resize the screen (or initialize it if not called before)
//! \param new_width    target width (0 = system default)
//! \param new_height   target height (0 = system default)
//! \param with_border  whether to render the border
//! \returns true if successful, false if failed
bool RF_ResizeScreen(RF_Context* ctx, uint16_t new_width, uint16_t new_height, bool with_border);

//! move the cursor to a new position
void RF_MoveCursor(RF_Context* ctx, uint16_t new_col, uint16_t new_row);

//! set the global colors
#define RF_SetForegroundColor(ctx, c) do { (ctx)->default_fg = c;   RF_Invalidate(ctx, false); } while(0)
#define RF_SetBackgroundColor(ctx, c) do { (ctx)->default_bg = c;   RF_Invalidate(ctx, false); } while(0)
#define RF_SetBorderColor(ctx, c)     do { (ctx)->border_color = c; (ctx)->border_color_changed = true; } while(0)

//! invalidate the whole screen
void RF_Invalidate(RF_Context* ctx, bool with_border);

//! render the screen (i.e. the "dirty" parts of it)
//! \returns true if anything changed, false otherwise
bool RF_Render(RF_Context* ctx, uint32_t time_msec);

//! destroy a context
void RF_DestroyContext(RF_Context* ctx);

#define RF_FreeContext(ctx) do { RF_DestroyContext(ctx); (ctx) = NULL; } while(0)

#endif
