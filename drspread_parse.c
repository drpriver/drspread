//
// Copyright © 2023-2024, David Priver <david@davidpriver.com>
//
#ifndef DRSPREAD_PARSE_C
#define DRSPREAD_PARSE_C
#include "drspread_parse.h"
#include "drspread_formula_funcs.h"
#include "parse_numbers.h"
#include "stringview.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef __clang__
#pragma clang assume_nonnull begin
#else
#ifndef _Nullable
#define _Nullable
#endif
#endif

// The gnu case range extension is much nicer than this, but is non-standard
// and is not supported on non-gnu-style compilers.
// To paper over that you need a macro anyway, in which case you might as well
// just do this and write them all out in the macro.
//
// Note: the first case is missing "case" and the last case is missing ":" so that
// indenters/formatters work correctly:
//
// switch(foo){
//     case CASE_0_9:
//         // whatever
//     default:
//         // whatever
// }
//
//
#ifndef CASE_0_9
#define CASE_0_9 '0': case '1': case '2': case '3': case '4': case '5': \
    case '6': case '7': case '8': case '9'
#endif

#ifndef CASE_a_z
#define CASE_a_z 'a': case 'b': case 'c': case 'd': case 'e': case 'f': \
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': \
    case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': \
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z'
#endif

#ifndef CASE_A_Z
#define CASE_A_Z 'A': case 'B': case 'C': case 'D': case 'E': case 'F': \
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': \
    case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': \
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z'
#endif

#define PARSEFUNC(x) Expression*_Nullable x(DrSpreadCtx* ctx, StringView* sv)
DRSP_INTERNAL PARSEFUNC(parse_comparison);
DRSP_INTERNAL PARSEFUNC(parse_addplus);
DRSP_INTERNAL PARSEFUNC(parse_divmul);
DRSP_INTERNAL PARSEFUNC(parse_unary);
DRSP_INTERNAL PARSEFUNC(parse_terminal);
DRSP_INTERNAL PARSEFUNC(parse_number);
DRSP_INTERNAL PARSEFUNC(parse_group);
DRSP_INTERNAL PARSEFUNC(parse_range);
DRSP_INTERNAL PARSEFUNC(parse_string);
DRSP_INTERNAL PARSEFUNC(parse_func_call);

static inline
Expression*_Nullable
parse_other_range_syntax(DrSpreadCtx* ctx, StringView* sv, const char* cn, size_t cn_len);

DRSP_INTERNAL
Expression*_Nullable
parse(DrSpreadCtx* ctx, DrspAtom a){
    {
        Expression* cached = has_cached_parse(ctx, a);
        if(cached) return expr_clone(ctx, cached);
    }
    StringView sv = {a->length, a->data};
    lstrip(&sv);
    while(sv.length && sv.text[0] == '=')
        sv.text++, sv.length--;
    lstrip(&sv);
    // printf("'%s'\n", sv.text);
    Expression* root = parse_comparison(ctx, &sv);
    if(!root || root->kind == EXPR_ERROR) {
        // XXX: free what we can
        if(root && root->kind == EXPR_ERROR)
            cache_parse(ctx, a, root);
        return root;
    }
    lstrip(&sv);
    if(sv.length != 0) {
        // XXX: free what we can
        Expression* e =  Error(ctx, "parsing expression did not consume all input");
        cache_parse(ctx, a, e);
        return e;
    }
    cache_parse(ctx, a, root);
    return root;
}

DRSP_INTERNAL
PARSEFUNC(parse_comparison){
    Expression* lhs = parse_addplus(ctx, sv);
    if(!lhs || lhs->kind == EXPR_ERROR) return lhs;
    lstrip(sv);
    while(sv->length && (sv->text[0] == '<' || sv->text[0] == '>' || sv->text[0] == '=' || sv->text[0] == '!')){
        char c = sv->text[0];
        sv->length--, sv->text++;
        char c2 = 0;
        if(sv->length && sv->text[0] == '='){
            sv->length--, sv->text++;
            c2 = '=';
        }
        BinaryKind op;
        switch(c){
            case '<': op = c2? BIN_LE : BIN_LT; break;
            case '>': op = c2? BIN_GE : BIN_GT; break;
            case '!':
                if(!c2) return Error(ctx, "expected '=' after '!'");
                op = BIN_NE;
                break;
            // Allow '=' and '=='.
            case '=': op = BIN_EQ; break;
            default: return Error(ctx, "invalid comparison operator");
        }
        lstrip(sv);
        Expression* rhs = parse_addplus(ctx, sv);
        if(!rhs || rhs->kind == EXPR_ERROR) return rhs;
        Binary* b = parser_expr_alloc(ctx, EXPR_BINARY);
        if(!b) return NULL;
        b->lhs = lhs;
        b->rhs = rhs;
        b->op = op;
        lhs = &b->e;
    }
    return lhs;
}

