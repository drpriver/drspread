//
// Copyright © 2023-2024, David Priver <david@davidpriver.com>
//
#ifdef __linux__
#define _GNU_SOURCE
#endif

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
typedef long long ssize_t;
#else
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#endif

#include "term_util.h"
#define DRSP_EXPORT static
#include "drspread.h"
#include "argument_parsing.h"
#include "drt.h"
#include "drp_merge_sort.h"
#include "measure_time.h"

static DrtLL* drt;

#ifdef _WIN32
static inline _Bool memeq(const void* a, const void* b, size_t sz);

static inline
void*
memmem(const void* haystack, size_t hsz, const void* needle, size_t nsz){
    if(nsz > hsz) return NULL;
    char* hay = (char*)haystack;
    for(size_t i = 0; i <= hsz-nsz; i++){
        if(memeq(hay+i, needle, nsz))
            return hay+i;
    }
    return NULL;
}

static inline
char*_Nullable
strsep(char*_Nullable*_Nonnull stringp, const char* delim){
    char* result = *stringp;
    char* s = *stringp;
    if(s){
        for(;*s;s++){
            for(const char* d=delim;*d;d++){
                if(*s == *d){
                    // This is garbage, but whatever, it's just for
                    // demo/testing purposes.
                    if(*d == '\n' && s != result && s[-1] == '\r')
                        s[-1] = '\0';
                    *s = '\0';
                    *stringp = s+1;
                    return result;
                }
            }
        }
    }
    *stringp = NULL;
    return result;
}
static inline void* xmalloc(size_t sz);

static inline
int
vasprintf(char*_Nullable*_Nonnull out, const char* fmt, va_list vap){
    va_list vap2;
    va_copy(vap2, vap);
    int len = vsnprintf(NULL, 0, fmt, vap);
    char* buff = xmalloc(len+1);
    int ret = vsnprintf(buff, len+1, fmt, vap2);
    *out = buff;
    va_end(vap2);
    return ret;
}

#if 0
static inline
char*
strdup(const char* p){
    size_t len = strlen(p);
    char* t = xmalloc(len+1);
    memcpy(t, p, len+1);
    return t;
}
#endif

#endif

static inline
_Bool
streq(const char* a, const char* b){
    return strcmp(a, b) == 0;
}

static inline
_Bool
memeq(const void* a, const void* b, size_t sz){
    return memcmp(a, b, sz) == 0;
}

static DrSpreadCtx* CTX;
static DrspAtom NIL_ATOM;
static _Bool DRAW_BORDERS;

static inline
_Bool
atom_contains(DrspAtom atom, const char* txt, size_t len){
    size_t alen;
    const char* atxt = drsp_atom_get_str(CTX, atom, &alen);
    return memmem(atxt, alen, txt, len) != NULL;
}

static inline
void*
xmalloc(size_t sz){
    void* p = malloc(sz);
    if(!p) abort();
    return p;
}

static inline
void*
xrealloc(void* p, size_t sz){
    p = realloc(p, sz);
    if(!p) abort();
    return p;
}

static inline
char*
xstrdup(const char* txt){
#ifdef _WIN32
    char* p = _strdup(txt);
#else
    char* p = strdup(txt);
#endif
    if(!p) abort();
    return p;
}

static inline
char*
xstrdup2(const char* txt, size_t len){
    char* p = xmalloc(len+1);
    memcpy(p, txt, len);
    p[len] = 0;
    return p;
}

static inline
char*
__attribute__((format(printf,1, 2)))
xsprintf(const char* fmt, ...){
    va_list va;
    va_start(va, fmt);
    char* result = NULL;
    int n = vasprintf(&result, fmt, va);
    va_end(va);
    if(result == NULL || n < 0)
        abort();
    return result;
}


static inline
DrspAtom
xatomize(const char* txt, size_t len){
    DrspAtom result = drsp_atomize(CTX, txt, len);
    if(!result) abort();
    return result;
}

static inline
DrspAtom
__attribute__((format(printf,1, 2)))
xaprintf(const char* fmt, ...){
    va_list va;
    va_start(va, fmt);
    char* p = NULL;
    int n = vasprintf(&p, fmt, va);
    va_end(va);
    if(p == NULL || n < 0)
        abort();
    DrspAtom atom = xatomize(p, n);
    if(!atom) abort();
    free(p);
    return atom;
}

static inline
int
imin(int a, int b){
    if(b < a) return b;
    return a;
}

static inline
int
imax(int a, int b){
    if(b > a) return b;
    return a;
}

static inline
void*
array_resize(void* data, size_t* cap, size_t T_sz){
    size_t new_cap = *cap?*cap*2:2;
    void* p = xrealloc(data, new_cap*T_sz);
    *cap = new_cap;
    return p;
}

static inline
void*
array_ensure(void*  data, size_t* cap, size_t count, size_t T_sz){
    if(count < *cap) return data;
    return array_resize(data, cap, T_sz);
}

#define append(array) \
    ((array)->data=array_ensure((array)->data, &(array)->capacity, (array)->count, sizeof *(array)->data), \
     &(array)->data[(array)->count++])

static const char* LOGFILE = NULL;
static FILE* LOGFILE_FP = NULL;
static inline
void
__attribute__((format(printf,1, 2)))
LOG(const char* fmt, ...){
    if(!LOGFILE) return;
#ifdef _WIN32
    if(!LOGFILE_FP) LOGFILE_FP = fopen(LOGFILE, "w");
#else
    if(!LOGFILE_FP) LOGFILE_FP = fopen(LOGFILE, "wbe");
#endif
    if(!LOGFILE_FP) return;

    va_list va;
    va_start(va, fmt);
    vfprintf(LOGFILE_FP, fmt, va);
    va_end(va);
    fflush(LOGFILE_FP);
}


enum {
    CTRL_A = 1,         // Ctrl-a
    CTRL_B = 2,         // Ctrl-b
    CTRL_C = 3,         // Ctrl-c
    CTRL_D = 4,         // Ctrl-d
    CTRL_E = 5,         // Ctrl-e
    CTRL_F = 6,         // Ctrl-f
    // CTRL_G = 7,      // unused
    CTRL_H = 8,         // Ctrl-h
    TAB    = 9,         // Tab / Ctrl-i
    CTRL_J = 10,        // Accept
    CTRL_K = 11,        // Ctrl-k
    CTRL_L = 12,        // Ctrl-l
    ENTER = 13,         // Enter / Ctrl-m
    CTRL_N = 14,        // Ctrl-n
    CTRL_O = 15,        // Ctrl-o
    CTRL_P = 16,        // Ctrl-p
    // CTRL_Q = 17,
    CTRL_R = 18,
    // CTRL_S = 19,
    CTRL_T = 20,        // Ctrl-t
    CTRL_U = 21,        // Ctrl-u
    CTRL_V = 22,        // Ctrl-v
    CTRL_W = 23,        // Ctrl-w
    // CTRL_X = 24,
    // CTRL_Y = 25,
    CTRL_Z = 26,        // Ctrl-z
    ESC = 27,           // Escape
    BACKSPACE =  127,   // Backspace
    // fake key codes
    #undef DELETE
    DELETE    = -1,
    UP        = -2,
    DOWN      = -3,
    LEFT      = -4,
    RIGHT     = -5,
    HOME      = -6,
    END       = -7,
    SHIFT_TAB = -8,
    PAGE_UP   = -9,
    PAGE_DOWN = -10,
    LCLICK_DOWN = -11,
    LCLICK_UP = -12,
    LDRAG = -13,
};
typedef struct TermState TermState;

struct TermState {
#ifdef _WIN32
    char pad;
#else
    struct termios raw;
    struct termios orig;
#endif
};

enum {
    DRAW_NONE    = 0x0,
    DRAW_HEADERS = 0x1,
    DRAW_LINES   = 0x2,
    DRAW_CELLS   = 0x4,
};
static TermState TS;
static int needs_rescale = 1;
static int needs_recalc = 1;
static int MODE = 0; // MOVE_MODE
static char* status = NULL;
enum {
    MOVE_MODE = 0,
    INSERT_MODE,
    CHANGE_MODE,
    CHANGE_SINGLE_MODE,
    COMMAND_MODE,
    QUERY_MODE,
    SEARCH_MODE,
    SELECT_MODE,
    SELECT_COMMAND_MODE,
    LINE_SELECT_MODE,
    DRAG_SELECT_MODE,
};
enum {DEFAULT_WIDTH=8};
enum {DRSP_TUI_MAX_FILL_WIDTH=32};
enum {DRSP_TUI_MAX_COLUMNS=4000};

static
void
set_status(const char* fmt, ...){
    free(status);
    va_list va;
    va_start(va, fmt);
    int n = vasprintf(&status, fmt, va);
    va_end(va);
    if(n < 0 || status == NULL)
        abort();
}
typedef struct SheetView SheetView;
static void redisplay(SheetView*);

static
void
change_mode(int m){
    if(m != MOVE_MODE && m != SELECT_MODE && m != LINE_SELECT_MODE && m != DRAG_SELECT_MODE){
        printf("\033[?25h\033[6 q");
    }
    else {
        printf("\033[?25l\033[2 q");
    }
    fflush(stdout);
    MODE = m;
}

static
const char*
mode_name(int mode){
    switch(mode){
        case MOVE_MODE:           return "MOVE";
        case INSERT_MODE:         return "EDIT";
        case CHANGE_MODE:         return "CHANGE";
        case CHANGE_SINGLE_MODE:  return "CHANGE (single)";
        case COMMAND_MODE:        return "COMMAND";
        case QUERY_MODE:          return "QUERY";
        case SEARCH_MODE:         return "SEARCH";
        case SELECT_MODE:         return "SELECT";
        case LINE_SELECT_MODE:    return "SELECT LINE";
        case DRAG_SELECT_MODE:    return "SELECT (drag)";
        case SELECT_COMMAND_MODE: return "SELECT (command)";
        default:                  return "????";
    }
}


static
void
recalc(void){
    needs_recalc = 1;
}

typedef struct RowValue RowValue;
struct RowValue {
    DrspAtom atom;
    double number;
};

typedef struct Row Row;
struct Row {
    RowValue* data;
    size_t count, capacity;
};

typedef struct Rows Rows;
struct Rows {
    Row* data;
    size_t count, capacity;
};

typedef struct Pasteboard Pasteboard;
struct Pasteboard {
    Rows rows;
    _Bool line_paste: 1;
};
static
Pasteboard PASTEBOARD;

static
void
set_rc_val_a(Rows* rows, int y, int x, DrspAtom a){
    if(y >= rows->count && a == NIL_ATOM) return;
    while(y >= rows->count)
        *append(rows) = (Row){0};
    Row* row = &rows->data[y];
    if(x >= row->count && a == NIL_ATOM) return;
    while(x >= row->count)
        *append(row) = (RowValue){NIL_ATOM, NAN};
    row->data[x] = (RowValue){a, NAN};
}

static
DrspAtom
set_rc_val(Rows* rows, int y, int x, const char* value, size_t len){
    DrspAtom a = xatomize(value, len);
    set_rc_val_a(rows, y, x, a);
    return a;
}

typedef struct TextChunk TextChunk;
struct TextChunk {
    DrspAtom atom;
    const char* _txt;
    size_t _byte_len;
    size_t vis_width;
};

static
TextChunk
get_rc_val(Rows* rows, int y, int x){
    TextChunk result = {NIL_ATOM, NULL, 0, 0};
    if(y < 0 || x < 0) return result;
    if(y >= rows->count) return result;
    Row* row = &rows->data[y];
    if(x >= row->count) return result;
    result.atom = row->data[x].atom;
    result._txt = drsp_atom_get_str(CTX, result.atom, &result._byte_len);

    // XXX vis width is wrong
    result.vis_width = result._byte_len;
    return result;
}

static
DrspAtom
get_rc_a(Rows* rows, int y, int x){
    DrspAtom result = NIL_ATOM;
    if(y < 0 || x < 0) return result;
    if(y >= rows->count) return result;
    Row* row = &rows->data[y];
    if(x >= row->count) return result;
    result = row->data[x].atom;
    return result;
}

typedef struct Column Column;
struct Column {
    char* name;
    int width;
    int explicit_width;
    _Bool hidden;
};

typedef struct Columns Columns;
struct Columns {
    Column* data;
    size_t count, capacity;
};

enum UndoType {
    UNDO_INVALID,
    UNDO_CHANGE_CELL,
    UNDO_NESTED,
    UNDO_DELETE_ROWS,
    UNDO_INSERT_ROWS,
};

typedef enum UndoType UndoType;

typedef struct Undoable Undoable;
struct Undoable {
    UndoType kind;
    union {
        // UNDO_CHANGE_CELL
        struct {
            int y, x;
            DrspAtom before;
            DrspAtom after;
        } change_cell;
        // UNDO_NESTED
        struct {
            Undoable* data;
            size_t count, capacity;
        } nested;
        // UNDO_DELETE_ROWS
        struct {
            int y, n;
        } delete_rows;
        struct {
            int y, n;
        } insert_rows;
    };
};

typedef struct UndoStack UndoStack;
struct UndoStack {
    Undoable* data;
    size_t count, capacity;
    size_t cursor;
};

static
void
cleanup_undoable(Undoable* undoable){
    switch(undoable->kind){
        case UNDO_INVALID:
            abort();
            break;
        case UNDO_CHANGE_CELL:
            break;
        case UNDO_NESTED:
            for(size_t i = 0; i < undoable->nested.count; i++){
                cleanup_undoable(&undoable->nested.data[i]);
            }
            free(undoable->nested.data);
            break;
        case UNDO_DELETE_ROWS:
            break;
        case UNDO_INSERT_ROWS:
            break;
    }
}

static
void
truncate_undos(UndoStack* undos){
    for(size_t i = undos->cursor; i < undos->count; i++){
        cleanup_undoable(&undos->data[i]);
    }
    undos->count = undos->cursor;
}

