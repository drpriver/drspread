//
// Copyright © 2023, David Priver <david@davidpriver.com>
//
#ifndef DRSPREAD_C
#define DRSPREAD_C

#include "drspread.h"
#include "drspread_evaluate.h"
#include "drspread_parse.h"
#include "stringview.h"
#include "drspread_types.h"


#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

#ifndef arrlen
#define arrlen(x) (sizeof(x)/sizeof(x[0]))
#endif

// null and 0 are false
// everything else is true

// unary:
//   -
//   +
//   !

// binary:
//   +
//   -
//   /
//   *
//   >
//   >=
//   <
//   <=
//   =, ==
//   !=
//   ~ for cat?

// Terminals:
//   function call
//   number
//   range
//   'string', "string"
//   (group)

force_inline
int
sp_set_display_number(const DrSpreadCtx* ctx, SheetHandle sheet, intptr_t row, intptr_t col, double value);

force_inline
int
sp_set_display_error(const DrSpreadCtx* ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* errmess, size_t errmess_len);

force_inline
int
sp_set_display_string(const DrSpreadCtx* ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* txt, size_t len);

DRSP_EXPORT
int
drsp_evaluate_formulas(DrSpreadCtx* ctx){
    int nerrs = 0;
    #if !defined(__wasm__) && defined(FrameAddress)
        ctx->limit = FrameAddress() - 300000;
    #endif
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    for(size_t i = 0; i < ctx->map.n; i++){
        SheetData* sd = &ctx->map.data[i];
        for(intptr_t row = 0; row < sd->height; row++){
            for(intptr_t col = 0; col < sd->width; col++){
                buff_set(ctx->a, bc);
                Expression* e = evaluate(ctx, sd, row, col);
                // benchmarking
                #ifdef BENCHMARKING
                    for(int i = 0; i < 100000; i++){
                        buff_set(ctx->a, bc);
                        e = evaluate(ctx, sd, row, col);
                    }
                #endif
                if(!e){ // OOM, don't cache the result.
                    nerrs++;
                    sp_set_display_error(ctx, sd->handle, row, col, "oom", 5);
                    continue;
                }
                CachedResult* cr = get_cached_result(&sd->result_cache, row, col);
                if(cr){
                    CachedResult tmp_cr;
                    tmp_cr.loc = (RowCol){row, col};
                    int err = expr_to_cached_result(ctx, e, &tmp_cr);
                    if(!err){
                        if(cached_result_eq_ignoring_loc(cr, &tmp_cr)){
                            if(tmp_cr.kind == CACHED_RESULT_ERROR)
                                nerrs++;
                            continue;
                        }
                        *cr = tmp_cr;
                        switch(tmp_cr.kind){
                            case CACHED_RESULT_NULL:
                                sp_set_display_string(ctx, sd->handle, row, col, "", 0);
                                continue;
                            case CACHED_RESULT_NUMBER:
                                sp_set_display_number(ctx, sd->handle, row, col, tmp_cr.number);
                                continue;
                            case CACHED_RESULT_STRING:
                                sp_set_display_string(ctx, sd->handle, row, col, tmp_cr.string->data, tmp_cr.string->length);
                                continue;
                            default: break;
                        }
                        nerrs++;
                        sp_set_display_error(ctx, sd->handle, row, col, "error", 5);
                        continue;
                    }
                }
                // Fallback, don't cache the result.
                // GCOV_EXCL_START
                switch(e->kind){
                    case EXPR_NUMBER:
                        sp_set_display_number(ctx, sd->handle, row, col, ((Number*)e)->value);
                        continue;
                    case EXPR_STRING:
                        sp_set_display_string(ctx, sd->handle, row, col, ((String*)e)->sv.text, ((String*)e)->sv.length);
                        continue;
                    case EXPR_BLANK:
                        sp_set_display_string(ctx, sd->handle, row, col, "", 0);
                        continue;
                    default: break;
                }
                nerrs++;
                sp_set_display_error(ctx, sd->handle, row, col, "error", 5);
                // GCOV_EXCL_STOP
            }
        }
    }
    buff_set(ctx->a, bc);
    return nerrs;
}

