#ifndef RETROFONT_H_
#define RETROFONT_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// glyph lookup cache configuration
#define RF_GLYPH_CACHE_MIN  32  //!< first codepoint to cache
#define RF_GLYPH_CACHE_MAX 255  //!< last codepoint to cache
#define RF_GLYPH_CACHE_SIZE (RF_GLYPH_CACHE_MAX - RF_GLYPH_CACHE_MIN + 1)  //!< size of the glyph case

//! create an ID from a string
//! \note the 's' parameter *must* be at least 4 characters long!
#define RF_MAKE_ID_S(s) ((((uint32_t)(((const char*)(s))[3])) << 24) \
                       | (((uint32_t)(((const char*)(s))[2])) << 16) \
                       | (((uint32_t)(((const char*)(s))[1])) <<  8) \
                       |  ((uint32_t)(((const char*)(s))[0])))
//! create an ID from four characters
#define RF_MAKE_ID(a,b,c,d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

// forward definitions of structures
typedef struct s_RF_Coord          RF_Coord;
typedef struct s_RF_Cell           RF_Cell;
typedef struct s_RF_RenderCommand  RF_RenderCommand;
typedef struct s_RF_SysClass       RF_SysClass;
typedef struct s_RF_System         RF_System;
typedef struct s_RF_Font           RF_Font;
typedef struct s_RF_GlyphMapEntry  RF_GlyphMapEntry;
typedef struct s_RF_FallbackGlyphs RF_FallbackGlyphs;
typedef struct s_RF_Context        RF_Context;

// color-related constants and macros
#define RF_COLOR_DEFAULT ((uint32_t)(-1))  //!< system default FG/BG color
#define RF_COLOR_BLACK   0x1000000ul  //!< standard black color
#define RF_COLOR_BLUE    0x1000001ul  //!< standard blue color
#define RF_COLOR_GREEN   0x1000002ul  //!< standard green color
#define RF_COLOR_CYAN    0x1000003ul  //!< standard cyan color
#define RF_COLOR_RED     0x1000004ul  //!< standard red color
#define RF_COLOR_MAGENTA 0x1000005ul  //!< standard magenta color
#define RF_COLOR_YELLOW  0x1000006ul  //!< standard yellow color
#define RF_COLOR_WHITE   0x1000007ul  //!< standard white (or light gray) color
#define RF_COLOR_BRIGHT  0x0000008ul  //!< OR'ed into the RF_COLOR_* codes
//! check whether a color is a fully specified RGB color
#define RF_IS_RGB_COLOR(c) (((c) & (~0xFFFFFFul)) == 0ul)
//! check whether a color is one of the standard 16 colors
#define RF_IS_STD_COLOR(c) (((c) & (~(RF_COLOR_WHITE | RF_COLOR_BRIGHT))) == 0ul)
//! create color from 8-bit RGB values
#define RF_COLOR_RGB(r,g,b) (((r) << 16) | ((g) << 8) | (b))
#define RF_COLOR_R(c) ((uint8_t)((c) >> 16))  //!< extract red   component from an RGB color
#define RF_COLOR_G(c) ((uint8_t)((c) >>  8))  //!< extract green component from an RGB color
#define RF_COLOR_B(c) ((uint8_t) (c))         //!< extract blue  component from an RGB color

// special codepoints (mapped roughly to their ASCII equivalents)
#define RF_CP_BACKSPACE  8  //!< remove character left of cursor
#define RF_CP_TAB        9  //!< advance cursor to next multiple of 8
#define RF_CP_ENTER     10  //!< advance cursor to next line
#define RF_CP_DELETE   127  //!< remove character under cursor

// glyph fallback modes
typedef enum e_RF_FallbackMode {
    RF_FB_NONE = 0,  //!< allow no fallback
    RF_FB_GLYPHS,    //!< allow fallback to other font's glyphs
} RF_FallbackMode;

// text markup types
typedef enum e_RF_MarkupType {
    RF_MT_NONE     = 0x99,  //!< no markup (plain text)
    RF_MT_INTERNAL = 0x60,  //!< RetroFont's internal markup system [TODO]
    RF_MT_ANSI     = 0x1B,  //!< VT-100 / ANSI compatible Escape codes [TODO]
} RF_MarkupType;