static
void
note_cell_change(UndoStack* undos, int y, int x, DrspAtom before, DrspAtom after){
    if(after == before) return;
    truncate_undos(undos);

    *append(undos) = (Undoable){
        .kind = UNDO_CHANGE_CELL,
        .change_cell = {
            .y = y, .x=x,
            .before=before, .after=after,
        },
    };
    undos->cursor++;
}

static
Undoable*
note_nested_change(UndoStack* undos){
    truncate_undos(undos);
    undos->cursor++;
    Undoable* result = append(undos);
    *result = (Undoable){ .kind = UNDO_NESTED, };
    return result;
}

static
void
push_undo_event(UndoStack* undos, Undoable ud){
    truncate_undos(undos);
    undos->cursor++;
    *append(undos) = ud;
}

static
Undoable*
note_delete_row(UndoStack* undos){
    truncate_undos(undos);
    undos->cursor++;
    Undoable* result = append(undos);
    *result = (Undoable){ .kind = UNDO_NESTED, };
    return result;
}

static
void
note_insert_rows(UndoStack* undos, int y, int n){
    truncate_undos(undos);
    undos->cursor++;
    *append(undos) = (Undoable){
        .kind = UNDO_INSERT_ROWS,
        .insert_rows = { .y=y, .n=n, },
    };
}

typedef struct _sheet_handle Sheet;
struct _sheet_handle {
    intptr_t rows, cols;
    Rows data;
    Rows disp;
    Columns columns;
    const char* filename;
    const char* name;
    UndoStack undo_stack;
};

typedef struct SheetView SheetView;
struct SheetView {
    Sheet* sheet;
    int base_x, base_y; // in cells
    int cell_x, cell_y; // in cells
    int sel_x, sel_y; // in cells
    int rows, cols;
    _Bool needs_redisplay: 1;
    int win_x, win_y;
};


static
void
redisplay(SheetView* view){
    view->needs_redisplay = 1;
}
typedef struct LineEdit LineEdit;
struct LineEdit {
    char prev_search[1024];
    int buff_cursor;
    int buff_len;
    char buff[1024];
};

static
LineEdit EDIT;

typedef struct Sheets Sheets;
struct Sheets {
    Sheet** data;
    size_t count, capacity;
};
static
Sheets SHEETS = {0};

static
Sheet*
find_sheet(const char* name){
    for(size_t i = 0; i < SHEETS.count; i++){
        if(streq(SHEETS.data[i]->name, name))
            return SHEETS.data[i];
    }
    return NULL;
}

static
Sheet*
new_sheet(const char* name, const char* filename){
    assert(!find_sheet(name));
    Sheet* result = xmalloc(sizeof *result);
    memset(result, 0, sizeof *result);
    result->name = xstrdup(name);
    if(filename) result->filename = xstrdup(filename);
    *append(&SHEETS) = result;
    return result;
}

static
struct {
    SheetView** data;
    size_t count, capacity;
} VIEWS = {0};

static
SheetView*
new_view(Sheet* sheet){
    SheetView* view = xmalloc(sizeof *view);
    memset(view, 0, sizeof * view);
    view->sheet = sheet;
    view->sel_x = -1;
    view->sel_y = -1;
    view->rows = -1;
    view->cols = -1;
    view->needs_redisplay = 1;
    *append(&VIEWS) = view;
    return view;
}

static
SheetView* _Nullable
find_view(Sheet* sheet){
    for(size_t i = 0; i < VIEWS.count; i++){
        if(VIEWS.data[i]->sheet == sheet)
            return VIEWS.data[i];
    }
    return NULL;
}

static
SheetView*
find_or_new_view(Sheet* sheet){
    SheetView* view = find_view(sheet);
    if(!view) view = new_view(sheet);
    return view;
}

static
void
clear(Sheet* sheet){
    for(int y = 0; y < sheet->data.count; y++){
        Row* row = &sheet->data.data[y];
        for(int x = 0; x < row->count; x++){
            if(row->data[x].atom != NIL_ATOM){
                row->data[x] = (RowValue){NIL_ATOM, NAN};
                drsp_set_cell_atom(CTX, sheet, y, x, NIL_ATOM);
            }
        }
    }
    recalc();
}

static
ptrdiff_t
int_to_colname(char* buff, size_t buffsz, int x){
    assert(buffsz > 1);
    char* end = buff+buffsz;
    char* p = end-1;
    *p = 0;
    // 0 -> a
    // 26 -> aa

    // a-z ->   0-25
    // aa-az -> 26-51
    // ba-bz -> 52-77
    do {
        p--;
        int c = x % 26;
        *p = (char)('A' + c);
        x/=26;
    }while(x-- && p != buff);
    memmove(buff, p, end-p);
    return end-p;
}
static
char*
xint_to_colname(int x){
    char buff[64];
    int n = int_to_colname(buff, sizeof buff, x);
    assert(n < sizeof buff);
    return xstrdup(buff);
}

static void fill_cols(Sheet* sheet, int x);
static
void
update_cell_no_eval_a(Sheet* sheet, int y, int x, DrspAtom a){
    set_rc_val_a(&sheet->data, y, x, a);
    int e;
    e = drsp_set_cell_atom(CTX, sheet, y, x, a);
    (void)e;
    fill_cols(sheet, x);
}

static
void
update_cell_a(Sheet* sheet, int y, int x, DrspAtom a){
    update_cell_no_eval_a(sheet, y, x, a);
    recalc();
}

static
void
insert_row(Sheet* sheet, int y, int n);

static
void
delete_row(Sheet* sheet, int y, int n);

enum PasteKind {
    PASTE_NORMAL = 0,
    PASTE_BELOW = 1,
    PASTE_ABOVE = 2,
    PASTE_REPLACE = 3,
};

typedef enum PasteKind PasteKind;

static
void
paste_rows(SheetView* view, const Rows* rows, int y, int x, PasteKind line_paste){
    Sheet* sheet = view->sheet;
    // LOG("line_paste: %d\n", (int)line_paste);
    Undoable* ud = NULL;
    switch(line_paste){
        case PASTE_NORMAL:
            for(int dy = 0; dy < rows->count; dy++){
                const Row* row = &rows->data[dy];
                for(int dx = 0; dx < row->count; dx++){
                    DrspAtom before = get_rc_a(&sheet->data, y+dy, x+dx);
                    if(before == row->data[dx].atom) continue;
                    goto did_change;
                }
            }
            return;
            did_change:;
            ud = note_nested_change(&sheet->undo_stack);
            break;
        case PASTE_BELOW:
            ud = note_nested_change(&sheet->undo_stack);
            y++;
            *append(&ud->nested) = (Undoable){
                .kind = UNDO_INSERT_ROWS,
                .insert_rows = { .y=y, .n=rows->count, },
            };
            insert_row(sheet, y, rows->count);
            break;
        case PASTE_ABOVE:
            ud = note_nested_change(&sheet->undo_stack);
            *append(&ud->nested) = (Undoable){
                .kind = UNDO_INSERT_ROWS,
                .insert_rows = { .y=y, .n=rows->count, },
            };
            insert_row(sheet, y, rows->count);
            break;
        case PASTE_REPLACE:{
            int h = imax(view->cell_y, view->sel_y) - imin(view->cell_y, view->sel_y) + 1;
            assert(h > 0);
            int diff = (int)rows->count - h;
            // LOG("h: %d\n", h);
            // LOG("diff: %d\n", diff);
            if(diff < 0){
                diff = -diff;
                ud = note_nested_change(&sheet->undo_stack);
                for(int dy = 0; dy < diff; dy++){
                    int iy = y + dy;
                    if(iy >= sheet->data.count) break;
                    const Row* row = &sheet->data.data[iy];
                    for(size_t x = 0; x < row->count; x++){
                        if(row->data[x].atom != NIL_ATOM){
                            *append(&ud->nested) = (Undoable){
                                .kind = UNDO_CHANGE_CELL,
                                .change_cell = {.y=iy, .x=x, .before=row->data[x].atom, .after=NIL_ATOM},
                            };
                        }
                    }
                }
                *append(&ud->nested) = (Undoable){
                    .kind = UNDO_DELETE_ROWS,
                    .delete_rows = {.y=y, .n=diff,},
                };
                delete_row(sheet, y, diff);
            }
            else if(diff > 0){
                ud = note_nested_change(&sheet->undo_stack);
                *append(&ud->nested) = (Undoable){
                    .kind = UNDO_INSERT_ROWS,
                    .insert_rows = { .y=y, .n=diff, },
                };
                // LOG("insert_row(%d, %d)\n", y, diff);
                insert_row(sheet, y, diff);
            }
            else
                ud = note_nested_change(&sheet->undo_stack);
        }break;
    }
    for(int dy = 0; dy < rows->count; dy++){
        const Row* row = &rows->data[dy];
        for(int dx = 0; dx < row->count; dx++){
            DrspAtom before = get_rc_a(&sheet->data, y+dy, x+dx);
            if(before == row->data[dx].atom) continue;
            update_cell_no_eval_a(sheet, y+dy, x+dx, row->data[dx].atom);
            DrspAtom after = row->data[dx].atom;
            *append(&ud->nested) = (Undoable){
                .kind = UNDO_CHANGE_CELL,
                .change_cell = {.y=y+dy, .x=x+dx, .before=before, .after=after},
            };
        }
    }
    recalc();
}

static
void
cleanup_row(Row* deleted){
    free(deleted->data);
}

static
void
copy_cells(Sheet* sheet, int y, int x, int h, int w){
    while(PASTEBOARD.rows.count > h){
        Row* last = &PASTEBOARD.rows.data[PASTEBOARD.rows.count-1];
        cleanup_row(last);
        PASTEBOARD.rows.count--;
    }
    while(PASTEBOARD.rows.count < h){
        *append(&PASTEBOARD.rows) = (Row){0};
    }
    for(int iy = 0; iy < h; iy++){
        PASTEBOARD.rows.data[iy].count = 0;
        for(int ix = 0; ix < w; ix++){
            DrspAtom a = get_rc_a(&sheet->data, y+iy, x+ix);
            *append(&PASTEBOARD.rows.data[iy]) = (RowValue){a, NAN};
        }
    }
    PASTEBOARD.line_paste = 0;
}

static
void
copy_rows(Sheet* sheet, int y, int h){
    int w = 0;
    for(size_t iy = y; iy < y+h && iy < sheet->data.count; iy++){
        const Row* row = &sheet->data.data[iy];
        if(row->count > w)
            w = row->count;
    }
    copy_cells(sheet, y, 0, h, w);
    PASTEBOARD.line_paste = 1;
}

static
void
delete_cells(Sheet* sheet, int y, int x, int h, int w){
    copy_cells(sheet, y, x, h, w);
    for(int dy = 0; dy < h; dy++){
        for(int dx = 0; dx < w; dx++){
            DrspAtom a  = get_rc_a(&sheet->data, y+dy, x+dx);
            if(a == NIL_ATOM) continue;
            goto did_change;
        }
    }
    return;
    did_change:;
    Undoable* u = note_nested_change(&sheet->undo_stack);
    for(int dy = 0; dy < h; dy++){
        for(int dx = 0; dx < w; dx++){
            DrspAtom before = get_rc_a(&sheet->data, y+dy, x+dx);
            if(before == NIL_ATOM) continue;
            update_cell_a(sheet, y+dy, x+dx, NIL_ATOM);
            DrspAtom after = NIL_ATOM;
            *append(&u->nested) = (Undoable){
                .kind = UNDO_CHANGE_CELL,
                .change_cell = {
                    .y = y+dy,
                    .x = x+dx,
                    .before = before,
                    .after = after,
                },
            };
        }
    }
    recalc();
}

static
void
paste(SheetView* view, int y, int x, PasteKind pastekind){
    paste_rows(view, &PASTEBOARD.rows, y, x, pastekind);
}

static
void
change_col_width(Sheet* sheet, int x, int dw){
    fill_cols(sheet, x);
    Column* col = &sheet->columns.data[x];
    if(!col->explicit_width){
        // XXX: can go too low
        col->explicit_width = col->width;
    }
    int w = col->explicit_width+dw;
    if(w <= 0) return;
    col->explicit_width = w;
    col->width = w;
}


static
void
change_col_width_fill(Sheet* sheet, int x){
    if(x >= sheet->columns.count)
        return;
    const Column* col = &sheet->columns.data[x];
    ssize_t w = -1;
    for(size_t iy = 0; iy < sheet->disp.count; iy++){
        const Row* row = &sheet->disp.data[iy];
        if(x >= row->count)
            continue;
        size_t len;
        drsp_atom_get_str(CTX, row->data[x].atom, &len);
        if(!len) continue;
        len += 2;
        if((ssize_t)len > w) w = (ssize_t)len;
    }
    if(w == -1 && col->name)
        w = strlen(col->name) + 2;
    if(w == -1)
        w = DEFAULT_WIDTH;
    if(w > DRSP_TUI_MAX_FILL_WIDTH)
        w = DRSP_TUI_MAX_FILL_WIDTH;
    if(col->width == w)
        return;
    change_col_width(sheet, x, w-col->width);
}

static void
fill_cols(Sheet* sheet, int x){
    while(x >= sheet->columns.count){
        size_t i = sheet->columns.count;
        Column* col = append(&sheet->columns);
        *col = (Column){0};
        col->name = xint_to_colname((int)i);
        col->width = 8;
    }
}

static
void
hide_columns(Sheet* sheet, int x0, int len){
    fill_cols(sheet, x0+len);
    for(int x = x0; x < x0 + len; x++){
        Column* col = &sheet->columns.data[x];
        col->hidden = 1;
    }
}
static
void
unhide_columns(Sheet* sheet){
    for(size_t i = 0; i < sheet->columns.count; i++){
        sheet->columns.data[i].hidden = 0;
    }
}

static
void
rename_column(Sheet* sheet, int x, const char* p){
    char* name = xstrdup(p);
    fill_cols(sheet, x);
    Column* col = &sheet->columns.data[x];
    free(col->name);
    col->name = name;
    int w = 2+(int)strlen(name); // XXX: unicode
    if(col->width < w || !col->explicit_width) col->width = w;
    int err = drsp_set_col_name(CTX, sheet, x, name, strlen(name));
    (void)err;
    recalc();
}

