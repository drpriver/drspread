#ifndef DRSPREAD_PARSE_C
#define DRSPREAD_PARSE_C
#include "drspread_parse.h"
#include "drspread_formula_funcs.h"
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
static PARSEFUNC(parse_comparison);
static PARSEFUNC(parse_addplus);
static PARSEFUNC(parse_divmul);
static PARSEFUNC(parse_unary);
static PARSEFUNC(parse_terminal);
static PARSEFUNC(parse_number);
static PARSEFUNC(parse_group);
static PARSEFUNC(parse_range);
static PARSEFUNC(parse_func_call);

static
Expression*_Nullable
parse(SpreadContext* ctx, const char* txt, size_t length){
    StringView sv = {length, txt};
    lstrip(&sv);
    while(sv.length && sv.text[0] == '=')
        sv.text++, sv.length--;
    lstrip(&sv);
    // printf("'%s'\n", sv.text);
    Expression* root = parse_comparison(ctx, &sv);
    if(!root || root->kind == EXPR_ERROR) return root;
    lstrip(&sv);
    if(sv.length != 0) return Error(ctx, "");
    return root;
}

static
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
            case '<':
                op = c2? BIN_LE : BIN_LT;
                break;
            case '>':
                op = c2? BIN_GE : BIN_GT;
                break;
            case '!':
                if(!c2) return Error(ctx, "");
                op = BIN_NE;
                break;
            case '=':
                op = BIN_EQ;
                break;
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
static
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
static
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
static
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
        default: break;
    }
    if(op != -1){
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
    Expression* e = parse_terminal(ctx, sv);
    if(!e || e->kind == EXPR_ERROR) return e;
    if(op != -1){
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
static
PARSEFUNC(parse_terminal){
    if(!sv->length) return Error(ctx, "");
    switch(sv->text[0]){
        case '[':
            return parse_range(ctx, sv);
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wgnu-case-range"
        case 'a' ... 'z':
        case 'A' ... 'Z':
            return parse_func_call(ctx, sv);
        case '0' ... '9':
        #pragma clang diagnostic pop
        case '.':
            return parse_number(ctx, sv);
        case '(':
            return parse_group(ctx, sv);
        default:
            return Error(ctx, "");
    }
}

static
PARSEFUNC(parse_range){
    assert(sv->length && sv->text[0] == '[');
    intptr_t col, row0, row1;
    sv->length--, sv->text++;
    lstrip(sv);
    if(!sv->length) return Error(ctx, "");
    const char* begin = sv->text;
    const char* end = begin;
    while(sv->length && sv->text[0] != ',' && sv->text[0] != ']'){
        end++, sv->length--, sv->text++;
    }
    if(begin == end) return Error(ctx, "");
    if(!sv->length) return Error(ctx, "");
    col = ctx->ops.name_to_col_idx(ctx->ops.ctx, begin, end-begin);
    if(sv->text[0] == ']'){
        sv->length--, sv->text++;
        Range1DColumn* r = expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
        if(!r) return NULL;
        r->col = col;
        r->row_start = 0;
        r->row_end = -1;
        return &r->e;
    }
    // TODO: real integer parsing
    sv->length--, sv->text++;
    lstrip(sv);
    begin = sv->text;
    end = begin;
    while(sv->length && sv->text[0] != ']' && sv->text[0] != ':'){
        end++, sv->length--, sv->text++;
    }
    if(!sv->length) return Error(ctx, "");
    if(begin == end)
        row0 = 0;
    else{
        row0 = atoi(begin);
        if(row0) row0--;
    }
    char c = sv->text[0];
    sv->text++, sv->length--;
    if(c == ']'){
        Range0D* r = expr_alloc(ctx, EXPR_RANGE0D);
        if(!r) return NULL;
        r->col = col;
        r->row = row0;
        return &r->e;
    }
    lstrip(sv);
    begin = sv->text;
    end = begin;
    while(sv->length && sv->text[0] != ']'){
        end++, sv->length--, sv->text++;
    }
    if(!sv->length) return Error(ctx, "");
    sv->text++, sv->length--;
    if(begin == end)
        row1 = -1;
    else{
        row1 = atoi(begin);
        if(row1) row1--;
    }
    Range1DColumn* r = expr_alloc(ctx, EXPR_RANGE1D_COLUMN);
    if(!r) return NULL;
    r->col = col;
    r->row_start = row0;
    r->row_end = row1;
    return &r->e;
}
static
PARSEFUNC(parse_number){
    const char* begin = sv->text;
    const char* end = begin;
    while(sv->length){
        char c = sv->text[0];
        switch(c){
            case '.':
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wgnu-case-range"
            case '0' ... '9':
            #pragma clang diagnostic pop
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
    n->value = strtod(begin, NULL);
    return &n->e;
}

static
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
    FormulaFunc* func = NULL;

    for(size_t i = 0; i < FUNCTABLE_LENGTH; i++){
        if(FUNCTABLE[i].name.length == name.length){
            if(memcmp(FUNCTABLE[i].name.text, name.text, name.length)==0){
                func = FUNCTABLE[i].func;
                break;
            }
        }
    }
    if(!func) return Error(ctx, "");
    enum {argmax=2};
    Expression** argv = buff_alloc(&ctx->a, argmax * sizeof *argv);
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
