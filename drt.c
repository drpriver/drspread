//
// Copyright © 2024, David Priver <david@davidpriver.com>
//
#ifndef DRT_LL_C
#define DRT_LL_C

#include "drt.h"
#include <stdarg.h>
#include <string.h>

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

enum {MAX_LINES=200, MAX_COLUMNS=400};

typedef struct DrtLLColor DrtLLColor;
struct DrtLLColor {
    union {
        struct {
            _Bool is_24bit:1;
            _Bool is_not_reset:1;
            union {
                unsigned char _8bit;
                unsigned char r;
            };
            unsigned char g;
            unsigned char b;
        };
        unsigned bits;
    };
};
_Static_assert(sizeof(DrtLLColor) == sizeof(unsigned), "");

typedef struct DrtLLState DrtLLState;
struct DrtLLState {
    unsigned style;
    DrtLLColor color;
    DrtLLColor bg_color;
};

typedef struct DrtLLCell DrtLLCell;
struct DrtLLCell {
    DrtLLColor color;
    DrtLLColor bg_color;
    unsigned char style:5;
    unsigned char rend_width:3;
    char txt[7];
};

struct DrtLL {
    DrtLLState state_stack[100];
    size_t state_cursor;
    int term_w, term_h;
    struct {
        int x, y, w, h;
    } draw_area;
    int x, y;
    DrtLLCell cells[2][MAX_LINES*MAX_COLUMNS];
    _Bool active_cells;
    _Bool dirty;
    _Bool force_paint;
    _Bool cursor_visible;
    char buff[32*MAX_LINES*MAX_COLUMNS];
    size_t buff_cursor;
    int cur_x, cur_y;
};

static inline
DrtLLCell*
drt_current_cell(DrtLL* ll){
    return &ll->cells[ll->active_cells][ll->x+ll->y*ll->draw_area.w];
}
static inline
DrtLLCell*
drt_old_cell(DrtLL* ll){
    return &ll->cells[!ll->active_cells][ll->x+ll->y*ll->draw_area.w];
}

static inline
DrtLLState*
drt_current_state(DrtLL* ll){
    return &ll->state_stack[ll->state_cursor];
}

static inline
void
drt_sprintf(DrtLL* ll, const char* fmt, ...){
    va_list va;
    va_start(va, fmt);
    char* buff = ll->buff+ ll->buff_cursor;
    size_t remainder = sizeof ll->buff - ll->buff_cursor;
    size_t n = vsnprintf(buff, remainder, fmt, va);
    if(n > remainder)
        n = remainder - 1;
    ll->buff_cursor += n;
    va_end(va);
}

static inline
void
drt_flush(DrtLL* ll){
    drt_sprintf(ll, "\x1b[%d;%dH", ll->cur_y+ll->draw_area.y+1, ll->cur_x+ll->draw_area.x+1);
    if(0){
        static FILE* fp;
        if(!fp) fp = fopen("drtlog.txt", "a");
        fprintf(fp, "\nflush\n-----\n\n");
        for(size_t i = 0; i < ll->buff_cursor; i++){
            char c = ll->buff[i];
            if((unsigned)(unsigned char)c < 0x20)
                fprintf(fp, "\\x%x", c);
            else
                fputc(c, fp);
        }
        fflush(fp);
    }
    fwrite(ll->buff, ll->buff_cursor, 1, stdout);
    fflush(stdout);
    ll->buff_cursor = 0;
    ll->force_paint = 0;
}

DRT_API
void
drt_init(DrtLL* ll){
    drt_sprintf(ll, "\x1b[?1049h");
}

DRT_API
void
drt_end(DrtLL* ll){
    drt_sprintf(ll, "\x1b[?25h");
    drt_sprintf(ll, "\x1b[?1049l");
    drt_sprintf(ll, "\n");
    drt_flush(ll);
}

typedef struct DrtPaint DrtPaint;
struct DrtPaint {
    DrtLLState state;
    int x, y;
};