DRSP_INTERNAL
PARSEFUNC(parse_addplus){
    Expression* lhs = parse_divmul(ctx, sv);
    if(!lhs || lhs->kind == EXPR_ERROR) return lhs;
    lstrip(sv);
    while(sv->length && (sv->text[0] == '+' || sv->text[0] == '-')){
        char c = sv->text[0];
        sv->length--, sv->text++;
        lstrip(sv);
        Expression* rhs = parse_divmul(ctx, sv);
        if(!rhs || rhs->kind == EXPR_ERROR) return rhs;
        BinaryKind op = c == '+'?BIN_ADD : BIN_SUB;
        Binary* b = parser_expr_alloc(ctx, EXPR_BINARY);
        if(!b) return NULL;
        b->lhs = lhs;
        b->rhs = rhs;
        b->op = op;
        lhs = &b->e;
    }
    return lhs;
}

DRSP_INTERNAL
PARSEFUNC(parse_divmul){
    Expression* lhs = parse_unary(ctx, sv);
    if(!lhs || lhs->kind == EXPR_ERROR) return lhs;
    lstrip(sv);
    while(sv->length && (sv->text[0] == '*' || sv->text[0] == '/')){
        char c = sv->text[0];
        sv->length--, sv->text++;
        lstrip(sv);
        Expression* rhs = parse_unary(ctx, sv);
        if(!rhs || rhs->kind == EXPR_ERROR) return rhs;
        BinaryKind op = c == '*'?BIN_MUL : BIN_DIV;
        Binary* b = parser_expr_alloc(ctx, EXPR_BINARY);
        if(!b) return NULL;
        b->lhs = lhs;
        b->rhs = rhs;
        b->op = op;
        lhs = &b->e;
    }
    return lhs;
}

DRSP_INTERNAL
PARSEFUNC(parse_unary){
    while(sv->length && sv->text[0] == '+'){
        sv->text++, sv->length--;
        lstrip(sv);
    }
    while(sv->length >= 2 && sv->text[0] == '-' && sv->text[1] == '-'){
        sv->text+=2;
        sv->length-=2;
        lstrip(sv);
    }
    if(!sv->length) return Error(ctx, "end of input before valid expression");
    char c = sv->text[0];
    UnaryKind op = (UnaryKind)-1;
    switch(c){
        case '!': op = UN_NOT; break;
        case '-': op = UN_NEG; break;
        default: return parse_terminal(ctx, sv);
    }
    if(op != (UnaryKind)-1){
        sv->text++, sv->length--;
        lstrip(sv);
    }
    while(sv->length && sv->text[0] == '+'){
        sv->text++, sv->length--;
        lstrip(sv);
    }
    while(sv->length >= 2 && sv->text[0] == '-' && sv->text[1] == '-'){
        sv->text+=2;
        sv->length-=2;
        lstrip(sv);
    }
    Expression* e = parse_unary(ctx, sv);
    if(!e || e->kind == EXPR_ERROR) return e;
    if(op != (UnaryKind)-1){
        _Bool handled = 0;
        if(e->kind == EXPR_NUMBER){
            Number* n = (Number*)e;
            if(op == UN_NOT){
                n->value = !n->value;
                handled = 1;
            }
            if(op == UN_NEG){
                n->value = -n->value;
                handled = 1;
            }
        }
        else if(e->kind == EXPR_UNARY){
            Unary* u = (Unary*)e;
            if(op == UN_NEG && u->op == UN_NEG){
                u->op = UN_PLUS;
                handled = 1;
            }
            else if(u->op == UN_PLUS){
                u->op = op;
                handled = 1;
            }
        }
        if(!handled){
            Unary* u = parser_expr_alloc(ctx, EXPR_UNARY);
            if(!u) return NULL;
            u->op = op;
            u->expr = e;
            e = &u->e;
        }
    }
    return e;
}