// misc other constants
#define RF_SIZE_DEFAULT ((uint16_t)(-1))       //!< system default size for RF_ResizeScreen()
#define RF_SIZE_PIXELS  0x8000u                //!< system's default_screen_size is in pixels
#define RF_SIZE_MASK    (RF_SIZE_PIXELS - 1u)  //!< mask to remove RF_SIZE_PIXELS flag

//! map any RGB color to one of the standard 16 colors (RF_COLOR_DEFAULT is left untouched)
//! \param bright_threshold  if the brightest component is brighter than this, the bright flag is set
uint32_t RF_MapRGBToStandardColor(uint32_t color, uint8_t bright_threshold);

//! map one of the standard 16 colors to RGB
//! \param std0     intensity value for inactive components if bright flag is not set
//! \param std1     intensity value for   active components if bright flag is not set
//! \param bright0  intensity value for inactive components if bright flag is     set
//! \param bright1  intensity value for   active components if bright flag is     set
uint32_t RF_MapStandardColorToRGB(uint32_t color, uint8_t std0, uint8_t std1, uint8_t bright0, uint8_t bright1);

//! 2D point coordinate
struct s_RF_Coord {
    uint16_t x, y;
};

//! single text screen cell
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

//! command structure used in the render_cell system class method
struct s_RF_RenderCommand {
    const RF_Context *ctx;      //!< context to draw from
                                //!< \note this pointer, and the 'system' and 'font'
                                //!        pointers are guaranteed to be valid
    RF_Cell *cell;              //!< cell to draw
    const uint8_t *glyph_data;  //!< glyph data to use for drawing
    uint8_t *pixel;             //!< pointer to upper-left corner of the cell in the target bitmap
    bool is_cursor;             //!< whether this cell is at the current cursor location
    bool blink_phase;           //!< blink phase (toggles between true and false periodically)
};

//! render a cell with specific RGB colors; honors the reverse and invisible flags,
//! but can perform an extra reverse or force invisibility if needed for blinking
void RF_RenderCell(
    const RF_RenderCommand* cmd,  //!< render command with base data
    uint32_t fg,                  //!< resolved foreground color (must be RGB!)
    uint32_t bg,                  //!< resolved background color (must be RGB!)
    uint16_t offset_x,            //!< X offset of the glyph in the cell
    uint16_t offset_y,            //!< Y offset of the glyph in the cell
    uint16_t line_start,          //!< start row of extra underline (e.g. for cursor)
    uint16_t line_end,            //!< end row (non-inclusive) of extra underline (e.g. for cursor)
    bool extra_reverse,           //!< whether to flip FG and BG color once more
    bool force_invisible,         //!< force the character to be invisible (all-background)
    bool allow_underline,         //!< allow underlining as defined in the font
    bool allow_bold               //!< allow bold printing
);

//! method table for a system class
struct s_RF_SysClass {
    //! map a border color to RGB
    uint32_t (*map_border_color) (uint32_t sys_id, uint32_t color);

    //! render a single character cell.
    //! This method typically resolves the foreground and background colors
    //! to RGB in a system-specific way and then calls RF_RenderCell().
    void (*render_cell) (const RF_RenderCommand* cmd);

    //! check whether a font is applicable for that system.
    //! This method can be used to further filter eligible fonts, in addition
    //! to the built-in RF_System::font_size filter.
    //! \note This can be NULL; in that case, every font will be accepted.
    bool (*check_font) (uint32_t sys_id, const RF_Font* font);
};

//! single entry of a codepoint-to-glyph map
struct s_RF_GlyphMapEntry {
    uint32_t codepoint;      //!< Unicode codepoint
    uint32_t bitmap_offset;  //!< offset in the central font bitmap where the glyph starts
};

//! system registry item
struct s_RF_System {
    uint32_t sys_id;                //!< internal system ID
    const char* name;               //!< user-facing system name
    const RF_SysClass *cls;         //!< pointer to method table
    const char* default_screen;     //!< default screen contents (in RF_MT_INTERNAL format)
    RF_Coord default_screen_size;   //!< platform's default screen size (in character cells,
                                    //!< OR in pixels if RF_SIZE_PIXELS is added)
                                    //!< \note if RF_SIZE_PIXELS is used, *both* axes must use it
    RF_Coord cell_size;             //!< platform's character cell size (in pixels)
                                    //!< \note if font_size is zero, this is the number of
                                    //!<       *extra* pixels to add to each cell
    RF_Coord font_size;             //!< platform's font bitmap size (in pixels)
                                    //!< \note either axis can be zero; the system
                                    //!<       then accepts any font size on this axis
    RF_Coord border_ul;             //!< default upper-left border
    RF_Coord border_lr;             //!< default lower-right border
    RF_Coord coarse_aspect;         //!< coarse pixel aspect ratio (typically 1x1, 1x2 or 2x1)
    uint32_t blink_interval_msec;   //!< blinking interval in milliseconds; 0 = no blinking
    uint32_t default_font_id;       //!< ID of the system's default font
};