static inline
void
drt_paint_update(DrtLL* ll, DrtPaint* p, int x, int y, DrtLLCell* new){
    if(x != p->x || y != p->y){
        // Goto coord
        int term_x = ll->draw_area.x + x+1;
        int term_y = ll->draw_area.y + y+1;
        drt_sprintf(ll, "\x1b[%d;%dH", term_y, term_x);
    }
    _Bool started = 0;
    if(p->state.style != new->style){
        // set style
        drt_sprintf(ll, "\x1b[0;");
        started = 1;
        p->state.color = (DrtLLColor){0};
        p->state.bg_color = (DrtLLColor){0};
        if(new->style & DRT_STYLE_BOLD){
            drt_sprintf(ll, "1;");
        }
        if(new->style & DRT_STYLE_ITALIC){
            drt_sprintf(ll, "3;");
        }
        if(new->style & DRT_STYLE_UNDERLINE){
            drt_sprintf(ll, "4;");
        }
        if(new->style & DRT_STYLE_STRIKETHROUGH){
            drt_sprintf(ll, "9;");
        }
        // drt_sprintf(ll, "m");
    }
    if(p->state.color.bits != new->color.bits){
        if(!started){
            started = 1;
            drt_sprintf(ll, "\x1b[");
        }
        // set color
        if(!new->color.is_not_reset){
            drt_sprintf(ll, "39;");
        }
        else if(new->color.is_24bit){
            drt_sprintf(ll, "38;2;%d;%d;%d;", new->color.r, new->color.g, new->color.b);
        }
        else {
            drt_sprintf(ll, "38;5;%d;", new->color._8bit);
        }
    }
    if(p->state.bg_color.bits != new->bg_color.bits){
        if(!started){
            started = 1;
            drt_sprintf(ll, "\x1b[");
        }
        if(!new->bg_color.is_not_reset){
            drt_sprintf(ll, "49;");
        }
        else if(new->bg_color.is_24bit){
            drt_sprintf(ll, "48;2;%d;%d;%d;", new->bg_color.r, new->bg_color.g, new->bg_color.b);
        }
        else {
            drt_sprintf(ll, "48;5;%d;", new->bg_color._8bit);
        }
    }
    if(started){
        ll->buff[ll->buff_cursor-1] = 'm';
    }
    // write char
    if((unsigned)(unsigned char)new->txt[0] <= 0x20u)
        drt_sprintf(ll, " ");
    else if((unsigned)(unsigned char)new->txt[0] == 0x7fu)
        drt_sprintf(ll, " ");
    else 
        drt_sprintf(ll, "%s", new->txt);
    p->x = x+(new->rend_width?new->rend_width:1);
    p->y = y;
    p->state.style = new->style;
    p->state.color = new->color;
    p->state.bg_color = new->bg_color;
}

DRT_API
int
drt_paint(DrtLL* ll){
    if(!ll->dirty && !ll->force_paint) return 0;
    drt_sprintf(ll, "\x1b[?25l");
    drt_sprintf(ll, "\x1b[?2026h");
    if(ll->force_paint){
        drt_sprintf(ll, "\x1b[%d;%dH\x1b[0J", ll->draw_area.y+1, ll->draw_area.x+1);
    }
    DrtPaint paint = {.x = -1, .y=-1};
    for(int y = 0; y < ll->draw_area.h; y++)
        for(int x = 0; x < ll->draw_area.w; x++){
            DrtLLCell* old = &ll->cells[!ll->active_cells][x+y*ll->draw_area.w];
            DrtLLCell* new = &ll->cells[ll->active_cells][x+y*ll->draw_area.w];
            if(!old->txt[0]) old->txt[0] = ' ';
            if(!new->txt[0]) new->txt[0] = ' ';
            if(!ll->force_paint){
                if(memcmp(old, new, sizeof *new) == 0) continue;
                // if((unsigned)(unsigned char)old->txt[0] <= 0x20 && (unsigned)(unsigned char)new->txt[0] <= 0x20) continue;
            }
            drt_paint_update(ll, &paint, x, y, new);
            *old = *new;
            if(new->rend_width > 1)
                x += new->rend_width-1;
        }
    if(ll->cursor_visible){
        drt_sprintf(ll, "\x1b[?25h");
    }
    else {
        // drt_sprintf(ll, "\x1b[?25l");
    }
    drt_sprintf(ll, "\x1b[0m");
    drt_sprintf(ll, "\x1b[?2026l");
    drt_flush(ll);
    ll->active_cells = !ll->active_cells;
    return 0;
}