static
void
line_select_bounds(Sheet* sheet, int* x0, int* x1){
    *x0 = 0;
    size_t max = 0;
    for(size_t i = 0; i < sheet->data.count; i++)
        if(sheet->data.data[i].count > max)
            max = sheet->data.data[i].count;
    *x1 = max;
}

static
void
scroll(SheetView* view, int dx, int dy){
    view->base_y += dy;
    view->base_x += dx;
    if(view->base_y < 0) view->base_y = 0;
    if(view->base_x < 0) view->base_x = 0;
    if(view->base_x >= DRSP_TUI_MAX_COLUMNS) view->base_x = DRSP_TUI_MAX_COLUMNS-1;
    view->cell_y += dy;
    view->cell_x += dx;
    if(view->cell_y < 0) view->cell_y = 0;

    if(view->cell_x < 0) view->cell_x = 0;
    if(view->cell_x >= DRSP_TUI_MAX_COLUMNS) view->cell_x = DRSP_TUI_MAX_COLUMNS-1;
    redisplay(view);
}

static
void
move(SheetView* view, int dx, int dy){
    view->cell_y += dy;
    view->cell_x += dx;
    if(view->cell_y < 0) view->cell_y = 0;

    if(view->cell_x < 0) view->cell_x = 0;
    if(view->cell_x >= DRSP_TUI_MAX_COLUMNS) view->cell_x = DRSP_TUI_MAX_COLUMNS-1;
    if(dx < 0 && view->cell_x < view->sheet->columns.count){
        ssize_t x = view->cell_x;
        while(x >= 0 && view->sheet->columns.data[x].hidden)
            x--;
        if(x < 0){
            x = view->cell_x;
            while(x < view->sheet->columns.count && view->sheet->columns.data[x].hidden)
                x++;
        }
        view->cell_x = x;
    }
    else if (dx > 0){
        ssize_t x = view->cell_x;
        while(x < view->sheet->columns.count && view->sheet->columns.data[x].hidden)
            x++;
        view->cell_x = x;
    }
    redisplay(view);
}

static
void
begin_line_edit_buff(const char* txt, size_t len){
    if(!txt) txt = "";
    assert(len < sizeof(EDIT.buff));
    memcpy(EDIT.buff, txt, len);
    EDIT.buff[len] = 0;
    EDIT.buff_cursor = len;
    EDIT.buff_len = len;
}

static
void
begin_line_edit(SheetView* view){
    if(MODE == CHANGE_MODE || MODE == CHANGE_SINGLE_MODE)
        begin_line_edit_buff("", 0);
    else {
        DrspAtom a = get_rc_a(&view->sheet->data, view->cell_y, view->cell_x);
        size_t len;
        const char* txt = drsp_atom_get_str(CTX, a, &len);
        begin_line_edit_buff(txt, len);
    }
}

static
void
insert_text(const char* txt, size_t n){
    if(EDIT.buff_len >= sizeof(EDIT.buff)-n)
        return;
    if(EDIT.buff_cursor != EDIT.buff_len){
        memmove(EDIT.buff+EDIT.buff_cursor+n, EDIT.buff+EDIT.buff_cursor, EDIT.buff_len-EDIT.buff_cursor);
    }
    for(size_t i = 0; i < n; i++){
        EDIT.buff[EDIT.buff_cursor++] = txt[i];
        EDIT.buff_len++;
    }
    EDIT.buff[EDIT.buff_len] = 0;
}

static
void
handle_edit_key(int c){
    switch(c){
        case HOME:
        case CTRL_A:
            EDIT.buff_cursor = 0;
            break;
        case END:
        case CTRL_E:
            EDIT.buff_cursor = EDIT.buff_len;
            break;
        case LEFT:
        case CTRL_B:
            if(EDIT.buff_cursor)
                EDIT.buff_cursor--;
            break;
        case RIGHT:
        case CTRL_F:
            if(EDIT.buff_cursor < EDIT.buff_len)
                EDIT.buff_cursor++;
            break;
        case CTRL_H:
        case BACKSPACE:
            if(EDIT.buff_cursor){
                EDIT.buff_cursor--;
                EDIT.buff_len--;
                if(EDIT.buff_cursor != EDIT.buff_len){
                    memmove(EDIT.buff+EDIT.buff_cursor, EDIT.buff+EDIT.buff_cursor+1, EDIT.buff_len-EDIT.buff_cursor);
                }
                EDIT.buff[EDIT.buff_len] = 0;
            }
            break;
        case CTRL_D:
        case DELETE:
            if(EDIT.buff_len && EDIT.buff_cursor != EDIT.buff_len){
                EDIT.buff_len--;
                if(EDIT.buff_cursor < EDIT.buff_len){
                    memmove(EDIT.buff+EDIT.buff_cursor, EDIT.buff+EDIT.buff_cursor+1, EDIT.buff_len-EDIT.buff_cursor);
                }
                EDIT.buff[EDIT.buff_len] = 0;
            }
            break;
        case CTRL_K:
            EDIT.buff_len = EDIT.buff_cursor;
            EDIT.buff[EDIT.buff_len] = 0;
            break;
        default:
            if(isprint(c)){
                char c_ = (char)(unsigned char)(unsigned)c;
                insert_text(&c_, 1);
            }
            else {
                LOG("unhandled: %#x\n", c);
            }
            break;
    }
}

static
void
draw_grid(SheetView* view){
    Sheet* sheet = view->sheet;
    int advance = 12;
    drt_move(drt, 0, 0);
    drt_clear_to_end_of_row(drt);
    const int borderless = !DRAW_BORDERS;
    for(int x = 5, ix=view->base_x; x < view->cols;x+=advance, ix++){
        const char* colname;
        char buff[12];
        size_t len;
        int width = DEFAULT_WIDTH;
        if(ix < sheet->columns.count){
            const Column* col = &sheet->columns.data[ix];
            if(col->hidden){
                advance = 0;
                continue;
            }
            colname = col->name;
            len = strlen(colname);
            width = col->width;
        }
        else {
            len = int_to_colname(buff, sizeof buff, ix)-1;
            colname = buff;
        }
        int lpad = len < width? (width-len)/2: 0;
        int pwidth = width - lpad;
        if(borderless){
            drt_move(drt, x-1, 0);
            drt_printf(drt, " %*s%.*s", lpad, "", pwidth, colname);
        }
        else{
            drt_move(drt, x-1, 0);
            drt_putc_mb(drt, "│", -1+sizeof "│", 1);
            drt_printf(drt, "%*s%.*s", lpad, "", pwidth, colname);
        }
        advance = width+1;
    }
    drt_move(drt, 0, 1);
    for(int i = 0; i < 4; i++) drt_putc_mb(drt, "─", -1+sizeof "─", 1);
    for(int x = 5, ix=view->base_x; x < view->cols;x+=advance, ix++){
        int width = DEFAULT_WIDTH;
        if(ix < sheet->columns.count){
            const Column* col = &sheet->columns.data[ix];
            if(col->hidden){
                advance = 0;
                continue;
            }
            width = col->width;
        }
        drt_move(drt, x-1, 1);
        for(int i = 0; i < 3*width+1; i++)
            drt_putc_mb(drt, "─", -1+sizeof "─", 1);
        advance = width+1;
    }
    int sel_x0 = view->cell_x;
    int sel_x1 = view->cell_x;
    int sel_y0 = view->cell_y;
    int sel_y1 = view->cell_y;
    if(MODE == SELECT_MODE || MODE == DRAG_SELECT_MODE || MODE == SELECT_COMMAND_MODE){
        sel_x0 = imin(view->cell_x, view->sel_x);
        sel_x1 = imax(view->cell_x, view->sel_x);
        sel_y0 = imin(view->cell_y, view->sel_y);
        sel_y1 = imax(view->cell_y, view->sel_y);
    }
    if(MODE == LINE_SELECT_MODE){
        sel_y0 = imin(view->cell_y, view->sel_y);
        sel_y1 = imax(view->cell_y, view->sel_y);
    }
    for(int iy = view->base_y, y = 3; y < view->rows-1;y++, iy++){
        drt_push_state(drt);
        drt_set_8bit_color(drt, 240);
        drt_move(drt, 0, y-1);
        drt_printf(drt, "%4d", iy+1);
        drt_pop_state(drt);
        for(int ix = view->base_x, x = 5; x < view->cols;x+=advance, ix++){
            int width = DEFAULT_WIDTH;
            if(ix < sheet->columns.count){
                const Column* col = &sheet->columns.data[ix];
                if(col->hidden){
                    advance = 0;
                    continue;
                }
                width = col->width;
            }
            _Bool selected =
                ((MODE == SELECT_MODE || MODE == DRAG_SELECT_MODE || MODE == SELECT_COMMAND_MODE)
                 && ix >= sel_x0
                 && ix <= sel_x1
                 && iy >= sel_y0
                 && iy <= sel_y1)
                ||
                (MODE == LINE_SELECT_MODE
                 && iy >= sel_y0
                 && iy <= sel_y1);

            drt_push_state(drt);
            if(selected){
                drt_set_style(drt, DRT_STYLE_BOLD);
                drt_bg_set_8bit_color(drt, 8);
                if(ix == view->cell_x && iy == view->cell_y){
                    drt_set_style(drt, DRT_STYLE_UNDERLINE|DRT_STYLE_BOLD);
                    drt_set_8bit_color(drt, 4);
                }
            }
            else if(ix == view->cell_x && iy == view->cell_y){
                drt_set_style(drt, DRT_STYLE_UNDERLINE|DRT_STYLE_BOLD);
                drt_set_8bit_color(drt, 12);
            }
            if(borderless){
                drt_move(drt, x-1, y-1);
                drt_putc(drt, ' ');
            }
            else{
                drt_move(drt, x-1, y-1);
                _Bool pushed = 0;
                if((MODE==SELECT_MODE || MODE==DRAG_SELECT_MODE || MODE == SELECT_COMMAND_MODE) && ix-1 >= sel_x0 && ix-1 <= sel_x1 && iy >= sel_y0 && iy <= sel_y1){
                    pushed = 1;
                    drt_push_state(drt);
                    drt_bg_set_8bit_color(drt, 8);
                    if(ix-1==view->cell_x && iy == view->cell_y){
                        drt_set_style(drt, DRT_STYLE_UNDERLINE|DRT_STYLE_BOLD);
                        drt_set_8bit_color(drt, 4);
                    }
                }
                else if(ix-1==view->cell_x && iy == view->cell_y){
                    pushed = 1;
                    drt_push_state(drt);
                    drt_set_style(drt, DRT_STYLE_UNDERLINE|DRT_STYLE_BOLD);
                    drt_set_8bit_color(drt, 12);
                }
                drt_putc_mb(drt, "│", -1+sizeof "│", 1);
                if(pushed) drt_pop_state(drt);
            }
            TextChunk chunk = get_rc_val(&sheet->disp, iy, ix);
            if(chunk.vis_width < width){
                drt_printf(drt, "%*.*s ", width-1, imin((int)chunk._byte_len,width-1), chunk._txt);
            }
            else{
                drt_printf(drt, "%*.*s", width, imin((int)chunk._byte_len, width), chunk._txt);
            }

            drt_pop_state(drt);

            advance = width+1;
        }
    }
}


static
void
enable_raw(TermState* ts){
#ifdef _WIN32
    (void)ts;
#else
    if(tcgetattr(STDIN_FILENO, &ts->orig) == -1)
        return;
    ts->raw = ts->orig;
    ts->raw.c_iflag &= ~(0lu
            | BRKINT // no break
            | ICRNL  // don't map CR to NL
            | INPCK  // skip parity check
            | ISTRIP // don't strip 8th bit off char
            | IXON   // don't allow start/stop input control
            );
    ts->raw.c_oflag &= ~(0lu
        | OPOST // disable post processing
        );
    ts->raw.c_cflag |= CS8; // 8 bit chars
    ts->raw.c_lflag &= ~(0lu
            | ECHO    // disable echo
            | ICANON  // disable canonical processing
            | IEXTEN  // no extended functions
            // Currently allowing these so ^Z works, could disable them
            | ISIG    // disable signals
            );
    ts->raw.c_cc[VMIN] = 1; // read every single byte
    ts->raw.c_cc[VTIME] = 0; // no timeout

    // set and flush
    // Change will ocurr after all output has been transmitted.
    // Unread input will be discarded.
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts->raw) < 0)
        return;
#endif
}

static
void
disable_raw(struct TermState*ts){
#ifdef _WIN32
    (void)ts;
#else
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts->orig);
#endif
}
#ifdef _WIN32
static HANDLE STDIN;
static HANDLE STDOUT;
#endif

