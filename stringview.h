//
// Copyright © 2023, David Priver
//
#ifndef STRINGVIEW_H
#define STRINGVIEW_H
#include <stddef.h>
#include <string.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

typedef struct StringView StringView;
struct StringView {
    size_t length;
    const char* text;
};

#define SV(x) ((StringView){sizeof(x)-1, x})

static inline
_Bool
sv_equals(StringView a, StringView b){
    if(a.length != b.length) return 0;
    return memcmp(a.text, b.text, a.length) == 0;
}

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

static inline
StringView
stripped(StringView sv){
    lstrip(&sv);
    rstrip(&sv);
    return sv;
}

static inline
StringView
stripped2(const char* txt, size_t len){
    StringView sv = {len, txt};
    lstrip(&sv);
    rstrip(&sv);
    return sv;
}


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
