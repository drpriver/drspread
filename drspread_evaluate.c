//
// Copyright © 2023-2025, David Priver <david@davidpriver.com>
//
#ifndef DRSPREAD_EVALUATE_C
#define DRSPREAD_EVALUATE_C
#include "drspread_types.h"
#include "drspread_evaluate.h"
#include "drspread_parse.h"
#include "drspread_utils.h"
#include "parse_numbers.h"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

DRSP_INTERNAL
Expression*_Nullable
evaluate(DrSpreadCtx* ctx, SheetData* sd, intptr_t row, intptr_t col){
    {
        #ifdef FrameAddress
        uintptr_t frm = FrameAddress();
            #ifdef __wasm__
                uintptr_t limit = 10000;
                if(frm < limit) return NULL;
            #else
                uintptr_t limit = ctx->limit;
                if(frm < limit) return NULL;
            #endif
        #endif
    }
    // HACK
    if(unlikely(sd->flags & DRSP_SHEET_FLAGS_IS_FUNCTION)){
        for(size_t i = 0; i < arrlen(sd->hacky_func_args); i++){
            const struct HackyFuncArg* hfa = &sd->hacky_func_args[i];
            if(!hfa->e) break;
            if(hfa->row == row && hfa->col == col)
                return hfa->e;
        }
    }
    if(row != IDX_EXTRA_DIMENSIONAL)
        if(row < 0 || col < 0 || row >= sd->height || col >= sd->width)
            return expr_alloc(ctx, EXPR_BLANK);
    const DrspAtom a = sp_cell_atom(sd, row, col);
    if(a == drsp_nil_atom()) return expr_alloc(ctx, EXPR_BLANK);
    StringView sv = {a->length, a->data};
    assert(sv.length);
    // These `goto`s are a bit unorthodox, but it is basically a switch,
    // with the ability of cell_number to jump to cell_other
    // if(!sv.length) goto cell_empty;
    if(sv.text[0] == '=')
        goto cell_formula;
    if((sv.text[0] >= '0' && sv.text[0] <= '9') || sv.text[0] == '.' || sv.text[0] == '-')
        goto cell_number;
    goto cell_other;
    /*
    {
        cell_empty:;
        Expression* e = expr_alloc(ctx, EXPR_BLANK);
        return e;
    }
    */
    {
        cell_other:;
        String* s = expr_alloc(ctx, EXPR_STRING);
        if(!s) return NULL;
        s->str = a;
        if(!s->str) return NULL;
        return &s->e;
    }
    {
        cell_number:;
        DoubleResult dr = parse_double(sv.text, sv.length);
        if(dr.errored) goto cell_other;
        double value = dr.result;
        Number* n = expr_alloc(ctx, EXPR_NUMBER);
        if(!n) return NULL;
        n->value = value;
        return &n->e;
    }
    {
        cell_formula:;
        if(1 && !(sd->flags & DRSP_SHEET_FLAGS_IS_FUNCTION)){
            CachedResult* cr = has_cached_output_result(&sd->result_cache, row, col);
            if(cr) return cached_result_to_expr(ctx, cr);
        }
        BuffCheckpoint bc = buff_checkpoint(ctx->a);
        Expression *root = parse(ctx, a);
        if(!root || root->kind == EXPR_ERROR){
            buff_set(ctx->a, bc);
            return root;
        }
        Expression *e = evaluate_expr(ctx, sd, root, row, col);
        if(!e || e->kind == EXPR_ERROR) {
            buff_set(ctx->a, bc);
            return e;
        }
        _Alignas(union ExprU) unsigned char tmp[sizeof(union ExprU)];
        ExpressionKind kind = e->kind;
        if(kind == EXPR_COMPUTED_ARRAY)
            return e;
        size_t sz = expr_size(kind);
        __builtin_memcpy(tmp, e, sz);
        buff_set(ctx->a, bc);
        Expression* r = expr_alloc(ctx, kind);
        __builtin_memcpy(r, tmp, sz);
        if(1 && !(sd->flags & DRSP_SHEET_FLAGS_IS_FUNCTION)){
            CachedResult* cr = get_cached_output_result(&sd->result_cache, row, col);
            if(cr){
                int err = expr_to_cached_result_no_array(ctx, r, cr);
                (void)err;
            }
        }
        return r;
    }
}

DRSP_INTERNAL
Expression*_Nullable
evaluate_string(DrSpreadCtx* ctx, SheetData* sd, const char* txt, size_t len, intptr_t caller_row, intptr_t caller_col){
    DrspAtom a = drsp_intern_str(ctx, txt, len);
    Expression *root = parse(ctx, a);
    if(!root || root->kind == EXPR_ERROR) return root;
    Expression *e = evaluate_expr(ctx, sd, root, caller_row, caller_col);
    return e;
}

