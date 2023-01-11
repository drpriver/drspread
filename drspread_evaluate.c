//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_EVALUATE_C
#define DRSPREAD_EVALUATE_C
#include "drspread_evaluate.h"
#include "drspread_parse.h"
#include "parse_numbers.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

static
Expression*_Nullable
evaluate(SpreadContext* ctx, intptr_t row, intptr_t col){
    CacheVal* cached = get_cached_val(&ctx->cache, row, col);
    if(cached){
        if(cached->kind == CACHE_IN_PROGRESS)
            return Error(ctx, "");
        if(cached->kind == EXPR_STRING){
            return &cached->e;
        }
        if(cached->kind == EXPR_NUMBER){
            return &cached->e;
        }
        if(cached->kind == EXPR_ERROR){
            return &cached->e;
        }
        if(cached->kind == EXPR_NULL){
            return &cached->e;
        }
        cached->kind = CACHE_IN_PROGRESS;
    }
    // printf("Evaluate %zd, %zd\n", row, col);
    CellKind kind = CELL_UNKNOWN;
    if(ctx->ops.query_cell_kind)
        kind = ctx->ops.query_cell_kind(ctx->ops.ctx, row, col);
    if(kind == CELL_UNKNOWN){
        size_t len = 0;
        const char* txt = ctx->ops.cell_txt(ctx->ops.ctx, row, col, &len);
        kind = classify_cell(txt, len);
    }
    switch(kind){
        case CELL_EMPTY:
            if(cached){
                cached->kind = EXPR_NULL;
                return &cached->e;
            }
            return expr_alloc(ctx, EXPR_NULL);
        case CELL_OTHER:{
            size_t len = 0;
            const char* txt = ctx->ops.cell_txt(ctx->ops.ctx, row, col, &len);
            if(cached){
                cached->e.kind = EXPR_STRING;
                cached->s.sv = stripped2(txt, len);
                return &cached->e;
            }
            String* s = expr_alloc(ctx, EXPR_STRING);
            if(!s) return NULL;
            s->sv = stripped2(txt, len);
            return &s->e;
        }
        case CELL_NUMBER:{
            double value = ctx->ops.cell_number(ctx->ops.ctx, row, col);
            if(cached){
                cached->n.e.kind = EXPR_NUMBER;
                cached->n.value = value;
                return &cached->e;
            }
            Number* n = expr_alloc(ctx, EXPR_NUMBER);
            if(!n) return NULL;
            n->value = value;
            return &n->e;
        }
        case CELL_FORMULA:{
            size_t len = 0;
            const char* txt = ctx->ops.cell_txt(ctx->ops.ctx, row, col, &len);
            char* chk = ctx->a.cursor;
            Expression *root = parse(ctx, txt, len);
            if(!root || root->kind == EXPR_ERROR)
                ctx->a.cursor = chk;
            if(!root) return NULL;
            if(root->kind == EXPR_ERROR) {
                if(cached){
                    cached->kind = EXPR_ERROR;
                    return &cached->e;
                }
                return root;
            }
            Expression *e = evaluate_expr(ctx, root);
            if(!e) return e;
            if(cached){
                if(e->kind == EXPR_NUMBER){
                    cached->kind = EXPR_NUMBER;
                    cached->n = *(Number*)e;
                    return &cached->e;
                }
                else if(e->kind == EXPR_STRING){
                    cached->kind = EXPR_STRING;
                    cached->s = *(String*)e;
                    return &cached->e;
                }
                else if(e->kind == EXPR_NULL){
                    cached->kind = EXPR_NULL;
                    return &cached->e;
                }
                else{
                    cached->kind = EXPR_ERROR;
                    return &cached->e;
                }
            }
            return e;
        }
        case CELL_UNKNOWN:
            __builtin_trap();
    }
}

// This is for a repl
static
Expression*_Nullable
evaluate_string(SpreadContext* ctx, const char* txt, size_t len){
    Expression *root = parse(ctx, txt, len);
    if(!root || root->kind == EXPR_ERROR) return root;
    Expression *e = evaluate_expr(ctx, root);
    return e;
}