DRSP_INTERNAL
PARSEFUNC(parse_terminal){
    if(!sv->length) return Error(ctx, "end of input when parsing terminal");
    switch(sv->text[0]){
        case '[':
            return parse_range(ctx, sv);
        case CASE_a_z:
        case CASE_A_Z:
        case '_':
            return parse_func_call(ctx, sv);
        case CASE_0_9:
        case '.':
            return parse_number(ctx, sv);
        case '(':
            return parse_group(ctx, sv);
        case '\'':
        case '"':
            return parse_string(ctx, sv);
        case ':':
            return parse_other_range_syntax(ctx, sv, "", 0);
        default:
            return Error(ctx, "invalid start of terminal expression");
    }
}

DRSP_INTERNAL
PARSEFUNC(parse_string){
    assert(sv->length && (sv->text[0] == '"' || sv->text[0] == '\''));
    char terminator = sv->text[0];
    sv->length--, sv->text++;
    const char* begin = sv->text;
    while(sv->length && sv->text[0] != terminator){
        sv->length--, sv->text++;
    }
    const char* end = sv->text;
    if(!sv->length) return Error(ctx, "Unterminated string");
    Expression* result;
    if(end != begin){
        String* s = parser_expr_alloc(ctx, EXPR_STRING);
        if(!s) return NULL;
        DrspAtom str = drsp_intern_str(ctx, begin, end-begin);
        if(!str) return NULL;
        s->str = str;
        result = &s->e;
    }
    else {
        result = parser_expr_alloc(ctx, EXPR_BLANK);
    }
    sv->length--, sv->text++;
    return result;
}

static inline
StringView
parse_possibly_bare_string(StringView* sv){
    if(!sv->length)
        return (StringView){0};
    char terminator = sv->text[0];
    if(terminator == '\'' || terminator == '"'){
        // quoted string
        sv->text++; sv->length--;
        const char* begin = sv->text;
        while(sv->length && sv->text[0] != terminator){
            sv->length--, sv->text++;
        }
        if(!sv->length) return (StringView){0};
        const char* end = sv->text;
        sv->length--, sv->text++;
        return (StringView){end-begin, begin};
    }
    switch(sv->text[0]){
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case ':': case '$':
            return (StringView){0};
        default: break;
    }
    // bare string
    const char* begin = sv->text;
    while(sv->length && sv->text[0] != ' ' && sv->text[0] != ',' && sv->text[0] != ']'){
        sv->length--, sv->text++;
    }
    if(!sv->length) return (StringView){0};
    const char* end = sv->text;
    return (StringView){end-begin, begin};
}

// Returns how many numbers were parsed, or -1 on error.
// Blank numbers are parsed as IDX_BLANK, which technically collides with a
// valid number, but no one will ever notice.
static inline
int
maybe_parse_number_pair(StringView* sv, intptr_t* a, intptr_t* b){
    if(!sv->length) return -1;
    switch(sv->text[0]){
        case ':':
            *a = IDX_BLANK;
            // secondnumber will advance sv
            goto secondnumber;
        case '$':
            *a = IDX_DOLLAR;
            sv->length--, sv->text++;
            break;
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':{
            const char* begin = sv->text;
            const char* p = begin+1;
            for(const char* end = sv->text+sv->length; p != end; p++){
                switch(*p){
                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        continue;
                    default: break;
                }
                break;
            }
            Int32Result ir = parse_int32(begin, p-begin);
            if(ir.errored) return -1;
            if(ir.result == INT32_MIN) return -1;
            *a = ir.result - 1;
            sv->text += p-begin, sv->length -= p-begin;
            break;
        }
        case ']':
            return 0;
        default:
              return -1;
    }
    lstrip(sv);
    if(!sv->length) return -1;
    if(sv->text[0] != ':')
        return 1;

    secondnumber:;
    sv->text++, sv->length--;
    lstrip(sv);
    if(!sv->length) return -1;
    switch(sv->text[0]){
        case ']':
            *b = IDX_BLANK;
            // Don't advance sv
            return 2;
        case '$':
            *b = IDX_DOLLAR;
            sv->length--, sv->text++;
            return 2;
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':{
            const char* begin = sv->text;
            const char* p = begin+1;
            for(const char* end = sv->text+sv->length; p != end; p++){
                switch(*p){
                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        continue;
                    default: break;
                }
                break;
            }
            Int32Result ir = parse_int32(begin, p-begin);
            if(ir.errored) return -1;
            *b = ir.result - 1;
            sv->text += p-begin, sv->length -= p-begin;
            return 2;
        }
        default:
              return -1;
    }
}