//! font registry item
struct s_RF_Font {
    uint32_t font_id;                   //!< internal font ID (0 = end of list)
    const char* name;                   //!< human-readable font name
    RF_Coord font_size;                 //!< font size in pixels
    const RF_GlyphMapEntry *glyph_map;  //!< codepoint-to-gylph map
                                        //!< \note MUST be sorted by codepoint!
    uint32_t glyph_count;               //!< number of codepoints in the glyph map
    uint32_t fallback_offset;           //!< bitmap offset of the fallback glyph
    uint16_t underline_row;             //!< row where underlining shall be done; 0 = no underline support
};

//! fallback registry item
struct s_RF_FallbackGlyphs {
    RF_Coord font_size;                 //!< font size in pixels (zero width = end of list)
    const RF_GlyphMapEntry *glyph_map;  //!< codepoint-to-gylph map
                                        //!< \note MUST be sorted by codepoint!
    uint32_t glyph_count;               //!< number of codepoints in the glyph map
};

//! RetroFont instance.
//! In general, all non-private members are free to access, but read-only,
//! except screen, which is read-write.
struct s_RF_Context {
    RF_Cell *screen;            //!< screen contents (character cells)
                                //!< \note When changing any cell here,
                                //!<       make sure to set its dirty attribute!
    RF_Coord screen_size;       //!< size of the screen (in character cells)
    const RF_System *system;    //!< currently selected system
    const RF_Font *font;        //!< currently selected font
    RF_FallbackMode fallback;   //!< what to do with invalid glyphs
    uint8_t *bitmap;            //!< rendered bitmap, top-down, RGB888 format
    size_t stride;              //!< distance between rows (always bitmap_size.x * 3)
    RF_Coord bitmap_size;       //!< size of the bitmap, in pixels
    RF_Coord main_ul;           //!< pixel coordinate of the upper-left corner of the main screen area
    RF_Coord main_lr;           //!< pixel coordinate of the lower-right corner of the main screen area (non inclusive)
    RF_Coord cell_size;         //!< effective cell size
    float pixel_aspect;         //!< system's pixel aspect ratio
    uint32_t border_rgb;        //!< border color (translated to RGB by RF_Render)
    RF_Cell attrib;             //!< attribute for next added character
    bool insert;                //!< false: RF_PutChar overwrites, true: RF_PutChar inserts on current line

//private: // (renderer)
    RF_Coord cursor_pos;        //!< \private cursor position
    uint32_t default_fg;        //!< \private default foreground color (default = system default)
    uint32_t default_bg;        //!< \private default background color (default = system default)
    uint32_t border_color;      //!< \private border color (default / standard / RGB)
    bool border_color_changed;  //!< \private true if the border color changed
    bool has_border;            //!< \private whether the border in included in the bitmap
    bool last_blink_phase;      //!< \private blink phase of the last RF_Render() call
    uint32_t glyph_offset_cache[RF_GLYPH_CACHE_SIZE];  //!< \private glyph cache (-1 = uncached)
    const RF_FallbackGlyphs* fb_glyphs;                //!< \private fallback glyph list (NULL = no fallback)

//private: // (markup parser)
    uint8_t utf8_cb_count;      //!< \private UTF-8 continuation byte count
    uint8_t esc_count;          //!< \private number of byte inside an escape sequence (0 = no escape)
    uint8_t esc_remain;         //!< \private number of characters of numbers to follow
    uint32_t num_buf;           //!< \private number buffer (e.g. current UTF-8 codepoint)
    const void* esc_class;      //!< \private pointer to internal escape type descriptor
};

// central registries
extern const RF_System* const  RF_SystemList[];          //!< system registry
extern const RF_Font           RF_FontList[];            //!< font registry
extern const RF_FallbackGlyphs RF_FallbackGlyphsList[];  //!< fallback glyph map registry
extern const uint8_t           RF_GlyphBitmaps[];        //!< glyph bitmaps
extern const RF_Cell           RF_EmptyCell;             //!< cell with default contents