static inline
ssize_t
read_one(char* buff, _Bool block){
#ifdef _WIN32
    for(;;){
        INPUT_RECORD record;
        DWORD num_read = 0;
        if(!block){
            DWORD timeout = 0;
            DWORD ev = WaitForSingleObject(STDIN, timeout);
            if(ev == WAIT_TIMEOUT){
                // LOG("WAIT_TIMEOUT\n");
                *buff = 0;
                return 0;
            }
            if(ev == WAIT_FAILED)
                return -1;
            if(ev != WAIT_OBJECT_0){
                if(0)LOG("%d WaitForSingleObject returned value other than TIMEOUT, OBJECT_FAILED OR OBJECT_O: %lu\n", __LINE__, ev);
                continue;
            }
        }
        // LOG("ReadConsoleInput\n");
        BOOL b = ReadConsoleInput(STDIN, &record, 1, &num_read);
        // LOG("DoneReadConsoleInput\n");
        if(!b) return -1;
        if(record.EventType == WINDOW_BUFFER_SIZE_EVENT){
            needs_rescale = 1;
            if(!block) continue;
            *buff = 0;
            if(0)LOG("window size event: %hd, %hd", record.Event.WindowBufferSizeEvent.dwSize.X, record.Event.WindowBufferSizeEvent.dwSize.Y);
            return 0;
        }
        if(record.EventType != KEY_EVENT){
            // LOG("record.EventType: %d\n", (int)record.EventType);
            continue;
        }
        if(!record.Event.KeyEvent.bKeyDown){
            // LOG("!record.Event.KeyEvent.bKeyDown\n");
            continue;
        }
        *buff = record.Event.KeyEvent.uChar.AsciiChar;
        return 1;
    }
#else
    if(block){
        ssize_t e;
        for(;;){
            e = read(STDIN_FILENO, buff, 1);
            if(e == -1 && errno == EINTR) {
                if(needs_rescale){
                    *buff = 0;
                    e = 0;
                }
                else {
                    LOG("EINTR\n");
                    continue;
                }
            }
            if(e == -1){
                LOG("read_one errno: %d\n", errno);
            }
            break;
        }
        return e;
    }
    else {
        ssize_t e;
        int flags = fcntl(STDIN_FILENO, F_GETFL);
        int new_flags = flags | O_NONBLOCK;
        fcntl(STDIN_FILENO, F_SETFL, new_flags);

        for(;;){
            e = read(STDIN_FILENO, buff, 1);
            if(e == -1 && errno == EINTR) continue;
            if(e == -1 && errno == EWOULDBLOCK) {
                *buff = 0;
                e = 0;
            }
            if(e == -1){
                LOG("read_one errno: %d\n", errno);
            }
            break;
        }
        fcntl(STDIN_FILENO, F_SETFL, flags);
        return e;
    }
#endif
}

static inline
ssize_t
read_one_nb(char* buff){
    return read_one(buff, /*block=*/0);
}

static inline
ssize_t
read_one_b(char* buff){
    return read_one(buff, /*block=*/1);
}


static
void
end_tui(void){
    disable_raw(&TS);
    // show cursor
    printf("\033[?25h");
    fflush(stdout);
    // disable alt buffer
    printf("\033[?1049l");
    fflush(stdout);
    // Normal tracking mode?
    printf("\033[?1006;1002l");
    // enable line wrapping
    printf("\033[=7h");
    fflush(stdout);
}

static
void
begin_tui(void){
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleMode(STDIN, ENABLE_VIRTUAL_TERMINAL_INPUT);
    SetConsoleMode(STDOUT, ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_VIRTUAL_TERMINAL_PROCESSING|DISABLE_NEWLINE_AUTO_RETURN);
#endif
    // alternative buffer
    printf("\033[?1049h");
    fflush(stdout);
    // hide cursor
    printf("\033[?25l");
    fflush(stdout);
    // X11 Mouse Reporting
    // See https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Mouse-Tracking
    printf("\033[?1006;1002h");
    // line wrapping
    printf("\033[=7l");
    fflush(stdout);
    enable_raw(&TS);
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleMode(STDIN, ENABLE_VIRTUAL_TERMINAL_INPUT);
    SetConsoleMode(STDOUT, ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_VIRTUAL_TERMINAL_PROCESSING|DISABLE_NEWLINE_AUTO_RETURN);
#endif
}

#ifdef _WIN32
#else
static
void
sighandler(int sig){
    if(sig == SIGWINCH){
        // LOG("SIGWINCH\n");
        needs_rescale = 1;
        return;
    }
}
#endif


static
int
display_number(void* ctx, Sheet* sheet, intptr_t row, intptr_t col, double value){
    (void)ctx;

    char buff[512];
    int n = snprintf(buff, sizeof buff, "%.2f", value);
    char* dot;
    if((dot = memchr(buff, '.', n))){
        for(;n;){
            if(buff[n-1] == '.'){
                buff[--n] = 0;
                break;
            }
            else if(buff[n-1] == '0'){
                buff[--n] = 0;
            }
            else
                break;
        }
        if(!n) buff[0] = '0';
        char* p = dot;
        while(p - buff > 3 + (buff[0]=='-')){
            p -= 3;
            memmove(p+1, p, n - (p-buff));
            n++;
            buff[n] = 0;
            *p = ',';
        }
    }
    if(0)LOG("%zd, %zd: num: %.*s\n", row, col, (int)n, buff);
    set_rc_val(&sheet->disp, row, col, buff, n);
    Row* r = &sheet->disp.data[row];
    r->data[col].number = value;
    SheetView* view = find_view(sheet);
    if(view) redisplay(view);
    return 0;
}

static
int
display_error(void* ctx, Sheet* sheet, intptr_t row, intptr_t col, const char* txt, size_t len){
    (void)ctx;
    DrspAtom a = xaprintf("#ERR: %.*s", (int)len, txt);
    if(0)LOG("%zd, %zd: #ERR: %.*s\n", row, col, (int)len, txt);
    set_rc_val_a(&sheet->disp, row, col, a);
    Row* r = &sheet->disp.data[row];
    r->data[col].number = NAN;
    SheetView* view = find_view(sheet);
    if(view) redisplay(view);
    return 0;
}

static
int
display_string(void* ctx, Sheet* sheet, intptr_t row, intptr_t col, const char* txt, size_t len){
    (void)ctx;
    if(0)LOG("%zd, %zd: str: %.*s\n", row, col, (int)len, txt);
    set_rc_val(&sheet->disp, row, col, txt, len);
    Row* r = &sheet->disp.data[row];
    r->data[col].number = NAN;
    SheetView* view = find_view(sheet);
    if(view) redisplay(view);
    return 0;
}


static
SheetOps
make_ops(void){
    SheetOps ops = {
        .ctx = NULL,
        .set_display_number = display_number,
        .set_display_error = display_error,
        .set_display_string = display_string,
    };
    return ops;
}

typedef struct CellLoc CellLoc;
struct CellLoc {
    int x, y, w;
};

typedef struct CellCoord CellCoord;
struct CellCoord {
    int column, row;
};

static
CellCoord
screen_to_cell(SheetView* view, int x, int y){
    Sheet* sheet = view->sheet;
    if(x < 4)
        x = 0;
    else
        x -= 4;
    if(y < 2)
        y = 0;
    else
        y -= 2;
    int cell_y = y + view->base_y;
    int cell_x;
    int screen_x = 0;
    for(int ix = view->base_x;; ix++){
        if(screen_x > x){
            cell_x = ix-1;
            break;
        }
        int advance = DEFAULT_WIDTH+1;
        if(ix < sheet->columns.count){
            Column* col = &sheet->columns.data[ix];
            if(col->hidden)
                advance = 0;
            else
                advance = 1 + sheet->columns.data[ix].width;
        }
        screen_x += advance;
    }
    return (CellCoord){
        .column = cell_x,
        .row=cell_y,
    };

}


static
CellLoc
cell_to_screen(SheetView* view, int x, int y){
    Sheet* sheet = view->sheet;
    int screen_x = 1+4;
    for(int ix = view->base_x; ix < x; ix++){
        int advance = DEFAULT_WIDTH+1;
        if(ix < sheet->columns.count){
            Column* col = &sheet->columns.data[ix];
            if(col->hidden)
                advance = 0;
            else
                advance = 1 + sheet->columns.data[ix].width;
        }
        screen_x += advance;
    }
    int width = x >= sheet->columns.count?DEFAULT_WIDTH:sheet->columns.data[x].hidden?0:sheet->columns.data[x].width;
    CellLoc loc = {
        screen_x,
        // x*12+1+4,
        y-view->base_y+3,
        width,
    };
    // LOG("x, y: %d, %d\n", x, y);
    // LOG("loc: .x=%d, .y=%d, .w%d\n", loc.x, loc.y, loc.w);
    return loc;
}

static
void
insert_row(Sheet* sheet, int y, int n){
    if(y < 0) y = 0;
    Rows* rows = &sheet->data;
    if(y >= rows->count)
        return; // virtual cells handle this.
    // fixup the ctx's representation of our data.
    //         -->
    // 1 2 3    | 1 2 3
    // 4 5      | _ _
    // 7 8 9 10 | 4 5 _ _
    // 11 12    | 7 8 9 10
    //          | 11 12

    //       -->
    // hello   | hello
    // world   | _
    // goodbye | world
    //         | goodbye
    for(int i = 0; i < n; i++) *append(rows) = (Row){0};
    memmove(&rows->data[y+n], &rows->data[y], (rows->count-y-n) * sizeof *rows->data);
    memset(&rows->data[y], 0, n*sizeof *rows->data);
    const Row empty = {0};

    for(size_t iy = y; iy < rows->count; iy++){
        const Row* row = &rows->data[iy];
        const Row* old_row = iy+n<rows->count?&rows->data[iy+n]:&empty;
        size_t x = 0;
        for(;x < row->count; x++){
            int err = drsp_set_cell_atom(CTX, sheet, iy, x, row->data[x].atom);
            (void)err;
        }
        for(;x < old_row->count; x++){
            int err = drsp_set_cell_atom(CTX, sheet, iy, x, NIL_ATOM);
            (void)err;
        }
    }
}

static
void
insert_row_with_undo(Sheet* sheet, int y, int n){
    if(y < 0) y = 0;
    Rows* rows = &sheet->data;
    if(y >= rows->count)
        return; // virtual cells handle this.
    insert_row(sheet, y, n);
    note_insert_rows(&sheet->undo_stack, y, n);
}


static
void
delete_row(Sheet* sheet, int y, int n){
    if(y < 0) y = 0;
    Rows* rows = &sheet->data;
    if(y >= rows->count) return;
    if(y+n > rows->count){
        n = rows->count - y;
    }
    // fixup the ctx's representation of our data.
    //         -->
    // 1 2 3    | 1 2 3
    // 4 5      | 7 8 9 10
    // 7 8 9 10 | 11 12 _ _
    // 11 12    | _ _

    //       -->
    // hello   | hello
    // world   | goodbye
    // goodbye | _
    for(size_t iy = y; iy < rows->count-n; iy++){
        const Row* deleted = &rows->data[iy];
        const Row* new_row = &rows->data[iy+n];
        size_t x = 0;
        for(; x < new_row->count; x++){
            int err = drsp_set_cell_atom(CTX, sheet, iy, x, new_row->data[x].atom);
            (void)err;
        }
        for(; x < deleted->count; x++){
            int err = drsp_set_cell_atom(CTX, sheet, iy, x, NIL_ATOM);
            (void)err;
        }
    }
    for(int i = 1; i <= n; i++){
        int iy = rows->count-i;
        const Row* last = &rows->data[iy];
        for(size_t x = 0; x < last->count; x++){
            int err = drsp_set_cell_atom(CTX, sheet, iy, x, NIL_ATOM);
            (void)err;
        }
    }
    for(int i = 0; i < n; i++)
        cleanup_row(&rows->data[y+i]);
    if(y != rows->count-n)
        memmove(&rows->data[y], &rows->data[y+n], (rows->count-y-n)* sizeof *rows->data);
    rows->count -= n;
}

static
void
delete_row_with_undo(Sheet* sheet, int y, int n){
    if(y < 0) y = 0;
    Rows* rows = &sheet->data;
    if(y >= rows->count) return;
    if(y+n > rows->count){
        n = rows->count - y;
    }
    Undoable* ud = note_delete_row(&sheet->undo_stack);
    for(int dy = 0; dy < n; dy++){
        int iy = y + dy;
        assert(iy < sheet->data.count);
        const Row* row = &sheet->data.data[iy];
        for(size_t x = 0;x < row->count; x++){
            if(row->data[x].atom != NIL_ATOM)
                goto did_change;
        }
    }
    goto did_not_change;
    did_change:;
    for(int dy = 0; dy < n; dy++){
        int iy = y + dy;
        assert(iy < sheet->data.count);
        const Row* row = &sheet->data.data[iy];
        for(size_t x = 0;x < row->count; x++){
            if(row->data[x].atom != NIL_ATOM)
                *append(&ud->nested) = (Undoable){
                    .kind = UNDO_CHANGE_CELL,
                    .change_cell = {.y=iy, .x=x, .before=row->data[x].atom, .after=NIL_ATOM},
                };
        }

    }

    did_not_change:;
    *append(&ud->nested) = (Undoable){
        .kind = UNDO_DELETE_ROWS,
        .delete_rows = {.y=y, .n=n},
    };
    delete_row(sheet, y, n);
}

typedef struct SheetSortCtx SheetSortCtx;
struct SheetSortCtx {
    Sheet* sheet;
    intptr_t col;
    int asc;
};

int
sheet_sort_cmp(void* ctx_, const void* a, const void* b){
    SheetSortCtx* ctx = ctx_;
    Sheet* sheet = ctx->sheet;
    intptr_t col = ctx->col;
    int asc = ctx->asc;
    const intptr_t* li = a;
    const intptr_t* ri = b;
    const Row* l = &sheet->disp.data[*li];
    const Row* r = &sheet->disp.data[*ri];
    const RowValue empty = {NIL_ATOM, NAN};
    const RowValue* left = l->count > col? &l->data[col] : &empty;
    const RowValue* right = r->count > col? &r->data[col] : &empty;
    if(0)LOG("left->number, right->number: %f,%f\n", left->number, right->number);
    if(!isnan(left->number)){
        if(isnan(right->number))
            return -1*asc;
        if(left->number < right->number) return -1*asc;
        if(left->number > right->number) return 1*asc;
        return 0;
    }
    if(!isnan(right->number)) return 1*asc;
    if(left->atom == right->atom) return 0;
    size_t llen, rlen;
    const char *lstr, *rstr;
    lstr = drsp_atom_get_str(CTX, left->atom, &llen);
    rstr = drsp_atom_get_str(CTX, right->atom, &rlen);
    if(0)LOG("left->atom, right->atom: '%.*s','%.*s'\n", (int)llen, lstr, (int)rlen, rstr);
    if(!llen) return !rlen?0:1*asc;
    if(!rlen) return -1*asc;
    size_t slen = llen < rlen? llen: rlen;
    if(slen){
        int diff = memcmp(lstr, rstr, slen);
        if(diff) return diff*asc;
    }
    return llen < rlen? -1*asc : llen > rlen? 1*asc: 0;
}

enum {
    SORT_DESCENDING = 0x0,
    SORT_ASCENDING = 0x1,
};