// -1 for error
// 0 for didn't parse
// 1 for parsed 1
// Precondition: already lstripped
static inline
int
maybe_parse_bare_string(StringView* sv, StringView* out){
    if(!sv->length)
        return -1;
    char terminator = sv->text[0];
    if(terminator == '\'' || terminator == '"'){
        // quoted string
        sv->text++; sv->length--;
        const char* begin = sv->text;
        while(sv->length && sv->text[0] != terminator){
            sv->length--, sv->text++;
        }
        if(!sv->length) return -1;
        const char* end = sv->text;
        sv->length--, sv->text++;
        *out = (StringView){end-begin, begin};
        return 1;
    }
    switch(sv->text[0]){
        // This is a number
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case ':': case '$':
            return 0;
        // possibly other characters shouldn't be allowed
        case '[':
            return -1;
        case ',': case ']':
            return 0;
        default: break;
    }
    // bare string
    const char* begin = sv->text;
    // possibly other characters shouldn't be allowed
    while(sv->length && sv->text[0] != ' ' && sv->text[0] != ',' && sv->text[0] != ']' && sv->text[0] != ':'){
        sv->length--, sv->text++;
    }
    if(!sv->length) return -1; // might be unreachable?
    const char* end = sv->text;
    *out = (StringView){end-begin, begin};
    return 1;
}

static inline
int
maybe_parse_string_pair(StringView* sv, StringView* a, StringView* b){
    StringView s = *sv; // in case we need to backtrack.
    if(!sv->length) return -1;
    {
        int n = maybe_parse_bare_string(sv, a);
        if(n < 0) return -1;
        lstrip(sv);
        if(!sv->length) return -1;
        if(sv->text[0] != ':'){
            return n;
        }
        if(n == 0) {
            *a = (StringView){0};
        }
    }
    sv->length--, sv->text++;
    lstrip(sv);
    if(!sv->length) return -1;
    if(!a->length){
        // check if it's actually a number range.
        switch(sv->text[0]){
            case '-': case '$':
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                *sv = s;
                return 0;
            default:
                break;
        }
    }
    int n2 = maybe_parse_bare_string(sv, b);
    if(n2 < 0) return -1;
    if(!n2) *b = (StringView){0};
    return 2;
}

static inline
intptr_t
maybe_parse_number(StringView* sv){
    if(!sv->length) return IDX_UNSET;
    switch(sv->text[0]){
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '$':
            break;
        default:
            return IDX_UNSET;
    }
    const char* begin = sv->text;
    while(sv->length && sv->text[0] != ' ' && sv->text[0] != ',' && sv->text[0] != ']' && sv->text[0] != ':'){
        sv->length--, sv->text++;
    }
    if(!sv->length)
        return IDX_UNSET;
    const char* end = sv->text;
    if(end - begin == 1 && *begin == '$')
        return IDX_DOLLAR;
    Int32Result ir = parse_int32(begin, end-begin);
    if(ir.errored) return IDX_UNSET;
    return ir.result;
}