static
Expression*_Nullable
evaluate_expr(SpreadContext* ctx, Expression* expr){
    switch(expr->kind){
        case EXPR_NULL:
        case EXPR_ERROR:
        case EXPR_NUMBER:
        case EXPR_RANGE1D_COLUMN:
        case EXPR_STRING:
            return expr;
        case EXPR_FUNCTION_CALL:{
            FunctionCall* fc = (FunctionCall*)expr;
            return fc->func(ctx, fc->argc, fc->argv);
        }
        case EXPR_RANGE0D:{
            Range0D* rng = (Range0D*)expr;
            return evaluate(ctx, rng->row, rng->col);
        }
        case EXPR_BINARY:{
            char* chk = ctx->a.cursor;
            Binary* b = (Binary*)expr;
            Expression* lhs = evaluate_expr(ctx, b->lhs);
            if(!lhs) return NULL;
            if(lhs->kind != EXPR_NUMBER && lhs->kind != EXPR_STRING) return NULL;
            Expression* rhs = evaluate_expr(ctx, b->rhs);
            if(!rhs) return NULL;
            if(lhs->kind == EXPR_STRING){
                if(rhs->kind != EXPR_STRING)
                    return Error(ctx, "");
                String* l = (String*)lhs;
                String* r = (String*)rhs;
                _Bool cmp;
                switch(b->op){
                    case BIN_EQ:
                        cmp = sv_equals(l->sv, r->sv);
                        break;
                    case BIN_NE:
                        cmp = !sv_equals(l->sv, r->sv);
                        break;
                    default:
                        return Error(ctx, "");
                }
                ctx->a.cursor = chk;
                Number* res = (Number*)expr_alloc(ctx, EXPR_NUMBER);
                res->value = cmp;
                return &res->e;
            }
            if(lhs->kind != EXPR_NUMBER) return Error(ctx, "");
            if(rhs->kind != EXPR_NUMBER) return Error(ctx, "");
            double l = ((Number*)lhs)->value;
            double r = ((Number*)rhs)->value;
            double value;
            switch(b->op){
                case BIN_ADD: value = l +  r; break;
                case BIN_SUB: value = l -  r; break;
                case BIN_MUL: value = l *  r; break;
                case BIN_DIV: value = l /  r; break;
                case BIN_LT:  value = l <  r; break;
                case BIN_LE:  value = l <= r; break;
                case BIN_GT:  value = l >  r; break;
                case BIN_GE:  value = l >= r; break;
                case BIN_EQ:  value = l == r; break;
                case BIN_NE:  value = l != r; break;
                default: __builtin_trap();
            }
            ctx->a.cursor = chk;
            Number* res = (Number*)expr_alloc(ctx, EXPR_NUMBER);
            if(!res) return NULL;
            res->value = value;
            return &res->e;
        }
        case EXPR_UNARY:{
            char* chk = ctx->a.cursor;
            Unary* u = (Unary*)expr;
            Expression* v = evaluate_expr(ctx, u->expr);
            if(!v) return NULL;
            if(v->kind != EXPR_NUMBER) return Error(ctx, "");
            if(u->op == UN_PLUS) return u->expr;
            double d = ((Number*)v)->value;
            double value;
            switch(u->op){
                case UN_NEG: value = -d; break;
                case UN_NOT: value = !d; break;
                case UN_PLUS: __builtin_unreachable();
                default: __builtin_trap();
            }
            ctx->a.cursor = chk;
            Number* r = (Number*)expr_alloc(ctx, EXPR_NUMBER);
            if(!r) return NULL;
            r->value = value;
            return &r->e;
        }
        case EXPR_GROUP:{
            Group* g = (Group*)expr;
            // TODO: check if this tail calls.
            // Otherwise we need a goto to the top.
            return evaluate_expr(ctx, g->expr);
        };
    };
    return NULL;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
