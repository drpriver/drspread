#ifndef DRSPREAD_EVALUATE_C
#define DRSPREAD_EVALUATE_C
#include "drspread_evaluate.h"
#include "drspread_parse.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

// This implementation seems wonky. It doesn't seem
// right to parse as part of eval.
static
int 
evaluate_s(SpreadContext*, const char* txt, size_t length, double* outval);

static
int
evaluate(SpreadContext* ctx, intptr_t row, intptr_t col, double* outval){
    size_t len = 0;
    const char* txt = ctx->ops.cell_txt(ctx->ops.ctx, row, col, &len);
    int err = evaluate_s(ctx, txt, len, outval);
    return err;
}

static
int 
evaluate_s(SpreadContext* ctx, const char* txt, size_t length, double* outval){
    Expression *root = parse(ctx, txt, length);
    if(!root) return 1;
    Expression *e = evaluate_expr(ctx, root);
    if(!e) return 1;
    switch(e->kind){
        case EXPR_NUMBER:
            *outval = ((Number*)e)->value;
            return 0;
        default:
            return 1;
    }
}

static
Expression*_Nullable
evaluate_expr(SpreadContext* ctx, Expression* expr){
    switch(expr->kind){
        case EXPR_ERROR:
        case EXPR_NUMBER:
        case EXPR_RANGE1D_COLUMN:
            return expr;
        case EXPR_FUNCTION_CALL:{
            FunctionCall* fc = (FunctionCall*)expr;
            return fc->func(ctx, fc->argc, fc->argv);
        }
        case EXPR_RANGE0D:{
            double num;
            Range0D* rng = (Range0D*)expr;
            int err = evaluate(ctx, rng->row, rng->col, &num);
            if(err) return Error(ctx, "");
            Number* n = expr_alloc(ctx, EXPR_NUMBER);
            n->value = num;
            return &n->e;
        }
        case EXPR_BINARY:{
            Binary* b = (Binary*)expr;
            Expression* lhs = evaluate_expr(ctx, b->lhs);
            if(!lhs) return NULL;
            if(lhs->kind != EXPR_NUMBER) return Error(ctx, "");
            Expression* rhs = evaluate_expr(ctx, b->rhs);
            if(!rhs) return NULL;
            if(rhs->kind != EXPR_NUMBER) return Error(ctx, "");
            Number* res = (Number*)expr_alloc(ctx, EXPR_NUMBER);
            if(!res) return NULL;
            double l = ((Number*)lhs)->value;
            double r = ((Number*)rhs)->value;
            switch(b->op){
                case BIN_ADD: res->value = l +  r; break;
                case BIN_SUB: res->value = l -  r; break;
                case BIN_MUL: res->value = l *  r; break;
                case BIN_DIV: res->value = l /  r; break;
                case BIN_LT:  res->value = l <  r; break;
                case BIN_LE:  res->value = l <= r; break;
                case BIN_GT:  res->value = l >  r; break;
                case BIN_GE:  res->value = l >= r; break;
                case BIN_EQ:  res->value = l == r; break;
                case BIN_NE:  res->value = l != r; break;
                default: __builtin_trap();
            }
            return &res->e;
        }
        case EXPR_UNARY:{
            Unary* u = (Unary*)expr;
            Expression* v = evaluate_expr(ctx, u->expr);
            if(!v) return NULL;
            if(v->kind != EXPR_NUMBER) return Error(ctx, "");
            if(u->op == UN_PLUS) return u->expr;
            double d = ((Number*)v)->value;
            Number* r = (Number*)expr_alloc(ctx, EXPR_NUMBER);
            switch(u->op){
                case UN_NEG: r->value = -d; break;
                case UN_NOT: r->value = !d; break;
                case UN_PLUS: __builtin_unreachable();
                default: __builtin_trap();
            }
            return &r->e;
        }
        case EXPR_GROUP:{
            Group* g = (Group*)expr;
            return evaluate_expr(ctx, g->expr);
        };
    };
    return NULL;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