//! create empty context with specified system ID
//! \note before the context can be used, RF_ResizeScreen() must be called
RF_Context* RF_CreateContext(uint32_t sys_id);

//! change the system (and load its default font and colors)
//! \param sys_id  ID of the system to load
//! \returns true if the system has been set, false if the system is unknown or invalid
bool RF_SetSystem(RF_Context* ctx, uint32_t sys_id);

//! set the font to be used
//! \param font_id  ID for the font to set; 0 = system default
//! \returns true if the font has been set, false if the font is unknown or invalid
bool RF_SetFont(RF_Context* ctx, uint32_t font_id);

//! set the fallback mode
void RF_SetFallbackMode(RF_Context* ctx, RF_FallbackMode mode);

//! resize the screen (or initialize it if not called before)
//! \param new_width    target width  (0 = keep old value; RF_SIZE_DEFAULT = system default)
//! \param new_height   target height (0 = keep old value; RF_SIZE_DEFAULT = system default)
//! \param with_border  whether to render the border
//! \returns true if successful, false if failed
//! \note This *MUST* be called after RF_CreateContext/RF_SetSystem and RF_SetFont
bool RF_ResizeScreen(RF_Context* ctx, uint16_t new_width, uint16_t new_height, bool with_border);

//! check whether a specific system can use a specific font
bool RF_SystemCanUseFont(const RF_System* sys, const RF_Font* font);
//! check whether the current system can use a specific font
#define RF_CanUseFont(ctx, font) ((ctx) ? RF_SystemCanUseFont((ctx)->system, font) : false)

//! move the cursor to a new position
void RF_MoveCursor(RF_Context* ctx, uint16_t new_col, uint16_t new_row);

//! set the default foreground color
#define RF_SetForegroundColor(ctx, c) do { (ctx)->default_fg = c;   RF_Invalidate(ctx, false); } while(0)
//! set the default background color
#define RF_SetBackgroundColor(ctx, c) do { (ctx)->default_bg = c;   RF_Invalidate(ctx, false); } while(0)
//! set the border color
#define RF_SetBorderColor(ctx, c)     do { (ctx)->border_color = c; (ctx)->border_color_changed = true; } while(0)

//! clear the screen
//! \param cell  cell contents to fill the screen with; NULL = use empty cell
void RF_ClearScreen(RF_Context* ctx, const RF_Cell* cell);
//! clear the screen, all attributes and all colors
#define RF_ClearAll(ctx) do { RF_ClearScreen(ctx, NULL); if (ctx) { (ctx)->default_fg = (ctx)->default_bg = RF_COLOR_DEFAULT; } RF_SetBorderColor(ctx, RF_COLOR_DEFAULT); } while (0)

//! create a "demo" screen with character set and attribute tests
//! \note uses the C library's rand() function; make sure to srand() before!
void RF_DemoScreen(RF_Context* ctx);

//! invalidate the whole screen
void RF_Invalidate(RF_Context* ctx, bool with_border);

//! put a single character on-screen (with the currently selected attribute).
//! RF_CP_* values are handled specially.
void RF_AddChar(RF_Context* ctx, uint32_t codepoint);

//! Add a string of UTF-8 text, optionally with markup.
void RF_AddText(RF_Context* ctx, const char* str, RF_MarkupType mt);

//! fill a region with blanks
//! \param attrib  attribute to use (NULL = use defaults)
void RF_ClearRegion(RF_Context* ctx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const RF_Cell* attrib);

//! scroll a region vertically (positive = downward, negative = upward)
//! \param attrib  attribute to use (NULL = autodetect)
void RF_ScrollRegion(RF_Context* ctx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int16_t dy, const RF_Cell* attrib);

//! render the screen (or rather, the "dirty" parts of it)
//! \returns true if anything changed, false otherwise
bool RF_Render(RF_Context* ctx, uint32_t time_msec);

//! destroy a context
void RF_DestroyContext(RF_Context* ctx);
//! destroy a context and set the pointer to NULL to avoid double-free
#define RF_FreeContext(ctx) do { RF_DestroyContext(ctx); (ctx) = NULL; } while(0)

#ifdef __cplusplus
}
#endif

#endif