DRSP_EXPORT
int
drsp_evaluate_string(DrSpreadCtx* ctx, SheetHandle sheethandle, const char* txt, size_t len, DrSpreadResult* outval, intptr_t row, intptr_t col){
    #if !defined(__wasm__) && defined(FrameAddress)
        ctx->limit = FrameAddress() - 300000;
    #endif
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    SheetData* sd = sheet_lookup_by_handle(ctx, sheethandle);
    if(!sd) return 1;
    Expression* e = evaluate_string(ctx, sd, txt, len, row, col);
    int error = 0;
    if(!e){
        error = 1;
        goto finish;
    }
    switch(e->kind){
        case EXPR_BLANK:
            outval->kind = DRSP_RESULT_NULL;
            break;
        case EXPR_NUMBER:
            outval->kind = DRSP_RESULT_NUMBER;
            outval->d = ((Number*)e)->value;
            break;
        case EXPR_STRING:{
            outval->kind = DRSP_RESULT_STRING;
            StringView sv = ((String*)e)->sv;
            DrspStr* s = drsp_create_str(ctx, sv.text, sv.length);
            if(!s){
                error = 1;
                goto finish;
            }
            outval->s.length = s->length;
            outval->s.text = s->data;
        }break;
        default:
            error = 1;
            break;
    }
    finish:
    buff_set(ctx->a, bc);
    return error;
}

DRSP_EXPORT
int
drsp_evaluate_function(DrSpreadCtx* ctx, SheetHandle func, size_t nargs, const StringView*_Null_unspecified targs, DrSpreadResult* outval){
    #if !defined(__wasm__) && defined(FrameAddress)
        ctx->limit = FrameAddress() - 300000;
    #endif
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    if(nargs > 4) return 1;
    if(nargs && !targs) return 1;
    SheetData* udf = sheet_lookup_by_handle(ctx, func);
    if(!udf) return 1;
    if(!(udf->flags & DRSP_SHEET_FLAGS_IS_FUNCTION))
        return 1;
    Expression* args[4];
    int error = 0;
    SheetData tmp = {0};
    for(size_t i = 0; i < nargs; i++){
        // Having to pass in a sheet data is weird as we end up
        // with access to non-existent sheets...
        // Maybe we should require the caller to pass in a SheetHandle?
        Expression* e = evaluate_string(ctx, &tmp, targs[i].text, targs[i].length, -2, -2);
        if(!e || e->kind == EXPR_ERROR){
            error = 1;
            goto finish;
        }
        args[i] = e;
    }
    Expression* e = call_udf(ctx, udf, nargs, args);
    if(!e){
        error = 1;
        goto finish;
    }
    switch(e->kind){
        case EXPR_BLANK:
            outval->kind = DRSP_RESULT_NULL;
            break;
        case EXPR_NUMBER:
            outval->kind = DRSP_RESULT_NUMBER;
            outval->d = ((Number*)e)->value;
            break;
        case EXPR_STRING:{
            outval->kind = DRSP_RESULT_STRING;
            StringView sv = ((String*)e)->sv;
            DrspStr* s = drsp_create_str(ctx, sv.text, sv.length);
            if(!s){
                error = 1;
                goto finish;
            }
            outval->s.length = s->length;
            outval->s.text = s->data;
        }break;
        default:
            error = 1;
            break;
    }
    finish:
    // XXX: I don't think this is needed as sheetdata could actually be
    // const?
    cleanup_sheet_data(&tmp);
    buff_set(ctx->a, bc);
    return error;
}

force_inline
int
sp_set_display_number(const DrSpreadCtx* ctx, SheetHandle sheet, intptr_t row, intptr_t col, double value){
    #ifdef DRSPREAD_DIRECT_OPS
        (void)ctx;
        sheet_set_display_number(sheet, row, col, value);
        return 0;
    #else
        return ctx->_ops.set_display_number(ctx->_ops.ctx, sheet, row, col, value);
    #endif
}

force_inline
int
sp_set_display_error(const DrSpreadCtx* ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* errmess, size_t errmess_len){
    #ifdef DRSPREAD_DIRECT_OPS
        (void)ctx;
        (void)errmess;
        (void)errmess_len;
        sheet_set_display_error(sheet, row, col);
        return 0;
    #else
        return ctx->_ops.set_display_error(ctx->_ops.ctx, sheet, row, col, errmess, errmess_len);
    #endif
}

force_inline
int
sp_set_display_string(const DrSpreadCtx* ctx, SheetHandle sheet, intptr_t row, intptr_t col, const char* txt, size_t len){
    #ifdef DRSPREAD_DIRECT_OPS
        (void)ctx;
        sheet_set_display_string(sheet, row, col, txt, len);
        return 0;
    #else
        return ctx->_ops.set_display_string(ctx->_ops.ctx, sheet, row, col, txt, len);
    #endif
}


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#include "drspread_parse.c"
#include "drspread_formula_funcs.c"
#include "drspread_evaluate.c"
#include "drspread_types.c"
#include "drspread_allocators.c"
#include "drspread_colcache.c"
#endif