DRT_API
void
drt_clear_screen(DrtLL* ll){
    memset(ll->cells[ll->active_cells], 0, sizeof ll->cells[ll->active_cells]);
}

DRT_API
void
drt_invalidate(DrtLL* ll){
    ll->force_paint = 1;
}

DRT_API
void
drt_move(DrtLL* ll, int x, int y){
    if(x > -1){
        if(x >= ll->draw_area.w)
            x = ll->draw_area.w-1;
        ll->x = x;
    }
    if(y > -1){
        if(y >= ll->draw_area.h)
            y = ll->draw_area.h-1;
        ll->y = y;
    }
}

DRT_API
void
drt_update_drawable_area(DrtLL* ll, int x, int y, int w, int h){
    if(w < 0) w = 0;
    if(h < 0) h = 0;
    if(x+w>ll->term_w) w = ll->term_w - x;
    if(y+h>ll->term_h) h = ll->term_h - y;
    if(ll->draw_area.x == x && ll->draw_area.y == y && ll->draw_area.w == w && ll->draw_area.h == h)
        return;
    ll->force_paint = 1;
    ll->draw_area.x = x;
    ll->draw_area.y = y;
    ll->draw_area.w = w;
    ll->draw_area.h = h;
    if(ll->x >= w) ll->x = w?w-1:0;
    if(ll->y >= h) ll->y = h?h-1:0;

}

DRT_API
void
drt_update_terminal_size(DrtLL* ll, int w, int h){
    if(w > MAX_COLUMNS) w = MAX_COLUMNS;
    if(h > MAX_LINES) h = MAX_LINES;
    if(ll->term_w == w && ll->term_h == h)
        return;
    ll->force_paint = 1;
    ll->term_w = w;
    ll->term_h = h;
    if(ll->draw_area.x + ll->draw_area.w > w)
        drt_update_drawable_area(ll, ll->draw_area.x, ll->draw_area.y, w - ll->draw_area.x, h);
    if(ll->draw_area.y + ll->draw_area.h > h)
        drt_update_drawable_area(ll, ll->draw_area.x, ll->draw_area.y, ll->draw_area.w, h - ll->draw_area.y);
}

DRT_API
void
drt_push_state(DrtLL* ll){
    if(ll->state_cursor +1 >= sizeof ll->state_stack / sizeof ll->state_stack[0]){
        // __builtin_debugtrap();
        return;
    }
    ll->state_stack[ll->state_cursor+1] = ll->state_stack[ll->state_cursor];
    ll->state_cursor++;
}

DRT_API
void
drt_pop_state(DrtLL* ll){
    if(ll->state_cursor) --ll->state_cursor;
    // else __builtin_debugtrap();
}

DRT_API
void
drt_pop_all_states(DrtLL* ll){
    ll->state_cursor = 0;
    *drt_current_state(ll) = (DrtLLState){0};
}

DRT_API
void
drt_clear_state(DrtLL* ll){
    *drt_current_state(ll) = (DrtLLState){0};
}

DRT_API
void
drt_set_style(DrtLL* ll, unsigned style){
    // if(ll->state_cursor == 0) __builtin_debugtrap();
    drt_current_state(ll)->style = style & DRT_STYLE_ALL;
}

DRT_API
void
drt_set_8bit_color(DrtLL* ll, unsigned color){
    drt_current_state(ll)->color = (DrtLLColor){._8bit=color, .is_not_reset=1};
}

DRT_API
void
drt_clear_color(DrtLL* ll){
    drt_current_state(ll)->color = (DrtLLColor){0};
}


DRT_API
void
drt_set_24bit_color(DrtLL* ll, unsigned char r, unsigned char g, unsigned char b){
    drt_current_state(ll)->color = (DrtLLColor){.r=r, .g=g, .b=b, .is_24bit=1, .is_not_reset=1};
}