static
void
sort_sheet(Sheet* sheet, intptr_t col, unsigned flags){
    LOG("sorting %zd, %s\n", col, flags & SORT_ASCENDING?"ascending":"descending");
    int asc = flags & SORT_ASCENDING?1:-1;
    SheetSortCtx ctx = {.sheet = sheet, .col=col, .asc=asc};
    intptr_t* idxes = xmalloc(2*sheet->disp.count * sizeof *idxes);
    for(intptr_t i = 0; i < sheet->disp.count; i++){
        idxes[i] = i;
    }
    {
    // uint64_t t0 = get_t();
    drp_merge_sort(idxes+sheet->disp.count, idxes, sheet->disp.count, sizeof *idxes, &ctx, &sheet_sort_cmp);
    // mergesort_b(idxes, sheet->disp.count, sizeof *idxes, ^(const void* a, const void* b){ return sheet_sort_cmp((void*)&ctx, a, b); });
    // uint64_t t1 = get_t();
    // LOG("%d merge_sort: %lluµs\n", __LINE__, t1-t0);
    // LOG("%d merge_sort: %.3fs\n", __LINE__, (t1-t0)/1e6);
    }
    {
    // uint64_t t0 = get_t();
    // Clear out old values
    for(int y = 0; y < sheet->data.count; y++){
        const Row* row = &sheet->data.data[y];
        for(int x = 0; x < row->count; x++){
            if(row->data[x].atom != NIL_ATOM){
                drsp_set_cell_atom(CTX, sheet, y, x, NIL_ATOM);
            }
        }
    }
    // uint64_t t1 = get_t();
    // LOG("%d clear old values: %lluµs\n", __LINE__, t1-t0);
    // LOG("%d clear old values: %.3fs\n", __LINE__, (t1-t0)/1e6);
    }
    if(0){
        LOG("Sort: \n");
        for(intptr_t i = 0; i < sheet->data.count; i++){
            LOG("  %zd -> %zd\n", idxes[i], i);
        }
        LOG("Before:\n");
        for(intptr_t i = 0; i < sheet->data.count; i++){
            Row r = sheet->data.data[i];
            DrspAtom a = r.count > col? r.data[col].atom: NIL_ATOM;
            size_t len; const char* txt = drsp_atom_get_str(CTX, a, &len);
            LOG("  %zd] '%.*s'\n", i, (int)len, txt);
        }
    }
    {
    // uint64_t t0 = get_t();
    for(intptr_t i = 0; i < sheet->data.count; i++){
        intptr_t dst = i;
        Row clobbered = sheet->data.data[i];
        for(;;){
            intptr_t src = idxes[dst];
            if(dst == src) break;
            if(src < 0) break;
            if(0)LOG("%zd) %zd -> %zd\n", i, src, dst);
            idxes[dst] = -1;
            Row new = src == i? clobbered: sheet->data.data[src];
            sheet->data.data[dst] = new;
            dst = src;
        }
    }
    // uint64_t t1 = get_t();
    // LOG("%d apply sort: %lluµs\n", __LINE__, t1-t0);
    // LOG("%d apply sort: %.3fs\n", __LINE__, (t1-t0)/1e6);
    }
    if(0){
        LOG("After:\n");
        for(intptr_t i = 0; i < sheet->data.count; i++){
            Row r = sheet->data.data[i];
            DrspAtom a = r.count > col? r.data[col].atom: NIL_ATOM;
            size_t len; const char* txt = drsp_atom_get_str(CTX, a, &len);
            LOG("  %zd] '%.*s'\n", i, (int)len, txt);
        }
    }
    {
    // uint64_t t0 = get_t();
    for(int y = 0; y < sheet->data.count; y++){
        const Row* row = &sheet->data.data[y];
        for(int x = 0; x < row->count; x++){
            if(row->data[x].atom != NIL_ATOM){
                drsp_set_cell_atom(CTX, sheet, y, x, row->data[x].atom);
            }
        }
    }
    // uint64_t t1 = get_t();
    // LOG("%d update cells: %lluµs\n", __LINE__, t1-t0);
    // LOG("%d update cells: %.3fs\n", __LINE__, (t1-t0)/1e6);
    }
    free(idxes);
    recalc();
}

static
char*
read_file(const char* filename){
    char* txt = NULL;
    int err = 0;
    long len = 0;
    size_t n = 0;
    FILE* fp = fopen(filename, "rb");
    if(!fp) goto fail;
    err = fseek(fp, 0, SEEK_END);
    if(err) goto fail;
    len = ftell(fp);
    if(len < 0) goto fail;
    err = fseek(fp, 0, SEEK_SET);
    if(err) goto fail;
    txt = xmalloc(len+1);
    if(!txt) goto fail;
    n = fread(txt, 1, len, fp);
    if(n != (size_t)len) goto fail;

    txt[len] = 0;
    fclose(fp);
    return txt;

    fail:
    if(txt) free(txt);
    if(fp) fclose(fp);
    return NULL;
}

static
int
atomically_write_sheet(Sheet* sheet, const char* filename){
    if(!filename){
        set_status("No filename");
        return 1;
    }
    char* tmpname = NULL;
    char* bak = NULL;
    FILE* fp = NULL;
    int result = 1;
    int delete_tmp = 0;
    tmpname = xsprintf("%s.tmp", filename);

    #ifdef _WIN32
    fp = fopen(tmpname, "wb");
    #else
    fp = fopen(tmpname, "wbxe");
    #endif
    if(!fp) {
        LOG("fopen failed: %s\n", strerror(errno));
        goto cleanup;
    }
    else
        LOG("opened tmp file: %s\n", filename);
    delete_tmp = 1;

    {
        const char* dot = strrchr(filename, '.');
        if(dot && streq(dot, ".drsp")){
            // write header
            int put;
            put = fputs("names", fp);
            if(put == EOF) {
                LOG("fputs failed: %s\n", strerror(errno));
                goto cleanup;
            }
            for(size_t i = 0; i < sheet->columns.count; i++){
                const Column* col = &sheet->columns.data[i];
                put = fputc('\t', fp);
                if(put == EOF) {
                LOG("fputc failed: %s\n", strerror(errno));
                    goto cleanup;
                }
                if(col->name) {
                    put = fputs(col->name, fp);
                    if(put == EOF) {
                        LOG("fputs failed: %s\n", strerror(errno));
                        goto cleanup;
                    }
                }
            }
            put = fputc('\n', fp);
            if(put == EOF) {
                LOG("fputc failed: %s\n", strerror(errno));
                goto cleanup;
            }
            put = fputc('\n', fp);
            if(put == EOF) {
                LOG("fputc failed: %s\n", strerror(errno));
                goto cleanup;
            }
        }
        else {
            char buff[32];
            _Bool explicit_names = 0;
            for(size_t i = 0; i < sheet->columns.count; i++){
                const Column* col = &sheet->columns.data[i];
                int_to_colname(buff, sizeof buff, i);
                if(col->name && strcmp(buff, col->name) != 0){
                    explicit_names = 1;
                    break;
                }
            }
            if(explicit_names){
                int put;
                for(size_t i = 0; i < sheet->columns.count; i++){
                    const Column* col = &sheet->columns.data[i];
                    if(i != 0) {
                        put = fputc('\t', fp);
                        if(put == EOF){
                            LOG("fputc failed: %s\n", strerror(errno));
                            goto cleanup;
                        }
                    }
                    int_to_colname(buff, sizeof buff, i);
                    if(col->name && strcmp(buff, col->name) != 0){
                        put = fputs(col->name, fp);
                        if(put == EOF) {
                            LOG("fputs failed: %s\n", strerror(errno));
                            goto cleanup;
                        }
                    }
                }
                put = fputc('\n', fp);
                if(put == EOF) {
                    LOG("fputc failed: %s\n", strerror(errno));
                    goto cleanup;
                }
            }
        }
    }

    int put;
    for(size_t y = 0; y < sheet->data.count;y++){
        Row* r = &sheet->data.data[y];
        for(size_t x = 0; x < r->count; x++){
            if(x != 0) {
                put = fputc('\t', fp);
                if(put == EOF) {
                    LOG("fputc failed: %s\n", strerror(errno));
                    goto cleanup;
                }
            }
            if(r->data[x].atom != NIL_ATOM){
                size_t len; const char* txt = drsp_atom_get_str(CTX, r->data[x].atom, &len);
                if(0)LOG("%d,%d: %.*s\n", (int)y, (int)x, (int)len, txt);
                size_t writ = fwrite(txt, len, 1, fp);
                if(writ != 1){
                    LOG("fwrite failed: %s\n", strerror(errno));
                    goto cleanup;
                }
            }
        }
        put = fputc('\n', fp);
        if(put == EOF) {
            LOG("fputc failed: %s\n", strerror(errno));
            goto cleanup;
        }
    }
    put = fclose(fp);
    fp = NULL;
    if(put == EOF) {
        LOG("fclose failed: %s\n", strerror(errno));
        goto cleanup;
    }

    int err;
    #ifdef _WIN32
    bak = xsprintf("%s.bak", filename);
    if(!bak) goto cleanup;
    LOG("backing up %s to %s\n", filename, bak);
    err = rename(filename, bak);
    if(err) {
        LOG("rename %s to %s failed\n", filename, bak);
        goto cleanup;
    }
    #endif
    LOG("renaming %s to %s\n", tmpname, filename);
    err = rename(tmpname, filename);
    if(err) {
        LOG("rename failed: %s\n", strerror(errno));
        goto cleanup;
    }
    if(bak) {
        LOG("removing %s\n", bak);
        remove(bak);
    }

    result = 0;
    delete_tmp = 0;

    cleanup:
    if(fp) fclose(fp);
    if(delete_tmp) {
        LOG("removing %s\n", tmpname);
        remove(tmpname);
    }
    free(tmpname);
    free(bak);
    if(result == 0){
        set_status("Wrote to '%s'", filename);
    }
    else {
        set_status("Failed to write to '%s': %s", filename, strerror(errno));
    }
    return result;
}

static
Sheet*
next_sheet(Sheet* sheet, int d){
    int idx;
    for(idx = 0; idx < SHEETS.count; idx++){
        if(SHEETS.data[idx] == sheet)
            break;
    }
    assert(idx < SHEETS.count);
    idx += d;
    if(idx < 0)
        idx = SHEETS.count-1;
    if(idx >= SHEETS.count-1)
        idx = 0;
    return SHEETS.data[idx];
}

static
void
apply_undo(Sheet* sheet, Undoable* u){
    switch(u->kind){
        case UNDO_INVALID:
            abort();
            break;
        case UNDO_NESTED:
            // apply undo in reverse order they happened
            for(size_t i = u->nested.count; i--;){
                Undoable* nu = &u->nested.data[i];
                apply_undo(sheet, nu);
            }
            break;
        case UNDO_CHANGE_CELL:
            update_cell_no_eval_a(sheet, u->change_cell.y, u->change_cell.x, u->change_cell.before);
            break;
        case UNDO_DELETE_ROWS:
            insert_row(sheet, u->delete_rows.y, u->delete_rows.n);
            break;
        case UNDO_INSERT_ROWS:
            delete_row(sheet, u->insert_rows.y, u->insert_rows.n);
            break;
    }

}
static
void
apply_redo(Sheet* sheet, Undoable* u){
    switch(u->kind){
        case UNDO_INVALID:
            abort();
            break;
        case UNDO_NESTED:
            // apply undo in reverse order they happened
            for(size_t i = 0 ; i < u->nested.count; i++){
                Undoable* nu = &u->nested.data[i];
                apply_redo(sheet, nu);
            }
            break;
        case UNDO_CHANGE_CELL:
            update_cell_no_eval_a(sheet, u->change_cell.y, u->change_cell.x, u->change_cell.after);
            break;
        case UNDO_DELETE_ROWS:
            delete_row(sheet, u->delete_rows.y, u->delete_rows.n);
            break;
        case UNDO_INSERT_ROWS:
            insert_row(sheet, u->insert_rows.y, u->insert_rows.n);
            break;
    }
}

static
void
undo(Sheet* sheet){
    UndoStack* ud = &sheet->undo_stack;
    LOG("undo cursor: %zu\n", ud->cursor);
    if(!ud->cursor) return;
    Undoable* u = &ud->data[--ud->cursor];
    apply_undo(sheet, u);
}

static
void
redo(Sheet* sheet){
    UndoStack* ud = &sheet->undo_stack;
    LOG("redo cursor: %zu\n", ud->cursor);
    if(ud->cursor == ud->count) return;
    Undoable* u = &ud->data[ud->cursor++];
    apply_redo(sheet, u);
}