static inline
double
double_bin_cmp(BinaryKind op, double l, double r){
    double value;
    switch(op){
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
        // GCOV_EXCL_START
        default: __builtin_trap();
    }
        // GCOV_EXCL_STOP
    return value;
}


// XXX: where are lhs and rhs nullable?
static inline
Expression*_Nullable
evaluate_binary_op(DrSpreadCtx* ctx, SheetData* sd, BinaryKind op, Expression*_Nullable lhs, Expression*_Nullable rhs, intptr_t caller_row, intptr_t caller_col){
    Expression* result = NULL;
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
#define BAD(x) do{result = x; goto cleanup;}while(0)
    {
        lhs = evaluate_expr(ctx, sd, lhs, caller_row, caller_col);
        if(!lhs || lhs->kind == EXPR_ERROR)
            BAD(lhs);
        rhs = evaluate_expr(ctx, sd, rhs, caller_row, caller_col);
        if(!rhs || rhs->kind == EXPR_ERROR)
            BAD(rhs);
        _Bool larraylike = expr_is_arraylike(lhs);
        _Bool rarraylike = expr_is_arraylike(rhs);
        if(larraylike || rarraylike){
            if(larraylike && !rarraylike){
                if(rhs->kind != EXPR_STRING && rhs->kind != EXPR_NUMBER)
                    BAD(Error(ctx, "rhs is not string or number"));
                lhs = convert_to_computed_array(ctx, sd, lhs, caller_row, caller_col);
                if(!lhs || lhs->kind == EXPR_ERROR)
                    BAD(lhs);
                ComputedArray* l = (ComputedArray*)lhs;
                if(rhs->kind == EXPR_STRING){
                    DrspAtom r = ((String*)rhs)->str;
                    for(intptr_t i = 0; i < l->length; i++){
                        Expression* e = l->data[i];
                        if(e->kind == EXPR_BLANK) continue;
                        if(e->kind != EXPR_STRING)
                            BAD(Error(ctx, "lhs is not a string"));
                        _Bool cmp;
                        DrspAtom s = ((String*)e)->str;
                        switch(op){
                            case BIN_EQ:
                                cmp = s == r;
                                break;
                            case BIN_NE:
                                cmp = s != r;
                                break;
                            default:
                                BAD(Error(ctx, "only '=' and '!=' supported for strings"));
                        }
                        Number* n = expr_alloc(ctx, EXPR_NUMBER);
                        if(!n) return NULL;
                        n->value = (double)cmp;
                        l->data[i] = &n->e;
                    }
                }
                else {
                    assert(rhs->kind == EXPR_NUMBER);
                    double r = ((Number*)rhs)->value;
                    for(intptr_t i = 0; i < l->length; i++){
                        Expression* e = l->data[i];
                        if(e->kind == EXPR_BLANK) continue;
                        if(e->kind != EXPR_NUMBER)
                            BAD(Error(ctx, "lhs is not a number"));
                        Number* n = (Number*)e;
                        n->value = double_bin_cmp(op, n->value, r);
                    }
                }
                return lhs;
            }
            else if(rarraylike && !larraylike){
                if(lhs->kind != EXPR_STRING && lhs->kind != EXPR_NUMBER)
                    BAD(Error(ctx, "lhs is not string or number"));
                rhs = convert_to_computed_array(ctx, sd, rhs, caller_row, caller_col);
                if(!rhs || rhs->kind == EXPR_ERROR)
                    BAD(rhs);
                ComputedArray* r = (ComputedArray*)rhs;
                if(lhs->kind == EXPR_STRING){
                    DrspAtom l = ((String*)lhs)->str;
                    for(intptr_t i = 0; i < r->length; i++){
                        Expression* e = r->data[i];
                        if(e->kind == EXPR_BLANK) continue;
                        if(e->kind != EXPR_STRING)
                            BAD(Error(ctx, "rhs is not a string"));
                        _Bool cmp;
                        DrspAtom s = ((String*)e)->str;
                        switch(op){
                            case BIN_EQ:
                                cmp = s == l;
                                break;
                            case BIN_NE:
                                cmp = s != l;
                                break;
                            default:
                                BAD(Error(ctx, "only '=' and '!=' supported for strings"));
                        }
                        Number* n = expr_alloc(ctx, EXPR_NUMBER);
                        if(!n) return NULL;
                        n->value = (double)cmp;
                        r->data[i] = &n->e;
                    }
                }
                else {
                    assert(lhs->kind == EXPR_NUMBER);
                    double l = ((Number*)lhs)->value;
                    for(intptr_t i = 0; i < r->length; i++){
                        Expression* e = r->data[i];
                        if(e->kind == EXPR_BLANK) continue;
                        if(e->kind != EXPR_NUMBER)
                            BAD(Error(ctx, "rhs is not a number"));
                        Number* n = (Number*)e;
                        n->value = double_bin_cmp(op, l, n->value);
                    }
                }
                return rhs;
            }
            else {
                assert(rarraylike && larraylike);
                // XXX: we could lazily iterate over the row range, but fuck it.
                if(rhs->kind == EXPR_RANGE1D_ROW || rhs->kind == EXPR_RANGE1D_ROW_FOREIGN){
                    rhs = convert_to_computed_array(ctx, sd, rhs, caller_row, caller_col);
                    if(!rhs || rhs->kind == EXPR_ERROR)
                        BAD(rhs);
                }
                if(lhs->kind != EXPR_COMPUTED_ARRAY && rhs->kind == EXPR_COMPUTED_ARRAY){
                    // Special case to avoid allocation
                    _Bool can_swap = 0;
                    // Could add swapped binary operands.
                    switch(op){
                        case BIN_ADD: can_swap = 1; break;
                        case BIN_SUB: break;
                        case BIN_MUL: can_swap = 1; break;
                        case BIN_DIV: break;
                        case BIN_LT:  can_swap = 1; op = BIN_GT; break;
                        case BIN_LE:  can_swap = 1; op = BIN_GE; break;
                        case BIN_GT:  can_swap = 1; op = BIN_LT; break;
                        case BIN_GE:  can_swap = 1; op = BIN_LE; break;
                        case BIN_EQ:  can_swap = 1; break;
                        case BIN_NE:  can_swap = 1; break;
                    }
                    if(can_swap){
                        Expression* tmp = lhs;
                        lhs = rhs;
                        rhs = tmp;
                    }
                }
                lhs = convert_to_computed_array(ctx, sd, lhs, caller_row, caller_col);
                if(!lhs || lhs->kind == EXPR_ERROR)
                    BAD(lhs);
                ComputedArray* l = (ComputedArray*)lhs;
                // XXX: we could lazily iterate over the row range, but fuck it.
                if(rhs->kind == EXPR_RANGE1D_ROW || rhs->kind == EXPR_RANGE1D_ROW_FOREIGN){
                    rhs = convert_to_computed_array(ctx, sd, rhs, caller_row, caller_col);
                    if(!rhs || rhs->kind == EXPR_ERROR)
                        BAD(rhs);
                }

                if(rhs->kind == EXPR_COMPUTED_ARRAY){
                    ComputedArray* r = (ComputedArray*)rhs;
                    if(l->length != r->length){
                        BAD(Error(ctx, "lhs not same length as rhs"));
                    }
                    for(intptr_t i = 0; i < l->length; i++){
                        Expression* e = evaluate_binary_op(ctx, sd, op, l->data[i], r->data[i], caller_row, caller_col);
                        if(!e || e->kind == EXPR_ERROR)
                            BAD(e);
                        l->data[i] = e;
                    }
                    return lhs;
                }
                SheetData* rsd = sd;
                intptr_t col, rstart, rend;
                if(get_range1dcol(ctx, sd, rhs, &col, &rstart, &rend, &rsd, caller_row, caller_col))
                    BAD(Error(ctx, "Bad Range"));
                if(rend - rstart +1 != l->length)
                    BAD(Error(ctx, "lhs not same length as rhs"));
                for(intptr_t row = rstart, i = 0; row <= rend; row++, i++){
                    BuffCheckpoint bc = buff_checkpoint(ctx->a);
                    Expression* ld = l->data[i];
                    if(ld->kind == EXPR_BLANK)
                        continue;
                    Expression* e = evaluate(ctx, rsd, row, col);
                    if(!e || e->kind == EXPR_ERROR) BAD(e);
                    if(ld->kind != e->kind){
                        BAD(Error(ctx, "lhs not same type as rhs"));
                    }
                    if(ld->kind == EXPR_NUMBER){
                        ((Number*)ld)->value = double_bin_cmp(op, ((Number*)ld)->value, ((Number*)e)->value);
                        buff_set(ctx->a, bc);
                        continue;
                    }
                    if(ld->kind == EXPR_STRING){
                        _Bool cmp;
                        switch(op){
                            case BIN_EQ:
                                cmp = ((String*)ld)->str == ((String*)e)->str;
                                break;
                            case BIN_NE:
                                cmp = ((String*)ld)->str != ((String*)e)->str;
                                break;
                            default:
                                BAD(Error(ctx, "only '=' and '!=' supported for strings"));
                        }
                        buff_set(ctx->a, bc);
                        Number* res = expr_alloc(ctx, EXPR_NUMBER);
                        res->value = cmp;
                        l->data[i] = &res->e;
                        continue;
                    }
                    BAD(Error(ctx, "lhs is not a string or number"));
                }
                return lhs;
            }
        }
        else {
            if(lhs->kind == EXPR_BLANK){
                result = lhs;
                goto cleanup;
            }
            if(rhs->kind == EXPR_BLANK){
                result = rhs;
                goto cleanup;
            }
            if(lhs->kind != EXPR_NUMBER && lhs->kind != EXPR_STRING)
                BAD(Error(ctx, "lhs is not a string or number"));
            if(lhs->kind == EXPR_STRING){
                if(rhs->kind != EXPR_STRING)
                    BAD(Error(ctx, "rhs not a string"));
                String* l = (String*)lhs;
                String* r = (String*)rhs;
                _Bool cmp;
                switch(op){
                    case BIN_EQ:
                        cmp = l->str == r->str;
                        break;
                    case BIN_NE:
                        cmp = l->str != r->str;
                        break;
                    default:
                        BAD(Error(ctx, "only '=' and '!=' supported for strings"));
                }
                buff_set(ctx->a, bc);
                Number* res = (Number*)expr_alloc(ctx, EXPR_NUMBER);
                res->value = cmp;
                return &res->e;
            }
            if(lhs->kind != EXPR_NUMBER)
                BAD(Error(ctx, "lhs not a number"));
            if(rhs->kind != EXPR_NUMBER)
                BAD(Error(ctx, "rhs not a number"));
            double l = ((Number*)lhs)->value;
            double r = ((Number*)rhs)->value;
            double value = double_bin_cmp(op, l, r);
            buff_set(ctx->a, bc);
            Number* res = (Number*)expr_alloc(ctx, EXPR_NUMBER);
            if(!res) return NULL;
            res->value = value;
            return &res->e;
        }
    }
#undef BAD
    cleanup:
    buff_set(ctx->a, bc);
    return result;
}