DRSP_INTERNAL
PARSEFUNC(parse_range){
    //
    // [col, 1]       -> 0d
    // [col]          -> 1d column
    //
    // [col, 1:3]     -> 1d column
    // [col, :3]      -> 1d column
    // [col, :]       -> 1d column
    // [col, 1:]      -> 1d column

    // [col:col, 1]   -> 1d row
    // [col:, 1]      -> 1d row
    // [:col, 1]      -> 1d row
    // [:, 1]         -> 1d row
    //
    // [col:col, 1:3] -> 2d range, but we don't support this
    //
    // [col:col] -> error
    //
    // Having a sheetname makes it foreign.
    //
    assert(sv->length && sv->text[0] == '[');
    sv->length--, sv->text++;
    StringView strings_0_[2];
    StringView strings_1_[2];
    intptr_t numbers[2] = {0};
    int nstrings[2] = {0};
    int nnumbers = 0;
    lstrip(sv);
    // These error messages suck but I want to delete this syntax anyway
    if(!sv->length) return Error(ctx, "bad range literal");
    {
        int n = maybe_parse_string_pair(sv, strings_0_, strings_0_+1);
        if(n < 0) return Error(ctx, "bad range literal");
        nstrings[0] = n;
        if(!n) return Error(ctx, "bad range literal");
        lstrip(sv);
    }
    if(!sv->length) return Error(ctx, "bad range literal");
    if(sv->text[0] == ','){
        if(nstrings[0] == 0) return Error(ctx, "bad range literal");
        sv->text++, sv->length--;
        lstrip(sv);
        if(!sv->length) return Error(ctx, "bad range literal");
    }
    {
        int n = maybe_parse_string_pair(sv, strings_1_, strings_1_+1);
        if(n < 0) return Error(ctx, "bad range literal");
        lstrip(sv);
        if(n == 2 && sv->length && sv->text[0] == ']' && !strings_1_[0].length && !strings_1_[1].length){
            // disambiguate as a number range instead.
            nnumbers = 2;
            numbers[0] = 0;
            numbers[1] = -1;
            goto after_numbers;
        }
        nstrings[1] = n;
    }
    if(!sv->length) return Error(ctx, "bad range literal");
    if(nstrings[1] && sv->text[0] == ','){
        if(nstrings[0] == 0) return Error(ctx, "bad range literal"); // XXX unnecessary check?
        sv->text++, sv->length--;
        lstrip(sv);
        if(!sv->length) return Error(ctx, "bad range literal");
    }
    {
        int n = maybe_parse_number_pair(sv, numbers, numbers+1);
        if(n < 0) return Error(ctx, "bad range literal");
        nnumbers += n;
        lstrip(sv);
        if(n > 0 && numbers[0] == IDX_BLANK)
            numbers[0] = 0;
        if(n > 1 && numbers[1] == IDX_BLANK)
            numbers[1] = -1;
    }
    if(!sv->length) return Error(ctx, "bad range literal");
    after_numbers:;
    if(sv->text[0] != ']') return Error(ctx, "bad range literal");
    sv->text++, sv->length--;
    if(nstrings[1] && nstrings[0] > 1) return Error(ctx, "sheet ranges unsupported");
    DrspAtom sheetname = NULL;
    int ncolnames;
    DrspAtom colnames[2];
    _Bool has_foreign;
    assert(nstrings[0]);
    if(nstrings[1]){
        for(int i = 0; i < nstrings[1]; i++){
            colnames[i] = drsp_intern_sv_lower(ctx, strings_1_[i]);
            if(!colnames[i]) return NULL;
        }
        sheetname = drsp_intern_sv_lower(ctx, strings_0_[0]);
        if(!sheetname) return NULL;
        ncolnames = nstrings[1];
        has_foreign = 1;
    }
    else {
        for(int i = 0; i < nstrings[0]; i++){
            colnames[i] = drsp_intern_sv_lower(ctx, strings_0_[i]);
            if(!colnames[i]) return NULL;
        }
        ncolnames = nstrings[0];
        has_foreign = 0;
    }
    if(!nnumbers){
        assert(ncolnames);
        if(ncolnames > 1)
            return Error(ctx, "[col1:col2] is not supported\n");
        if(has_foreign){
            ForeignRange1DColumn* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_COLUMN_FOREIGN);
            if(!rng) return NULL;
            rng->r.col_name = colnames[0];
            if(!rng->r.col_name) return NULL;
            rng->r.row_start = 0;
            rng->r.row_end = -1;
            rng->sheet_name = sheetname;
            if(!rng->sheet_name) return NULL;
            return &rng->e;
        }
        Range1DColumn* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
        if(!rng) return NULL;
        rng->col_name = colnames[0];
        if(!rng->col_name) return NULL;
        rng->row_start = 0;
        rng->row_end = -1;
        return &rng->e;
    }
    if(nnumbers == 1){
        assert(ncolnames); // XXX might need to be an early return? write tests
        if(ncolnames == 2){
            if(has_foreign){
                ForeignRange1DRow* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_ROW_FOREIGN);
                if(!rng) return NULL;
                rng->r.col_start = colnames[0];
                rng->r.col_end = colnames[1];
                rng->r.row_idx = numbers[0];
                rng->sheet_name = sheetname;
                return &rng->e;
            }
            Range1DRow* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_ROW);
            if(!rng) return NULL;
            rng->col_start = colnames[0];
            rng->col_end = colnames[1];
            rng->row_idx = numbers[0];
            return &rng->e;
        }
        if(has_foreign){
            ForeignRange0D* rng = parser_expr_alloc(ctx, EXPR_RANGE0D_FOREIGN);
            if(!rng) return NULL;
            rng->sheet_name = sheetname;
            rng->r.col_name = colnames[0];
            rng->r.row = numbers[0];
            return &rng->e;
        }
        Range0D* rng = parser_expr_alloc(ctx, EXPR_RANGE0D);
        if(!rng) return NULL;
        rng->col_name = colnames[0];
        rng->row = numbers[0];
        return &rng->e;
    }
    assert(nnumbers == 2);
    assert(ncolnames);
    if(ncolnames == 2) return Error(ctx, "2d ranges unsupported");
    assert(ncolnames == 1);
    if(has_foreign){
        ForeignRange1DColumn* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_COLUMN_FOREIGN);
        if(!rng) return NULL;
        rng->r.col_name = colnames[0];
        rng->r.row_start = numbers[0];
        rng->r.row_end = numbers[1];
        rng->sheet_name = sheetname;
        return &rng->e;
    }
    Range1DColumn* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
    if(!rng) return NULL;
    rng->col_name = colnames[0];
    rng->row_start = numbers[0];
    rng->row_end = numbers[1];
    return &rng->e;
}