DRT_API
void
drt_bg_set_8bit_color(DrtLL* ll, unsigned color){
    drt_current_state(ll)->bg_color = (DrtLLColor){._8bit=color, .is_not_reset=1};
}

DRT_API
void
drt_bg_clear_color(DrtLL* ll){
    drt_current_state(ll)->bg_color = (DrtLLColor){0};
}


DRT_API
void
drt_bg_set_24bit_color(DrtLL* ll, unsigned char r, unsigned char g, unsigned char b){
    drt_current_state(ll)->bg_color = (DrtLLColor){.r=r, .g=g, .b=b, .is_24bit=1, .is_not_reset=1};
}

DRT_API
void
drt_setc(DrtLL* ll, char c){
    // if(ll->state_cursor == 0){
        // if(drt_current_state(ll)->style) __builtin_debugtrap();
    // }
    DrtLLState* state = drt_current_state(ll);
    DrtLLCell* cell = drt_current_cell(ll);
    memset(cell->txt, 0, sizeof cell->txt);
    cell->txt[0] = c;
    cell->color = state->color;
    cell->bg_color = state->bg_color;
    cell->style = state->style;
    cell->rend_width = 1;
    if(memcmp(cell, drt_old_cell(ll), sizeof *cell) != 0)
        ll->dirty = 1;
}

DRT_API
void
drt_setc_at(DrtLL* ll, int x, int y, char c){
    if(x >= ll->draw_area.w) return;
    if(y >= ll->draw_area.h) return;
    DrtLLState* state = drt_current_state(ll);
    DrtLLCell* cell = &ll->cells[ll->active_cells][x+y*ll->draw_area.w];
    memset(cell->txt, 0, sizeof cell->txt);
    cell->txt[0] = c;
    cell->color = state->color;
    cell->bg_color = state->bg_color;
    cell->style = state->style;
    cell->rend_width = 1;
    if(memcmp(cell, drt_old_cell(ll), sizeof *cell) != 0)
        ll->dirty = 1;
}

DRT_API
void
drt_putc(DrtLL* ll, char c){
    drt_setc(ll, c);
    drt_move(ll, ll->x+1, -1);
}

DRT_API
void
drt_setc_mb(DrtLL* ll, const char* c, size_t length, size_t rendwidth){
    DrtLLState* state = drt_current_state(ll);
    DrtLLCell* cell = drt_current_cell(ll);
    if(length > 7) return;
    memset(cell->txt, 0, sizeof cell->txt);
    memcpy(cell->txt, c, length);
    cell->color = state->color;
    cell->bg_color = state->bg_color;
    cell->style = state->style;
    cell->rend_width = rendwidth;
    if(memcmp(cell, drt_old_cell(ll), sizeof *cell) != 0)
        ll->dirty = 1;
}
DRT_API
void
drt_putc_mb(DrtLL* ll, const char* c, size_t length, size_t rend_width){
    drt_setc_mb(ll, c, length, rend_width);
    drt_move(ll, ll->x+rend_width, -1);
}

DRT_API
void
drt_puts(DrtLL* ll, const char* txt, size_t length){
    for(size_t i = 0; i < length; i++)
        drt_putc(ll, txt[i]);
}

DRT_API
void
drt_set_cursor_visible(DrtLL* ll, _Bool show){
    ll->cursor_visible = show;
}

DRT_API
void
drt_move_cursor(DrtLL* ll, int x, int y){
    ll->cur_x = x;
    ll->cur_y = y;
}

DRT_API
void
drt_printf(DrtLL* ll, const char* fmt, ...){
    char buff[1024];
    va_list va;
    va_start(va, fmt);
    int n = vsnprintf(buff, sizeof buff, fmt, va);
    va_end(va);
    drt_puts(ll, buff, n < sizeof buff? n : -1+sizeof buff);
}
DRT_API
void
drt_clear_to_end_of_row(DrtLL*ll){
    int w = ll->draw_area.w - ll->x;
    if(!w) return;
    memset(&ll->cells[ll->active_cells][ll->x+ll->y*ll->draw_area.w], 0, w * sizeof(DrtLLCell));
    ll->dirty = 1;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
