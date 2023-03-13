//
// Copyright © 2023, David Priver
//
#ifndef DRSPREAD_C
#define DRSPREAD_C

#include "drspread_evaluate.h"
#include "drspread_parse.h"
#include "stringview.h"
#include "drspread_types.h"
#include "drspread.h"


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

DRSP_EXPORT
int
drsp_evaluate_formulas(DrSpreadCtx* ctx, SheetHandle sheethandle, SheetHandle _Null_unspecified*_Nullable sheetdeps, size_t sheetdepslen){
    if(sheetdeps)
        __builtin_memset(sheetdeps, 0, sheetdepslen * sizeof *sheetdeps);
    int nerrs = 0;
    #ifndef __wasm__
    ctx->limit = (uintptr_t)__builtin_frame_address(0) - 300000;
    #endif
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    SheetData* sd = sheet_lookup_by_handle(ctx, sheethandle);
    if(!sd) return -1;
    for(intptr_t row = 0; row < sd->height; row++){
        for(intptr_t col = 0; col < sd->width; col++){
            buff_set(ctx->a, bc);
            Expression* e = evaluate(ctx, sheethandle, row, col);
            // benchmarking
            #ifdef BENCHMARKING
                for(int i = 0; i < 100000; i++){
                    buff_set(ctx->a, bc);
                    e = evaluate(ctx, sheethandle, row, col);
                }
            #endif
            if(!e){ // OOM, don't cache the result.
                nerrs++;
                sp_set_display_error(ctx, sheethandle, row, col, "oom", 5);
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
                            sp_set_display_string(ctx, sheethandle, row, col, "", 0);
                            continue;
                        case CACHED_RESULT_NUMBER:
                            sp_set_display_number(ctx, sheethandle, row, col, tmp_cr.number);
                            continue;
                        case CACHED_RESULT_STRING:
                            sp_set_display_string(ctx, sheethandle, row, col, tmp_cr.string->data, tmp_cr.string->length);
                            continue;
                        default: break;
                    }
                    nerrs++;
                    sp_set_display_error(ctx, sheethandle, row, col, "error", 5);
                    continue;
                }
            }
            // Fallback, don't cache the result.
            switch(e->kind){
                case EXPR_NUMBER:
                    sp_set_display_number(ctx, sheethandle, row, col, ((Number*)e)->value);
                    continue;
                case EXPR_STRING:
                    sp_set_display_string(ctx, sheethandle, row, col, ((String*)e)->sv.text, ((String*)e)->sv.length);
                    continue;
                case EXPR_NULL:
                    sp_set_display_string(ctx, sheethandle, row, col, "", 0);
                    continue;
                default: break;
            }
            nerrs++;
            sp_set_display_error(ctx, sheethandle, row, col, "error", 5);
        }
    }
#if 0
    if(sheetdeps)
        for(size_t i = 0; i < arrlen(ctx.sheetcache.items) && i < sheetdepslen; i++){
            if(!ctx.sheetcache.items[i].s.length)
                break;
            sheetdeps[i] = ctx.sheetcache.items[i].sheet;
        }
#else
    (void)sheetdeps, (void)sheetdepslen;
#endif
    buff_set(ctx->a, bc);
    return nerrs;
}

DRSP_EXPORT
int
drsp_evaluate_string(DrSpreadCtx* ctx, SheetHandle sheethandle, const char* txt, size_t len, DrSpreadResult* outval, intptr_t row, intptr_t col){
    #ifndef __wasm__
    ctx->limit = (uintptr_t)__builtin_frame_address(0) - 300000;
    #endif
    BuffCheckpoint bc = buff_checkpoint(ctx->a);
    Expression* e = evaluate_string(ctx, sheethandle, txt, len, row, col);
    int error = 0;
    if(!e){
        error = 1;
        goto finish;
    }
    switch(e->kind){
        case EXPR_NULL:
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


#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#include "drspread_parse.c"
#include "drspread_formula_funcs.c"
#include "drspread_evaluate.c"
#include "drspread_types.c"
#endif
