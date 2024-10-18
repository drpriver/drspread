#ifndef DRT_LL_H
#define DRT_LL_H

#include <stddef.h>

#ifndef DRT_API
#define DRT_API static
#endif

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

typedef struct DrtLL DrtLL;

DRT_API
int
drt_paint(DrtLL* ll);

DRT_API
void
drt_init(DrtLL* ll);

DRT_API
void
drt_invalidate(DrtLL* ll);

DRT_API
void
drt_move(DrtLL* ll, int x, int y);

DRT_API
void
drt_update_drawable_area(DrtLL* ll, int x, int y, int w, int h);

DRT_API
void
drt_update_terminal_size(DrtLL* ll, int w, int h);

DRT_API
void
drt_push_state(DrtLL* ll);

DRT_API
void
drt_pop_state(DrtLL* ll);

DRT_API
void
drt_pop_all_states(DrtLL* ll);

DRT_API
void
drt_clear_state(DrtLL* ll);

enum DrtStyle {
    DRT_STYLE_NONE = 0x0,
    DRT_STYLE_BOLD = 0x1,
    DRT_STYLE_ITALIC = 0x2,
    DRT_STYLE_UNDERLINE = 0x4,
    DRT_STYLE_STRIKETHROUGH = 0x8,
    DRT_STYLE_ALL = DRT_STYLE_BOLD|DRT_STYLE_ITALIC|DRT_STYLE_UNDERLINE|DRT_STYLE_STRIKETHROUGH,
} typedef DrtStyle;

DRT_API
void
drt_set_style(DrtLL* ll, unsigned style);

DRT_API
void
drt_clear_color(DrtLL* ll);

DRT_API
void
drt_set_8bit_color(DrtLL* ll, unsigned color);

DRT_API
void
drt_set_24bit_color(DrtLL* ll, unsigned char r, unsigned char g, unsigned char b);

DRT_API
void
drt_bg_clear_color(DrtLL* ll);

DRT_API
void
drt_bg_set_8bit_color(DrtLL* ll, unsigned color);

DRT_API
void
drt_bg_set_24bit_color(DrtLL* ll, unsigned char r, unsigned char g, unsigned char b);


DRT_API
void
drt_setc(DrtLL* ll, char c);

DRT_API
void
drt_setc_at(DrtLL* ll, int x, int y, char c);

DRT_API
void
drt_putc(DrtLL* ll, char c);
DRT_API
void
drt_putc(DrtLL* ll, char c);

DRT_API
void
drt_putc_mb(DrtLL* ll, const char* c, size_t length, size_t rendwith);

DRT_API
void
drt_puts(DrtLL* ll, const char* txt, size_t length);

DRT_API
void
drt_set_cursor_visible(DrtLL* ll, _Bool show);

DRT_API
void
drt_clear_screen(DrtLL* ll);

DRT_API
void
drt_move_cursor(DrtLL* ll, int x, int y);

DRT_API
void
__attribute__((format(printf,2, 3)))
drt_printf(DrtLL* ll, const char* fmt, ...);

DRT_API
void
drt_clear_to_end_of_row(DrtLL*ll);

// DRT_API
// void
// drt_putc_mb(DrtLL* ll, const char* txt, size_t length, size_t render_length);

// DRT_API
// void
// drt_setc_mb(DrtLL* ll, const char* txt, size_t length, size_t render_length);




#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