DRSP_INTERNAL
PARSEFUNC(parse_number){
    const char* begin = sv->text;
    const char* end = begin;
    while(sv->length){
        char c = sv->text[0];
        switch(c){
            case '.':
            case 'e':
            case 'E':
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                end++, sv->length--, sv->text++;
                continue;
            default:
                break;
        }
        break;
    }
    if(begin == end) return Error(ctx, ""); // unreachable?
    Number* n = parser_expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    DoubleResult dr = parse_double(begin, end-begin);
    if(dr.errored) return Error(ctx, "invalid number");
    n->value = dr.result;
    return &n->e;
}

DRSP_INTERNAL
PARSEFUNC(parse_group){
    (void)sv;
    assert(sv->length && sv->text[0] == '(');
    sv->text++, sv->length--;
    Expression* e = parse_comparison(ctx, sv);
    if(!e || e->kind == EXPR_ERROR) return e;
    lstrip(sv);
    if(!sv->length || sv->text[0] != ')') return Error(ctx, "unterminated paren, no ')'");
    sv->text++, sv->length--;
    return e;
}

static inline
Expression*_Nullable
parse_other_range_syntax(DrSpreadCtx* ctx, StringView* sv, const char* cn, size_t cn_len){
    StringView _colname = {cn_len, cn};
    rstrip(&_colname);
    DrspAtom colname = drsp_intern_str_lower(ctx, _colname.text, _colname.length);
    if(!colname) return NULL;
    intptr_t row_idx = IDX_UNSET;
    {
        const char* begin = sv->text;
        const char* end = begin;
        for(;sv->length; end++, sv->length--, sv->text++){
            switch(sv->text[0]){
                case '$':
                case CASE_0_9:
                    continue;
                default:
                    break;
            }
            break;
        }
        if(begin != end){
            if(end == begin + 1 && *begin == '$'){
                row_idx = IDX_DOLLAR;
            }
            else {
                Int32Result ir = parse_int32(begin, end-begin);
                if(ir.errored) return Error(ctx, ""); // TODO: stringify error
                row_idx = ir.result - 1;
            }
        }
        lstrip(sv);
        if(!sv->length || sv->text[0] != ':'){
            if(row_idx == IDX_UNSET){
                Range1DColumn* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
                if(!rng) return NULL;
                rng->col_name = colname;
                rng->row_start = 0;
                rng->row_end = -1;
                return &rng->e;
            }
            Range0D* rng = parser_expr_alloc(ctx, EXPR_RANGE0D);
            if(!rng) return NULL;
            rng->col_name = colname;
            rng->row = row_idx;
            return &rng->e;
        }
        sv->text++, sv->length--;
        lstrip(sv);
        if(row_idx == IDX_UNSET) row_idx = 0;
    }
    {
        DrspAtom colname2 = drsp_nil_atom();
        {
            const char* begin = sv->text;
            const char* end = begin;
            for(;sv->length; end++, sv->length--, sv->text++){
                switch(sv->text[0]){
                    case CASE_a_z:
                    case CASE_A_Z:
                    case ' ': // allow spaces in identifiers
                        continue;
                    case '$':
                    case CASE_0_9:
                        // if(end == begin) return Error(ctx, "");
                        colname2 = drsp_intern_str_lower(ctx, begin, end-begin);
                        if(!colname2) return NULL;
                        goto parsenum;
                    default:
                        break;
                }
                StringView _colname2 = {end-begin, begin};
                rstrip(&_colname2);
                colname2 = drsp_intern_str_lower(ctx, _colname2.text, _colname2.length);
                if(!colname2) return NULL;
                break;
            }
        }
        parsenum:;
        const char* begin = sv->text;
        const char* end = begin;
        for(;sv->length; end++, sv->length--, sv->text++){
            switch(sv->text[0]){
                case '$':
                case CASE_0_9:
                    continue;
                default:
                    break;
            }
            break;
        }
        lstrip(sv);
        // 2nd number is optional
        intptr_t row_idx2 = -1;
        if(begin != end){
            if(end == begin + 1 && *begin == '$'){
                row_idx2 = IDX_DOLLAR;
            }
            else {
                Int32Result ir = parse_int32(begin, end-begin);
                if(ir.errored) return Error(ctx, ""); // TODO: stringify error
                row_idx2 = ir.result-1;
            }
        }
        if(colname == drsp_nil_atom())
            colname = colname2;
        if(colname2 == drsp_nil_atom() || colname == colname2){
            Range1DColumn* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
            if(!rng) return NULL;
            rng->col_name = colname;
            rng->row_start = row_idx;
            rng->row_end = row_idx2;
            return &rng->e;
        }
        else if(row_idx == row_idx2 || (row_idx != -1 && row_idx2 == -1)){
            Range1DRow* rng = parser_expr_alloc(ctx, EXPR_RANGE1D_ROW);
            if(!rng) return NULL;
            rng->col_start = colname;
            rng->col_end = colname2;
            rng->row_idx = row_idx;
            return &rng->e;
        }
        else {
            return Error(ctx, "2d ranges not supported yet");
        }
    }
}