static
void
update_display(SheetView* view){
    if(needs_rescale){
        TermSize sz = get_terminal_size();
        drt_update_terminal_size(drt, sz.columns, sz.rows);
        drt_update_drawable_area(drt, 0, 0, sz.columns, sz.rows);
        drt_invalidate(drt);
        drt_clear_screen(drt);
        if(view->rows != sz.rows || view->cols != sz.columns){
            view->rows = sz.rows;
            view->cols = sz.columns;
        }
        needs_rescale = 0;
        view->needs_redisplay = 1;
    }
    if(view->needs_redisplay){
        view->needs_redisplay = 0;
        int cell_rows = view->rows - 2 - 3;
        if(view->cell_y < view->base_y)
            view->base_y = view->cell_y;
        if(view->cell_y - view->base_y >= cell_rows)
            view->base_y = view->cell_y - cell_rows + 1;
        if(view->base_y < 0) view->base_y = 0;
        if(view->cell_x < view->base_x)
            view->base_x = view->cell_x;
        int screen_x = 1 + 4;
        for(int ix = view->base_x; ix < view->cell_x; ix++){
            int advance = DEFAULT_WIDTH+1;
            if(ix < view->sheet->columns.count){
                Column* col = &view->sheet->columns.data[ix];
                if(col->hidden)
                    advance = 0;
                else
                    advance = 1 + col->width;
            }
            screen_x += advance;
        }
        while(screen_x+1 >= view->cols){
            int advance = DEFAULT_WIDTH+1;
            if(view->base_x < view->sheet->columns.count){
                Column* col = &view->sheet->columns.data[view->base_x];
                if(col->hidden)
                    advance = 0;
                else
                    advance = 1 + col->width;
            }
            screen_x -= advance;
            view->base_x++;
        }
        draw_grid(view);
    }
    drt_move(drt, 0, view->rows-1-1-1);
    drt_clear_to_end_of_row(drt);
    drt_printf(drt, "%-8s -- ", mode_name(MODE));
    for(size_t i = 0; i < SHEETS.count && i < 9; i++){
        Sheet* sh = SHEETS.data[i];
        if(sh == view->sheet){
            drt_push_state(drt);
            drt_set_8bit_color(drt, 8);
            drt_set_style(drt, DRT_STYLE_BOLD);
            drt_printf(drt, "[%zu] %s ", i+1, sh->name);
            drt_pop_state(drt);
        }
        else
            drt_printf(drt, "[%zu] %s ", i+1, sh->name);
    }
    drt_move(drt, 0, view->rows-1-1);
    drt_clear_to_end_of_row(drt);
    drt_printf(drt, "%s", status);
    drt_move(drt, 0, view->rows-1);
    drt_set_cursor_visible(drt, 1);
    if(MODE == INSERT_MODE || MODE == CHANGE_MODE || MODE == CHANGE_SINGLE_MODE){
        drt_clear_to_end_of_row(drt);
        drt_printf(drt, "%s", EDIT.buff);
        drt_move_cursor(drt, EDIT.buff_cursor, view->rows-1);
    }
    else if(MODE == MOVE_MODE){
        drt_set_cursor_visible(drt, 0);
        DrspAtom a = get_rc_a(&view->sheet->data, view->cell_y, view->cell_x);
        size_t len; const char* txt = drsp_atom_get_str(CTX, a, &len);
        drt_clear_to_end_of_row(drt);
        drt_printf(drt, "%.*s", (int)len, txt);
    }
    else if(MODE == COMMAND_MODE || MODE == SELECT_COMMAND_MODE){
        drt_clear_to_end_of_row(drt);
        drt_printf(drt, ":%s", EDIT.buff);
        drt_move_cursor(drt, EDIT.buff_cursor+1, view->rows-1);
    }
    else if(MODE == QUERY_MODE){
        drt_clear_to_end_of_row(drt);
        drt_printf(drt, "> %s", EDIT.buff);
        drt_move_cursor(drt, EDIT.buff_cursor+1+1, view->rows-1);
    }
    else if(MODE == SEARCH_MODE){
        drt_clear_to_end_of_row(drt);
        drt_printf(drt, "/%s", EDIT.buff);
        drt_move_cursor(drt, EDIT.buff_cursor+1, view->rows-1);
    }
    else
        drt_set_cursor_visible(drt, 0);
    drt_paint(drt);
}

static
int
get_input(int* pc, int* pcx, int* pcy, int* pmagnitude){
    char _c;
    char sequence[32] = {0};
    ssize_t nread = read_one_b(&_c);
    int c = (int)(unsigned char)_c;
    if(nread < 0)
        return -1;
    if(!nread) return 0;
    if(c > 127){ // utf-8 sequence
        sequence[0] = (char)c;
        int length;
        if((c & 0xe0) == 0xc0)
            length = 2;
        else if((c & 0xf0) == 0xe0)
            length = 3;
        else if((c & 0xf8) == 0xf0)
            length = 4;
        else {
            if(0)LOG("invalid utf-8 starter? %#x\n", c);
            // invalid sequence
             return 0;
        }
        ssize_t e = 0;
        for(int i = 1; i < length; i++){
            e = read_one_nb(sequence+i);
            if(e == -1) return -1;
            int val = (int)(unsigned char)sequence[i];
            if(val <= 127){
                if(0)LOG("val: %d\n", val);
                if(0)LOG("invalid utf-8? '%.*s'\n", i+1, sequence);
                break;
            }
        }
        for(int i = 0; i < length; i++){
            if(0)LOG("seq[%d] = %x\n", i, (int)(unsigned char)sequence[i]);
        }
        if(0)LOG("utf-8: %.*s\n", length, sequence);
        return 0;
    }
    int cx = 0, cy = 0;
    int magnitude = 1;
    if(c == ESC){
        if(read_one_nb(sequence) == -1) return -1;
        if(read_one_nb(sequence+1) == -1) return -1;
        if(0)LOG("ESC %d %d\n", sequence[0], sequence[1]);
        if(sequence[0] == '['){
            if(sequence[1] == '<'){
                // See https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-Extended-coordinates
                int i;
                int mb = 0;
                for(i = 2; i < sizeof sequence; i++){
                    if(read_one_nb(sequence+i) == -1) return -1;
                    if(sequence[i] == 0) return 0; // unexpected end of escape sequence
                    if(sequence[i] == ';') break;
                    if(sequence[i] < '0' || sequence[i] > '9') return 0; // out of range, should be decimal
                    mb *= 10;
                    mb += sequence[i] - '0';
                }
                int x = 0;
                for(; i < sizeof sequence; i++){
                    if(read_one_nb(sequence+i) == -1) return -1;
                    if(sequence[i] == 0) return 0; // unexpected end of escape sequence
                    if(sequence[i] == ';') break;
                    if(sequence[i] < '0' || sequence[i] > '9') return 0; // out of range, should be decimal
                    x *= 10;
                    x += sequence[i] - '0';
                }
                int y = 0;
                for(; i < sizeof sequence; i++){
                    if(read_one_nb(sequence+i) == -1) return -1;
                    if(sequence[i] == 0) return 0; // unexpected end of escape sequence
                    if(sequence[i] == 'm' || sequence[i] == 'M') break;
                    if(sequence[i] < '0' || sequence[i] > '9') return 0; // out of range, should be decimal
                    y *= 10;
                    y += sequence[i] - '0';
                }
                _Bool up = sequence[i] == 'm';
                if(0){LOG("ESC [ <");
                for(int s = 2; s <= i; s++)
                    LOG(" %d", sequence[s]);
                LOG("\n");
                LOG("mb: %d\n", mb);
                LOG("x: %d\n", x);
                LOG("y: %d\n", y);
                LOG("up: %s\n", up?"true":"false");
                }
                cx = x -1;
                cy = y -1;
                switch(mb){
                    case 0: c = up?LCLICK_UP:LCLICK_DOWN; break;
                    case 32: c = LDRAG; break;
                    case 64: c = UP; magnitude = 3; break;
                    case 65: c = DOWN; magnitude = 3; break;
                    default: LOG("Unknown mb: %d\n", mb);
                }
            }
            else if(sequence[1] == 'M'){
                if(read_one_nb(sequence+2) == -1) return -1;
                if(read_one_nb(sequence+3) == -1) return -1;
                // LOG("click?\n");
                if(0)LOG("ESC [ M %d %d %d\n", sequence[2], sequence[3], sequence[4]);
                cx = ((int)(unsigned char)sequence[3]) - 32-1;
                cy = ((int)(unsigned char)sequence[4]) - 32-1;
                // LOG("x, y: %d,%d\n", cx, cy);
                switch(sequence[2]){
                    case 32:
                        c = LCLICK_DOWN;
                        break;
                    case 35:
                        c = LCLICK_UP;
                        break;
                    case 96:
                        c = UP;
                        magnitude = 3;
                        break;
                    case 97:
                        c = DOWN;
                        magnitude = 3;
                        break;
                }
            }
            else if (sequence[1] >= '0' && sequence[1] <= '9'){
                // Extended escape, read additional byte.
                if (read_one_nb(sequence+2) == -1) return -1;
                if (sequence[2] == '~') {
                    switch(sequence[1]) {
                    case '1':
                        c = HOME;
                        break;
                    // 2 is INSERT, which idk what that would do.
                    case '3': /* Delete key. */
                        c = DELETE;
                        break;
                    case '4':
                        c = END;
                        break;
                    case '5':
                        c = PAGE_UP;
                        break;
                    case '6':
                        c = PAGE_DOWN;
                        break;
                    case '7':
                        c = HOME;
                        break;
                    case '8':
                        c = END;
                        break;
                    // Maybe we want to use f-keys later?
                    // 10 F0
                    // 11 F1
                    // 12 F2
                    // 13 F3
                    // 14 F4
                    // 15 F5
                    // 17 F6
                    // 18 F7
                    // 19 F8
                    // 20 F9
                    // 21 F10
                    // 23 F11
                    // 24 F12

                    }
                }
            }
            else {
                switch(sequence[1]) {
                case 'A': // Up
                    c = UP;
                    break;
                case 'B': // Down
                    c = DOWN;
                    break;
                case 'C': // Right
                    c = RIGHT;
                    break;
                case 'D': // Left
                    c = LEFT;
                    break;
                case 'H': // Home
                    c = HOME;
                    break;
                case 'F': // End
                    c = END;
                    break;
                case 'Z': // Shift-tab
                    c = SHIFT_TAB;
                    break;
                }
            }
        }
        else if(sequence[0] == 'O'){
            switch(sequence[1]){
                case 'H': // Home
                    c = HOME;
                    break;
                case 'F': // End
                    c = END;
                    break;
            }
        }
    }
    else
        if(0)LOG("c: %d\n", c);
    if(0)
    LOG("%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            (int)(unsigned char)_c,
            (int)(unsigned char)sequence[0],
            (int)(unsigned char)sequence[1],
            (int)(unsigned char)sequence[2],
            (int)(unsigned char)sequence[3],
            (int)(unsigned char)sequence[4],
            (int)(unsigned char)sequence[5],
            (int)(unsigned char)sequence[6],
            (int)(unsigned char)sequence[7]);
    *pc = c;
    *pcx = cx;
    *pcy = cy;
    *pmagnitude = magnitude;
    return 1;
}

static
void
drsp_parse_args(int argc, char** argv, char* (*files)[64], _Bool* first_row_is_not_names){
    ArgToParse pos_args[] = {
        {
            .name = SV("files"),
            .dest = ARGDEST(*files),
            .min_num = 0,
            .max_num = arrlen(*files)-1,
            .help = "drsp or tsvs",
        },
    };
    ArgToParse kw_args[] = {
        {
            .name = SV("--bordered"),
            .altname1 = SV("-b"),
            .dest = ARGDEST(&DRAW_BORDERS),
            .help = "Draw separators between cells",
        },
        {
            .name = SV("--logfile"),
            .altname1 = SV("-l"),
            .dest = ARGDEST(&LOGFILE),
            .help = "File to log to.",
        },
        {
            .name = SV("--not-first"),
            .altname1 = SV("--first-row-is-not-names"),
            .dest = ARGDEST(first_row_is_not_names),
            .help = "Don't use the first row of the file as column names",
        },
    };
    enum {HELP=0, FISH=1};
    ArgToParse early_args[] = {
        [HELP] = {
            .name = SV("-h"),
            .altname1 = SV("--help"),
            .help = "Print this help and exit.",
        },
        [FISH] = {
            .name = SV("-f"),
            .altname1 = SV("--fish"),
            .help = "List the names of the test functions and exit.",
            .hidden = 1,
        },
    };
    ArgParser argparser = {
        .name = argc?argv[0]:"drsp",
        .description = "TUI spreadsheet editor/evaluator",
        .positional={
            .args = pos_args,
            .count = arrlen(pos_args),
        },
        .keyword={
            .args = kw_args,
            .count = arrlen(kw_args),
        },
        .early_out={
            .args = early_args,
            .count = arrlen(early_args),
        },
        .styling={.plain = !isatty(fileno(stdout)),},
    };
    Args args = argc?(Args){argc-1, (const char*const*)argv+1}: (Args){0, 0};
    switch(check_for_early_out_args(&argparser, &args)){
        case HELP:{
            int columns = get_terminal_size().columns;
            if(columns > 80)
                columns = 80;
            print_argparse_help(&argparser, columns);
            exit(0);
        }break;
        case FISH:
            print_argparse_fish_completions(&argparser);
            exit(0);
            break;
        default:
            break;
    }
    enum ArgParseError e = parse_args(&argparser, &args, ARGPARSE_FLAGS_NONE);
    if(e){
        print_argparse_error(&argparser, e);
        fprintf(stderr, "Use --help to see usage.\n");
        exit((int)e);
    }
}

int
main(int argc, char** argv){
    #ifdef _WIN32
    STDIN = GetStdHandle(STD_INPUT_HANDLE);
    STDOUT = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    char* files[64] = {0};
    _Bool first_row_is_not_names = 0;
    drsp_parse_args(argc, argv, &files, &first_row_is_not_names);
    set_status("");
    #ifdef _WIN32
    #else
    int pid = getpid();
    LOG("pid: %d\n", pid);
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sighandler;
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);
    // signal(SIGWINCH, sighandler);
    #endif

    SheetOps ops = make_ops();
    CTX = drsp_create_ctx(&ops);
    NIL_ATOM = xatomize("", 0);
    Sheet* SHEET = NULL;
    SheetView* active_view = NULL;

    if(!files[0]){
        SHEET = new_sheet("main", NULL);
        int e = drsp_set_sheet_name(CTX, SHEET, SHEET->name, strlen(SHEET->name));
        if(e) goto finally;
    }
    else {
        int e;
        for(char** f = files; *f; f++){
            char* filename = *f;
            char* name;
            _Bool is_drsp = 0;
            {
                char* slash = strrchr(filename, '/');
                #ifdef _WIN32
                {
                    char* backslash = strrchr(slash?slash:filename, '\\');
                    if(backslash) slash = backslash;
                }
                #endif
                name = slash?slash+1:filename;
                char* dot = strrchr(name, '.');
                if(dot)
                    name = xstrdup2(name, dot-name);
                else
                    name = xstrdup(name);
                if(dot && streq(dot, ".drsp"))
                    is_drsp = 1;
            }
            if(find_sheet(name)){
                free(name);
                continue;
            }
            SHEET = new_sheet(name, filename);
            e = drsp_set_sheet_name(CTX, SHEET, SHEET->name, strlen(SHEET->name));
            if(e) goto finally;
            char* txt = read_file(filename);
            if(!txt) txt = xstrdup("");
            char* line;
            _Bool first_row_as_names = !first_row_is_not_names;
            for(int y =0;(line=strsep(&txt, "\n"));y++){
                char* token;
                int x = 0;
                if(is_drsp){
                    y = -1;
                    if(streq(line, "---") || !strlen(line)){
                        is_drsp = 0;
                        continue;
                    }
                    token = strsep(&line, "\t|");
                    if(!token) continue;
                    if(streq(token, "names")){
                        for(;(token = strsep(&line, "\t|"));x++){
                            if(*token) rename_column(SHEET, x, token);
                        }
                        continue;
                    }
                    y = 0;
                    is_drsp = 0;
                    goto not_drsp;
                }
                else if(y == 0 && first_row_as_names){
                    y = -1;
                    first_row_as_names = 0;
                    for(;(token = strsep(&line, "\t|"));x++){
                        if(*token) rename_column(SHEET, x, token);
                    }
                    continue;
                }
                for(;(token = strsep(&line, "\t|"));x++){
                    not_drsp:;
                    update_cell_no_eval_a(SHEET, y, x, xatomize(token, strlen(token)));
                }
            }
            free(txt);
        }
    }
    assert(SHEET);
    if(!SHEET) {
        return 1;
    }
    active_view = new_view(SHEET);