DRSP_INTERNAL
Expression*_Nullable
evaluate_expr(DrSpreadCtx* ctx, SheetData* sd, Expression* expr, intptr_t caller_row, intptr_t caller_col){
    switch(expr->kind){
        case EXPR_BLANK:
        case EXPR_ERROR:
        case EXPR_NUMBER:
        case EXPR_RANGE1D_ROW:
        case EXPR_RANGE1D_ROW_FOREIGN:
        case EXPR_COMPUTED_ARRAY:
        case EXPR_STRING:
            return expr;
        case EXPR_RANGE1D_COLUMN:{
            Range1DColumn* rng = (Range1DColumn*)expr;
            if(rng->row_start == 0 && rng->row_end == -1){
                // this could be a named cell
                const NamedCell* cell = get_named_cell(&sd->named_cells, rng->col_name);
                if(cell) return evaluate(ctx, sd, cell->row, cell->col);
            }
            return expr;
        }
        case EXPR_RANGE1D_COLUMN_FOREIGN:{
            ForeignRange1DColumn* rng = (ForeignRange1DColumn*)expr;
            if(rng->r.row_start == 0 && rng->r.row_end == -1){
                // this could be a named cell.
                SheetData* fsd = sheet_lookup_by_name(ctx, rng->sheet_name);
                if(fsd){
                    if(fsd != sd){
                        int err = sheet_add_dependant(ctx, fsd, sd->handle);
                        if(err) return Error(ctx, "oom");
                    }
                    const NamedCell* cell = get_named_cell(&fsd->named_cells, rng->r.col_name);
                    if(cell) return evaluate(ctx, fsd, cell->row, cell->col);
                }
            }
            return expr;
        }
        case EXPR_FUNCTION_CALL:{
            FunctionCall* fc = (FunctionCall*)expr;
            return fc->func(ctx, sd, caller_row, caller_col, fc->argc, fc->argv);
        }
        case EXPR_RANGE0D_FOREIGN:{
            ForeignRange0D* rng = (ForeignRange0D*)expr;
            SheetData* fsd = sheet_lookup_by_name(ctx, rng->sheet_name);
            if(!fsd) return Error(ctx, "");
            if(fsd != sd){
                int err = sheet_add_dependant(ctx, fsd, sd->handle);
                if(err) return Error(ctx, "oom");
            }
            intptr_t r = rng->r.row;
            if(r == IDX_DOLLAR) r = caller_row;
            intptr_t c;
            if(rng->r.col_name == drsp_dollar_atom())
                c = caller_col;
            else
                c = sp_name_to_col_idx(fsd, rng->r.col_name);
            if(c == IDX_DOLLAR) c = caller_col;
            if(c == -1) return Error(ctx, "column not found");
            return evaluate(ctx, fsd, r, c);
        }
        case EXPR_RANGE0D:{
            Range0D* rng = (Range0D*)expr;
            intptr_t r = rng->row;
            if(r == IDX_DOLLAR) r = caller_row;
            intptr_t c;
            if(rng->col_name == drsp_dollar_atom())
                c = caller_col;
            else
                c = sp_name_to_col_idx(sd, rng->col_name);
            if(c == IDX_DOLLAR) c = caller_col;
            if(c == -1) return Error(ctx, "column not found");
            return evaluate(ctx, sd, r, c);
        }
        case EXPR_BINARY:{
            Binary* b = (Binary*)expr;
            return evaluate_binary_op(ctx, sd, b->op, b->lhs, b->rhs, caller_row, caller_col);
        }
        case EXPR_UNARY:{
            char* chk = ctx->a->cursor;
            Unary* u = (Unary*)expr;
            Expression* v = evaluate_expr(ctx, sd, u->expr, caller_row, caller_col);
            if(!v) return NULL;
            if(v->kind != EXPR_NUMBER) return Error(ctx, "");
            if(u->op == UN_PLUS) return u->expr;
            double d = ((Number*)v)->value;
            double value;
            switch(u->op){
                case UN_NEG: value = -d; break;
                case UN_NOT: value = !d; break;
                // GCOV_EXCL_START
                case UN_PLUS: __builtin_unreachable();
                default: __builtin_trap();
            }
                // GCOV_EXCL_STOP
            ctx->a->cursor = chk;
            Number* r = expr_alloc(ctx, EXPR_NUMBER);
            if(!r) return NULL;
            r->value = value;
            return &r->e;
        }
        // GCOV_EXCL_START
        // This is actually unreachable due to the way we parse things.
        case EXPR_GROUP:{
            Group* g = (Group*)expr;
            // TODO: check if this tail calls.
            // Otherwise we need a goto to the top.
            return evaluate_expr(ctx, sd, g->expr, caller_row, caller_col);
        }
        // GCOV_EXCL_STOP
        case EXPR_USER_DEFINED_FUNC_CALL:{
            UserFunctionCall* ufc = (UserFunctionCall*)expr;
            SheetData* udf = udf_lookup_by_name(ctx, ufc->name);
            if(!udf) return Error(ctx, "Can't find udf of this name");
            {
                int err = sheet_add_dependant(ctx, udf, sd->handle);
                if(err) return Error(ctx, "oom");
            }
            if(udf->paramc != ufc->argc){
                return Error(ctx, "Wrong number of args");
            }
            int argc = ufc->argc;
            Expression* args[4];
            for(int i = 0; i < argc; i++){
                Expression* e = evaluate_expr(ctx, sd, ufc->argv[i], caller_row, caller_col);
                if(!e || e->kind == EXPR_ERROR) return e;
                if(expr_is_arraylike(e))
                    e = convert_to_computed_array(ctx, sd, e, caller_row, caller_col);
                if(!e || e->kind == EXPR_ERROR) return e;
                args[i] = e;
            }
            return call_udf(ctx, udf, argc, args);
        }
    }
    // GCOV_EXCL_START
    // should be unreachable
    return NULL;
    // GCOV_EXCL_STOP
}

DRSP_INTERNAL
Expression*_Nullable
call_udf(DrSpreadCtx* ctx, SheetData* func, size_t nargs, Expression*_Nonnull*_Nonnull args){
    if(nargs != (size_t)func->paramc)
        return Error(ctx, "mismatched number of args");
    for(size_t i = 0; i < arrlen(func->hacky_func_args); i++){
        if(func->hacky_func_args[i].e)
            return Error(ctx, "Recursion is not supported in user-defined functions");
    }
    __builtin_memset(func->hacky_func_args, 0, sizeof func->hacky_func_args);
    for(size_t i = 0; i < nargs; i++){
        func->hacky_func_args[i] = (struct HackyFuncArg){
            .col = func->params[i].col,
            .row = func->params[i].row,
            .e = args[i],
        };
    }
    Expression* result = evaluate(ctx, func, func->out_row, func->out_col);
    __builtin_memset(func->hacky_func_args, 0, sizeof func->hacky_func_args);
    return result;
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
