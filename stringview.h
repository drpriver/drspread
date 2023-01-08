#ifndef STRINGVIEW_H
#define STRINGVIEW_H
#include <stddef.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

typedef struct StringView StringView;
struct StringView {
    size_t length;
    const char* text;
};

static inline
void
lstrip(StringView* sv){
    while(sv->length){
        switch(sv->text[0]){
            case ' ':
            case '\n':
            case '\t':
                sv->length--;
                sv->text++;
                continue;
            default:
                return;
        }
    }
}

static inline
void
rstrip(StringView* sv){
    while(sv->length){
        switch(sv->text[sv->length-1]){
            case ' ':
            case '\n':
            case '\t':
                sv->length--;
                continue;
            default:
                return;
        }
    }
}

static inline
void
lstripc(StringView* sv){
    while(sv->length){
        switch(sv->text[0]){
            case ' ':
            case '\n':
            case '\t':
            case ',':
                sv->length--;
                sv->text++;
                continue;
            default:
                return;
        }
    }
}


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