#if 0 || defined(DRSP_TUI_BENCH)
    int err = drsp_evaluate_formulas(CTX);
    return err;
#endif

    begin_tui();
    atexit(end_tui);
    Undoable editing_ud = {0};

    int prev_c = 0;
    int prev_search = 0;
    int search_backward = 0;
    for(;;){
        if(needs_recalc){
            uint64_t t0 = get_t();
            int err = drsp_evaluate_formulas(CTX);
            uint64_t t1 = get_t();
            (void)err;
            needs_recalc = 0;
            LOG("%d drsp_evaluate_formulas: %lluµs\n", __LINE__, (unsigned long long)t1-t0);
            LOG("%d drsp_evaluate_formulas: %.3fs\n", __LINE__, (t1-t0)/1e6);
        }
        update_display(active_view);
        int c;
        int cx, cy;
        int magnitude;
        int r = get_input(&c, &cx, &cy, &magnitude);
        if(r == -1) goto finally;
        if(!r) continue;

        if(c == CTRL_Z){
            #ifdef _WIN32
            #else
            end_tui();
            raise(SIGTSTP);
            begin_tui();
            change_mode(MODE);
            redisplay(active_view);
            #endif
            continue;
        }
        if(MODE == MOVE_MODE || MODE == SELECT_MODE || MODE == LINE_SELECT_MODE || MODE==DRAG_SELECT_MODE){
            if(c != 'g' && c != 'd' && c != 'y' && c != 'z'){
                prev_c = 0;
            }
        }
        switch(MODE){
            case MOVE_MODE:
                switch(c){
                    case 'u':{
                        undo(SHEET);
                        recalc();
                    }break;
                    case CTRL_R:{
                        redo(SHEET);
                        recalc();
                    }break;
                    case 'o':{
                        insert_row_with_undo(SHEET, active_view->cell_y+1, 1);
                        recalc();
                    }break;
                    case 'O':{
                        insert_row_with_undo(SHEET, active_view->cell_y, 1);
                        recalc();
                    }break;
                    case 'd':
                        if(prev_c != 'd'){
                            prev_c = c;
                        }
                        else {
                            prev_c = 0;
                            copy_rows(SHEET, active_view->cell_y, 1);
                            delete_row_with_undo(SHEET, active_view->cell_y, 1);
                            recalc();
                        }
                        break;
                    case 'v':
                        change_mode(SELECT_MODE);
                        redisplay(active_view);
                        active_view->sel_x = active_view->cell_x;
                        active_view->sel_y = active_view->cell_y;
                        break;
                    case 'V':
                        change_mode(LINE_SELECT_MODE);
                        redisplay(active_view);
                        active_view->sel_y = active_view->cell_y;
                        active_view->sel_x = 0;
                        break;
                    case 'y':
                        if(prev_c == 'y'){
                            copy_rows(SHEET, active_view->cell_y, 1);
                            prev_c = 0;
                        }
                        else
                            prev_c = 'y';
                        break;
                    case 'P':
                        paste(active_view, active_view->cell_y, active_view->cell_x, PASTEBOARD.line_paste?PASTE_ABOVE:PASTE_NORMAL);
                        break;
                    case 'p':
                        paste(active_view, active_view->cell_y, active_view->cell_x, PASTEBOARD.line_paste?PASTE_BELOW:PASTE_NORMAL);
                        break;
                    case LCLICK_DOWN: {
                        CellCoord cc = screen_to_cell(active_view, cx, cy);
                        move(active_view, cc.column - active_view->cell_x, cc.row - active_view->cell_y);
                    } break;
                    case LDRAG: {
                        change_mode(DRAG_SELECT_MODE);
                        redisplay(active_view);
                        active_view->sel_x = active_view->cell_x;
                        active_view->sel_y = active_view->cell_y;
                        CellCoord cc = screen_to_cell(active_view, cx, cy);
                        move(active_view, cc.column - active_view->cell_x, cc.row - active_view->cell_y);
                    } break;
                    case 'x':
                    case BACKSPACE:
                    case DELETE:
                        delete_cells(SHEET, active_view->cell_y, active_view->cell_x, 1, 1);
                        break;
                    case PAGE_UP:
                        move(active_view, 0, -active_view->rows);
                        break;
                    case PAGE_DOWN:
                        move(active_view, 0, +active_view->rows);
                        break;
                    case CTRL_U:
                        move(active_view, 0, -active_view->rows/2);
                        break;
                    case '=':
                        change_col_width_fill(SHEET, active_view->cell_x);
                        redisplay(active_view);
                        break;
                    case '-':
                    case '<':
                        change_col_width(SHEET, active_view->cell_x, -1);
                        redisplay(active_view);
                        break;
                    case '+':
                    case '>':
                        change_col_width(SHEET, active_view->cell_x, +1);
                        redisplay(active_view);
                        break;
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':{
                        int idx = c - '1';
                        if(idx < SHEETS.count){
                            Sheet* sheet = SHEETS.data[idx];
                            if(sheet != SHEET){
                                SHEET = sheet;
                                active_view = find_or_new_view(SHEET);
                                active_view->needs_redisplay = 1;
                                needs_rescale = 1;
                                redisplay(active_view);
                            }
                        }
                    }break;
                    case '0':
                        move(active_view, -active_view->cell_x, 0);
                        break;
                    case '$':
                        if(active_view->cell_y < SHEET->data.count){
                            int end = SHEET->data.data[active_view->cell_y].count;
                            int dx = end - active_view->cell_x-1;
                            if(dx) move(active_view, dx, 0);
                        }
                        break;
                    case 'g':
                        if(prev_c != 'g')
                            prev_c = c;
                        else{
                            prev_c = 0;
                            move(active_view, 0, -active_view->cell_y);
                        }
                        break;
                    case 'N':
                        if(prev_search){
                            if(search_backward)
                                goto forward_search;
                            reverse_search:;
                            if(!SHEET->data.count) break;
                            int starty = active_view->cell_y;
                            if(starty >= SHEET->data.count)
                                starty = SHEET->data.count;

                            for(int y = starty; y >= 0; y--){
                                for(int x = y==active_view->cell_y?active_view->cell_x-1:SHEET->data.data[y].count-1;x >= 0 && x < SHEET->data.data[y].count; x--){
                                    if(SHEET->data.data[y].data[x].atom && atom_contains(SHEET->data.data[y].data[x].atom, EDIT.prev_search, strlen(EDIT.prev_search))){
                                        move(active_view, x-active_view->cell_x, y-active_view->cell_y);
                                        goto break_N;
                                    }
                                }
                            }
                        }
                        break_N:;
                        break;
                    case 'n':
                        if(prev_search){
                            if(search_backward)
                                goto reverse_search;
                            forward_search:;
                            for(int y = active_view->cell_y; y < SHEET->data.count; y++){
                                for(int x = y==active_view->cell_y?active_view->cell_x+1:0;x < SHEET->data.data[y].count; x++){
                                    if(SHEET->data.data[y].data[x].atom && atom_contains(SHEET->data.data[y].data[x].atom, EDIT.prev_search, strlen(EDIT.prev_search))){
                                        move(active_view, x-active_view->cell_x, y-active_view->cell_y);
                                        goto break_n;
                                    }
                                }
                            }
                        }
                        break_n:;
                        break;
                    case 'B':
                    case 'b':
                        for(int y = active_view->cell_y; y >= 0 && y < SHEET->data.count; y--){
                            for(int x = y==active_view->cell_y?active_view->cell_x-1:SHEET->data.data[y].count-1;x >= 0 && x < SHEET->data.data[y].count; x--){
                                if(SHEET->data.data[y].data[x].atom && SHEET->data.data[y].data[x].atom != NIL_ATOM){
                                    move(active_view, x-active_view->cell_x, y-active_view->cell_y);
                                    goto break_b;
                                }
                            }
                        }
                        break_b:;
                        break;
                    case 'W':
                    case 'w':
                        for(int y = active_view->cell_y; y < SHEET->data.count; y++){
                            for(int x = y==active_view->cell_y?active_view->cell_x+1:0;x < SHEET->data.data[y].count; x++){
                                if(SHEET->data.data[y].data[x].atom && SHEET->data.data[y].data[x].atom != NIL_ATOM){
                                    move(active_view, x-active_view->cell_x, y-active_view->cell_y);
                                    goto break_w;
                                }
                            }
                        }
                        break_w:;
                        break;
                    case 'G':
                        move(active_view, 0, SHEET->data.count-active_view->cell_y);
                        break;
                    case 'k':
                    case UP:
                    case CTRL_P:
                        if(magnitude != 1) scroll(active_view, 0, -magnitude);
                        else move(active_view, 0, -magnitude);
                        break;
                    case CTRL_D:
                        move(active_view, 0, +active_view->rows/2);
                        break;
                    case 'j':
                    case DOWN:
                    case CTRL_N:
                        if(magnitude != 1) scroll(active_view, 0, magnitude);
                        else move(active_view, 0, +magnitude);
                        break;
                    case 'H':
                        hide_columns(active_view->sheet, active_view->cell_x, 1);
                        move(active_view, 1, 0);
                        break;
                    case 's':
                        sort_sheet(active_view->sheet, active_view->cell_x, SORT_ASCENDING);
                        break;
                    case 'S':
                        sort_sheet(active_view->sheet, active_view->cell_x, SORT_DESCENDING);
                        break;
                    case 'h':
                    case LEFT:
                    case CTRL_B:
                    case SHIFT_TAB:
                        move(active_view, -1, 0);
                        break;
                    case 'l':
                    case RIGHT:
                    case TAB:
                    case CTRL_F:
                        move(active_view, +1, 0);
                        break;
                    // case ENTER:
                    case CTRL_J:{
                        DrspAtom a = get_rc_a(&SHEET->data, active_view->cell_y, active_view->cell_x);
                        move(active_view, 0, +1);
                        DrspAtom before = get_rc_a(&SHEET->data, active_view->cell_y, active_view->cell_x);
                        if(before != a){
                            update_cell_a(SHEET, active_view->cell_y, active_view->cell_x, a);
                            note_cell_change(&SHEET->undo_stack, active_view->cell_y, active_view->cell_x, before, a);
                        }
                    } break;
                    case CTRL_C:
                        redisplay(active_view);
                        break;
                    case 'c':
                        change_mode(CHANGE_MODE);
                        redisplay(active_view);
                        begin_line_edit(active_view);
                        break;
                    case 'r':
                        change_mode(CHANGE_SINGLE_MODE);
                        redisplay(active_view);
                        begin_line_edit(active_view);
                        break;
                    case 'e':
                    case 'i':
                        change_mode(INSERT_MODE);
                        redisplay(active_view);
                        begin_line_edit(active_view);
                        break;
                    case ESC:
                        set_status("");
                        change_mode(MOVE_MODE);
                        redisplay(active_view);
                        break;
                    case ';':
                    case ':':
                        change_mode(COMMAND_MODE);
                        redisplay(active_view);
                        begin_line_edit_buff("", 0);
                        break;
                    case 'q':
                    case 'Q':
                        change_mode(QUERY_MODE);
                        redisplay(active_view);
                        begin_line_edit_buff("", 0);
                        break;
                    case '/':
                        search_backward = 0;
                        change_mode(SEARCH_MODE);
                        redisplay(active_view);
                        begin_line_edit_buff("", 0);
                        break;
                    case '?':
                        search_backward = 1;
                        change_mode(SEARCH_MODE);
                        redisplay(active_view);
                        begin_line_edit_buff("", 0);
                        break;
                    case 'z':
                        if(prev_c != 'z')
                            prev_c = 'z';
                        else {
                            active_view->base_y = active_view->cell_y - active_view->cols/2;
                            if(active_view->base_y < 0)
                                active_view->base_y = 0;
                            redisplay(active_view);
                        }
                        break;
                    default:
                        break;
                }
                break;
            case DRAG_SELECT_MODE:
            case LINE_SELECT_MODE:
            case SELECT_MODE:
                switch(c){
                    case 'V':
                        if(MODE != LINE_SELECT_MODE){
                            change_mode(LINE_SELECT_MODE);
                            redisplay(active_view);
                        }
                        break;
                    case 'd':{
                        if(MODE == SELECT_MODE || MODE == DRAG_SELECT_MODE)
                            goto sel_DELETE;
                        prev_c = 0;
                        int y = imin(active_view->cell_y, active_view->sel_y);
                        int y1 = imax(active_view->cell_y, active_view->sel_y);
                        int h = y1-y+1;
                        copy_rows(SHEET, y, h);
                        delete_row_with_undo(SHEET, y, h);
                        move(active_view, 0, 1-h);
                        recalc();
                        change_mode(MOVE_MODE);
                        redisplay(active_view);
                    }break;
                    case 'y':{
                        prev_c = 0;
                        int y = imin(active_view->cell_y, active_view->sel_y);
                        int y1 = imax(active_view->cell_y, active_view->sel_y);
                        int h = y1-y+1;
                        if(MODE == LINE_SELECT_MODE){
                            copy_rows(SHEET, y, h);
                        }
                        else {
                            int x = imin(active_view->cell_x, active_view->sel_x);
                            int x1 = imax(active_view->cell_x, active_view->sel_x);
                            int w = x1-x+1;
                            copy_cells(SHEET, y, x, h, w);
                        }
                        change_mode(MOVE_MODE);
                        redisplay(active_view);
                    }break;
                    case 'o':{
                        int tmp;
                        tmp = active_view->cell_x;
                        active_view->cell_x = active_view->sel_x;
                        active_view->sel_x = tmp;

                        tmp = active_view->cell_y;
                        active_view->cell_y = active_view->sel_y;
                        active_view->sel_y = tmp;
                        redisplay(active_view);
                    }break;
                    case 'p':
                        paste(active_view, imin(active_view->cell_y, active_view->sel_y), imin(active_view->cell_x, active_view->sel_x), PASTEBOARD.line_paste?PASTE_REPLACE:PASTE_NORMAL);
                        recalc();
                        change_mode(MOVE_MODE);
                        redisplay(active_view);
                        break;
                    case LDRAG:
                    case LCLICK_DOWN:
                        {
                        // XXX: this needs to also needs to change the active view
                        CellCoord cc = screen_to_cell(active_view, cx, cy);
                        move(active_view, cc.column - active_view->cell_x, cc.row - active_view->cell_y);
                        if(MODE == DRAG_SELECT_MODE && c == LCLICK_DOWN)
                            change_mode(MOVE_MODE);
                    } break;
                    case 'x':
                    case BACKSPACE:
                    case DELETE:{
                        sel_DELETE:;
                        int x =  imin(active_view->cell_x, active_view->sel_x);
                        int y =  imin(active_view->cell_y, active_view->sel_y);
                        int x1 = imax(active_view->cell_x, active_view->sel_x);
                        int y1 = imax(active_view->cell_y, active_view->sel_y);
                        if(MODE == LINE_SELECT_MODE) line_select_bounds(SHEET, &x, &x1);
                        delete_cells(SHEET, y, x, y1-y+1, x1-x+1);
                        change_mode(MOVE_MODE);
                        redisplay(active_view);
                    }break;
                    case PAGE_UP:
                        move(active_view, 0, -active_view->rows);
                        break;
                    case PAGE_DOWN:
                        move(active_view, 0, +active_view->rows);
                        break;
                    case CTRL_U:
                        move(active_view, 0, -active_view->rows/2);
                        break;
                    case '0':
                        move(active_view, -active_view->cell_x, 0);
                        break;
                    case '$':
                        if(SHEET->data.count)
                            move(active_view, SHEET->data.data[0].count-active_view->cell_x, 0);
                        break;
                    case 'g':
                        if(prev_c != 'g')
                            prev_c = c;
                        else{
                            prev_c = 0;
                            move(active_view, 0, -active_view->cell_y);
                        }
                        break;
                    case 'G':
                        move(active_view, 0, SHEET->data.count-active_view->cell_y);
                        break;
                    case 'k':
                    case UP:
                    case CTRL_P:
                        if(magnitude != 1) scroll(active_view, 0, -magnitude);
                        else move(active_view, 0, -magnitude);
                        break;
                    case CTRL_D:
                        move(active_view, 0, +active_view->rows/2);
                        break;
                    case 'j':
                    case DOWN:
                    case CTRL_N:
                        if(magnitude != 1) scroll(active_view, 0, +magnitude);
                        else move(active_view, 0, +magnitude);
                        break;
                    case 'h':
                    case LEFT:
                    case CTRL_B:
                    case SHIFT_TAB:
                        move(active_view, -1, 0);
                        break;
                    case 'l':
                    case RIGHT:
                    case TAB:
                    case CTRL_F:
                        move(active_view, +1, 0);
                        break;
                    case CTRL_C:
                        change_mode(MOVE_MODE);
                        redisplay(active_view);
                        break;
                    case ESC:
                        change_mode(MOVE_MODE);
                        redisplay(active_view);
                        break;
                    case ';':
                    case ':':
                        change_mode(SELECT_COMMAND_MODE);
                        begin_line_edit_buff("", 0);
                        redisplay(active_view);
                        break;
                    default:
                        break;
                }
                break;
            case CHANGE_SINGLE_MODE:
            case CHANGE_MODE:
            case INSERT_MODE:
                switch(c){
                    case '`':
                    case ESC:
                    case CTRL_C:
                        if(editing_ud.kind != UNDO_INVALID){
                            push_undo_event(&SHEET->undo_stack, editing_ud);
                            editing_ud = (Undoable){
                                .kind = UNDO_INVALID,
                            };
                        }
                        change_mode(MOVE_MODE);
                        redisplay(active_view);
                        break;
                    case CTRL_J:
                    case ENTER:{
                        DrspAtom before = get_rc_a(&SHEET->data, active_view->cell_y, active_view->cell_x);
                        update_cell_a(SHEET, active_view->cell_y, active_view->cell_x, xatomize(EDIT.buff, EDIT.buff_len));
                        DrspAtom after = get_rc_a(&SHEET->data, active_view->cell_y, active_view->cell_x);
                        if(before != after){
                            if(editing_ud.kind != UNDO_NESTED){
                                assert(editing_ud.kind == UNDO_INVALID);
                                editing_ud = (Undoable){
                                    .kind = UNDO_NESTED,
                                };
                            }
                            *append(&editing_ud.nested) = (Undoable){
                                .kind = UNDO_CHANGE_CELL,
                                .change_cell = {.y=active_view->cell_y, .x=active_view->cell_x, .before=before, .after=after},
                            };
                        }
                        if(MODE == CHANGE_SINGLE_MODE){
                            change_mode(MOVE_MODE);
                            break;
                        }
                        move(active_view, 0, +1);
                        begin_line_edit(active_view);
                    }break;
                    case CTRL_N:
                    case DOWN:
                        move(active_view, 0, +1);
                        begin_line_edit(active_view);
                        break;
                    case SHIFT_TAB:
                    case LEFT:
                        move(active_view, -1, 0);
                        begin_line_edit(active_view);
                        break;
                    case TAB:
                    case RIGHT:
                        move(active_view, +1, 0);
                        begin_line_edit(active_view);
                        break;
                    case CTRL_P:
                    case UP:
                        move(active_view, 0, -1);
                        begin_line_edit(active_view);
                        break;
                    case LCLICK_DOWN:{
                        CellCoord cc = screen_to_cell(active_view, cx, cy);
                        char buff[24];
                        int n = int_to_colname(buff, sizeof buff, cc.column);
                        n--;
                        if(cc.row == active_view->cell_y){
                            buff[n++] = '$';
                        }
                        else{
                            int n2 = snprintf(buff+n, (sizeof buff)-n, "%d", cc.row+1);
                            if(n2 > 0)
                                n += n2;
                            else
                                n = -1;
                        }
                        if(n > 0)
                            insert_text(buff, n);
                    }break;
                    case LCLICK_UP:
                        break;
                    default:
                        handle_edit_key(c);
                        break;
                }
                break;
            case SEARCH_MODE:
            case QUERY_MODE:
            case COMMAND_MODE:
            case SELECT_COMMAND_MODE:
                switch(c){
                    case CTRL_J:
                    case ENTER:
                        set_status("");
                        if(MODE == QUERY_MODE){
                            DrSpreadResult outval = {0};
                            int e = drsp_evaluate_string(CTX, SHEET, EDIT.buff, EDIT.buff_len, &outval, -1, -1);
                            if(e){
                                outval.kind = DRSP_RESULT_STRING;
                                outval.s.text = "error";
                                outval.s.length = 5;
                            }
                            if(outval.kind == DRSP_RESULT_NULL){
                                outval.kind = DRSP_RESULT_STRING;
                                outval.s.text = "''";
                                outval.s.length = 2;
                            }
                            switch(outval.kind){
                                case DRSP_RESULT_STRING:
                                    set_status("%s -> %.*s", EDIT.buff, (int)outval.s.length, outval.s.text);
                                    begin_line_edit_buff("", 0);
                                    break;
                                case DRSP_RESULT_NUMBER:
                                    set_status("%s -> %.2f", EDIT.buff, outval.d);
                                    begin_line_edit_buff("", 0);
                                    break;
                                case DRSP_RESULT_NULL:
                                    break;
                                default: assert(0);
                            }
                        }
                        else if(MODE == SEARCH_MODE){
                            strcpy(EDIT.prev_search, EDIT.buff);
                            prev_search = 1;
                            change_mode(MOVE_MODE);
                            redisplay(active_view);
                            for(int y = active_view->cell_y; y < SHEET->data.count; y++){
                                for(int x = y==active_view->cell_y?active_view->cell_x+1:0;x < SHEET->data.data[y].count; x++){
                                    if(SHEET->data.data[y].data[x].atom && atom_contains(SHEET->data.data[y].data[x].atom, EDIT.buff, EDIT.buff_len)){
                                        move(active_view, x-active_view->cell_x, y-active_view->cell_y);
                                        goto break_search;
                                    }
                                }
                            }
                            break_search:;
                        }
                        else if(MODE == SELECT_COMMAND_MODE){
                            // TODO: use selection here
                            if(streq(EDIT.buff, "sort")){
                                change_mode(MOVE_MODE);
                                sort_sheet(active_view->sheet, active_view->cell_x, SORT_ASCENDING);
                                redisplay(active_view);
                                continue;
                            }
                            if(memeq(EDIT.buff, "sort ", 5)){
                                // TODO
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            if(streq(EDIT.buff, "h") || streq(EDIT.buff, "hide") || streq(EDIT.buff, "hi")){
                                int x0 = imin(active_view->cell_x, active_view->sel_x);
                                int x1 = imax(active_view->cell_x, active_view->sel_x);
                                hide_columns(active_view->sheet, x0, x1-x0+1);
                                move(active_view, 1, 0);
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            set_status("Not a recognized command: '%s'", EDIT.buff);
                        }
                        else {
                            // TODO
                            if(streq(EDIT.buff, "q") || streq(EDIT.buff, "quit") || streq(EDIT.buff, "exit"))
                                goto finally;
                            if(streq(EDIT.buff, "clear")){
                                clear(SHEET);
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            if(streq(EDIT.buff, "bg")){
                                #ifdef _WIN32
                                #else
                                end_tui();
                                raise(SIGTSTP);
                                begin_tui();
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                #endif
                                continue;
                            }
                            if(streq(EDIT.buff, "wq") || streq(EDIT.buff, "x")){
                                int err = atomically_write_sheet(SHEET, SHEET->filename);
                                // TODO: report errors
                                if(!err) goto finally;
                                continue;
                            }
                            if(streq(EDIT.buff, "w") || streq(EDIT.buff, "write")){
                                int err = atomically_write_sheet(SHEET, SHEET->filename);
                                (void)err; // TODO: report errors
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            if(memeq(EDIT.buff, "w ", 2)){
                                int err = atomically_write_sheet(SHEET, EDIT.buff+2);
                                (void)err; // TODO: report errors
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            if(memeq(EDIT.buff, "write ", 6)){
                                int err = atomically_write_sheet(SHEET, EDIT.buff+6);
                                (void)err; // TODO: report errors
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            if(memeq(EDIT.buff, "rc ", 3)){
                                rename_column(SHEET, active_view->cell_x, EDIT.buff+3);
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            if(streq(EDIT.buff, "ne")
                            || streq(EDIT.buff, "nex")
                            || streq(EDIT.buff, "next")){
                                SHEET = next_sheet(SHEET, +1);
                                active_view = find_view(SHEET);
                                redisplay(active_view);
                                change_mode(MOVE_MODE);
                                continue;
                            }
                            if(streq(EDIT.buff, "pr")
                            || streq(EDIT.buff, "prev")
                            || streq(EDIT.buff, "previous")){
                                SHEET = next_sheet(SHEET, -1);
                                active_view = find_view(SHEET);
                                redisplay(active_view);
                                change_mode(MOVE_MODE);
                                continue;
                            }
                            if(streq(EDIT.buff, "border")
                            || streq(EDIT.buff, "bo")
                            || streq(EDIT.buff, "borderless")
                            ){
                                DRAW_BORDERS = !DRAW_BORDERS;
                                redisplay(active_view);
                                change_mode(MOVE_MODE);
                                continue;
                            }
                            if(streq(EDIT.buff, "redraw")){
                                needs_rescale = 1;
                                redisplay(active_view);
                                change_mode(MOVE_MODE);
                                continue;
                            }
                            if(streq(EDIT.buff, "sort")){
                                change_mode(MOVE_MODE);
                                sort_sheet(active_view->sheet, active_view->cell_x, SORT_ASCENDING);
                                redisplay(active_view);
                                continue;
                            }
                            if(memeq(EDIT.buff, "sort ", 5)){
                                // TODO
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            if(streq(EDIT.buff, "h") || streq(EDIT.buff, "hide") || streq(EDIT.buff, "hi")){
                                hide_columns(active_view->sheet, active_view->cell_x, 1);
                                move(active_view, 1, 0);
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            if(streq(EDIT.buff, "unhide") || streq(EDIT.buff, "unhi")){
                                unhide_columns(active_view->sheet);
                                change_mode(MOVE_MODE);
                                redisplay(active_view);
                                continue;
                            }
                            set_status("Not a recognized command: '%s'", EDIT.buff);
                        }
                        break;
                    case CTRL_C:
                    case ESC:
                        change_mode(MODE==SELECT_COMMAND_MODE?SELECT_MODE:MOVE_MODE);
                        redisplay(active_view);
                        break;
                    default:
                        handle_edit_key(c);
                        break;
                }
                break;
        }
    }
    finally:
    return 0;
}


#define DRSP_INTRINS 1
#include "drspread.c"
#include "drt.c"
static DrtLL _drt;
static DrtLL* drt = &_drt;