DRSP_INTERNAL
PARSEFUNC(parse_func_call){
    const char* begin = sv->text;

    const char* end = begin;
    for(;sv->length; end++, sv->length--, sv->text++){
        switch(sv->text[0]){
            case CASE_a_z:
            case CASE_A_Z:
            case ' ': // allow spaces in identifiers
                continue;
            case '$':
            case CASE_0_9:{
                if(begin == end) return Error(ctx, ""); // when does this happen
                return parse_other_range_syntax(ctx, sv, begin, end-begin);
            }
            default:
                break;
        }
        break;
    }
    if(begin == end)
        return Error(ctx, "");
    if(!sv->length || sv->text[0] != '('){
        // This is a range literal actually.
        return parse_other_range_syntax(ctx, sv, begin, end-begin);
    }
    sv->length--, sv->text++;
    StringView name = {end-begin, begin};
    rstrip(&name);
    DrspAtom a = drsp_intern_sv_lower(ctx, name);
    if(!a) return NULL;
    FormulaFunc* func = lookup_func(a);
    // This is pretty sloppy - always allocates space
    // for exactly 4 args - can't do less or more.
    enum {argmax=4};
    Expression** argv = linked_arena_alloc(&ctx->pheap.arena, argmax * sizeof *argv);
    int argc;
    for(argc = 0; argc < argmax; argc++){
        lstripc(sv);
        if(!sv->length) return Error(ctx, "end of input before closing ')'");
        if(sv->text[0] == ')')
            break;
        Expression* e = parse_comparison(ctx, sv);
        if(!e || e->kind == EXPR_ERROR) return e;
        argv[argc] = e;
    }
    if(!sv->length || sv->text[0] != ')')
        return Error(ctx, "end of input before closing ')'");
    sv->length--, sv->text++;
    if(func){
        FunctionCall* fc = parser_expr_alloc(ctx, EXPR_FUNCTION_CALL);
        if(!fc) return NULL;
        fc->func = func;
        fc->argc = argc;
        fc->argv = argv;
        return &fc->e;
    }
    else {
        UserFunctionCall* fc = parser_expr_alloc(ctx, EXPR_USER_DEFINED_FUNC_CALL);
        if(!fc) return NULL;
        fc->name = a;
        fc->argc = argc;
        fc->argv = argv;
        return &fc->e;
    }
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
