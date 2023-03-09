//
// Copyright © 2023, David Priver
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

#define PARSEFUNC(x) Expression*_Nullable x(SpreadContext* ctx, StringView* sv)
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

DRSP_INTERNAL
Expression*_Nullable
parse(SpreadContext* ctx, const char* txt, size_t length){
    StringView sv = {length, txt};
    lstrip(&sv);
    while(sv.length && sv.text[0] == '=')
        sv.text++, sv.length--;
    lstrip(&sv);
    // printf("'%s'\n", sv.text);
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* root = parse_comparison(ctx, &sv);
    if(!root || root->kind == EXPR_ERROR) {
        buff_set(ctx->a, bc);
        return root;
    }
    lstrip(&sv);
    if(sv.length != 0) {
        buff_set(ctx->a, bc);
        return Error(ctx, "");
    }
    // fprintf(stderr, "%s:%d %zd bytes used\n", __func__, __LINE__, ctx->a.cursor - ctx->a.data);
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
                if(!c2) return Error(ctx, "");
                op = BIN_NE;
                break;
            // Allow '=' and '=='.
            case '=': op = BIN_EQ; break;
            default: return Error(ctx, "");
        }
        lstrip(sv);
        Expression* rhs = parse_addplus(ctx, sv);
        if(!rhs || rhs->kind == EXPR_ERROR) return rhs;
        Binary* b = expr_alloc(ctx, EXPR_BINARY);
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
        Binary* b = expr_alloc(ctx, EXPR_BINARY);
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
        Binary* b = expr_alloc(ctx, EXPR_BINARY);
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
    if(!sv->length) return Error(ctx, "");
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
            Unary* u = expr_alloc(ctx, EXPR_UNARY);
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
    if(!sv->length) return Error(ctx, "");
    switch(sv->text[0]){
        case '[':
            return parse_range(ctx, sv);
        case 'a': case 'b': case 'c': case 'd':
        case 'e': case 'f': case 'g': case 'h':
        case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p':
        case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D':
        case 'E': case 'F': case 'G': case 'H':
        case 'I': case 'J': case 'K': case 'L':
        case 'M': case 'N': case 'O': case 'P':
        case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X':
        case 'Y': case 'Z':
        case '_':
            return parse_func_call(ctx, sv);
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '.':
            return parse_number(ctx, sv);
        case '(':
            return parse_group(ctx, sv);
        case '\'':
        case '"':
            return parse_string(ctx, sv);
        default:
            return Error(ctx, "");
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
    if(!sv->length) return Error(ctx, "");
    Expression* result;
    if(end != begin){
        String* s = expr_alloc(ctx, EXPR_STRING);
        if(!s) return NULL;
        s->sv.text = begin;
        s->sv.length = end - begin;
        result = &s->e;
    }
    else {
        result = expr_alloc(ctx, EXPR_NULL);
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
    assert(sv->length && sv->text[0] == '[');
    sv->length--, sv->text++;
    StringView sheetname = {0};
    StringView colname = {0};
    intptr_t row0 = IDX_UNSET, row1 = IDX_UNSET;
    lstrip(sv);
    if(!sv->length) return Error(ctx, "");
    sheetname = parse_possibly_bare_string(sv);
    lstripc(sv);
    colname = parse_possibly_bare_string(sv);
    if(!colname.length){
        colname = sheetname;
        sheetname = (StringView){0};
    }
    if(!colname.length) return Error(ctx, "");

    lstripc(sv);
    if(sv->length && sv->text[0] == ':'){
        row0 = 1;
    }
    else {
        row0 = maybe_parse_number(sv);
        lstrip(sv);
    }
    if(sv->length && sv->text[0] == ':'){
        sv->length--, sv->text++;
        lstrip(sv);
        row1 = maybe_parse_number(sv);
        lstrip(sv);
        if(row1 == IDX_UNSET)
            row1 = -1;
    }
    if(!sv->length || sv->text[0] != ']')
        return Error(ctx, "");
    assert(sv->length && sv->text[0] == ']');
    sv->length--, sv->text++;
    if(row0 == IDX_UNSET && row1 == IDX_UNSET){
        row0 = 1;
        row1 = -1;
    }
    if(row0 > 0)
        row0--;
    if(row1 > 0)
        row1--;
    if(row1 == IDX_UNSET){
        // 0D
        if(sheetname.length){
            // Foreign
            ForeignRange0D* r = expr_alloc(ctx, EXPR_RANGE0D_FOREIGN);
            if(!r) return NULL;
            r->r.col_name = colname;
            r->sheet_name = sheetname;
            r->r.row = row0;
            return &r->e;
        }
        else {
            Range0D* r = expr_alloc(ctx, EXPR_RANGE0D);
            if(!r) return NULL;
            r->col_name = colname;
            r->row = row0;
            return &r->e;
        }
    }
    else {
        // 1D
        if(sheetname.length){
            // Foreign
            ForeignRange1DColumn* r = expr_alloc(ctx, EXPR_RANGE1D_COLUMN_FOREIGN);
            if(!r) return NULL;
            r->sheet_name = sheetname;
            r->r.col_name = colname;
            r->r.row_start = row0;
            r->r.row_end = row1;
            return &r->e;
        }
        else {
            Range1DColumn* r = expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
            if(!r) return NULL;
            r->col_name = colname;
            r->row_start = row0;
            r->row_end = row1;
            return &r->e;
        }
    }
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
    if(begin == end) return Error(ctx, "");
    Number* n = expr_alloc(ctx, EXPR_NUMBER);
    if(!n) return NULL;
    DoubleResult dr = parse_double(begin, end-begin);
    if(dr.errored) return Error(ctx, "");
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
    if(!sv->length || sv->text[0] != ')') return Error(ctx, "");
    sv->text++, sv->length--;
    return e;
}

DRSP_INTERNAL
PARSEFUNC(parse_func_call){
    const char* begin = sv->text;
    const char* end = begin;
    while(sv->length && sv->text[0] != '('){
        end++, sv->length--, sv->text++;
    }
    if(begin == end || !sv->length || sv->text[0] != '(')
        return Error(ctx, "");
    sv->length--, sv->text++;
    StringView name = {end-begin, begin};
    rstrip(&name);
    FormulaFunc* func = lookup_func(name);
    if(!func) return Error(ctx, "");
    // This is pretty sloppy - always allocates space
    // for exactly 4 args - can't do less or more.
    enum {argmax=4};
    Expression** argv = buff_alloc(ctx->a, argmax * sizeof *argv);
    int argc;
    for(argc = 0; argc < argmax; argc++){
        lstripc(sv);
        if(!sv->length) return Error(ctx, "");
        if(sv->text[0] == ')')
            break;
        Expression* e = parse_comparison(ctx, sv);
        if(!e || e->kind == EXPR_ERROR) return e;
        argv[argc] = e;
    }
    if(!sv->length || sv->text[0] != ')')
        return Error(ctx, "");
    sv->length--, sv->text++;
    FunctionCall* fc = expr_alloc(ctx, EXPR_FUNCTION_CALL);
    if(!fc) return NULL;
    fc->func = func;
    fc->argc = argc;
    fc->argv = argv;
    return &fc->e;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
