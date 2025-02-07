//
// Copyright © 2023-2025, David Priver <david@davidpriver.com>
//
#ifdef _WIN32
#define _CRT_NONSTDC_NO_WARNINGS 1
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifndef DRSP_TEST_DYLINK
#define DRSP_EXPORT static
#else
#define DRSP_DYLIB 1
#endif

#include "spreadsheet.h"
#include "drspread.h"
#include "testing.h"
#include "drspread_allocators.h"
#ifdef __wasm__
#pragma push_macro("__FILE__")
#pragma clang diagnostic ignored "-Wbuiltin-macro-redefined"
#define __FILE__ "<a href=TestDrSpread.c>TestDrSpread.c</a>"
#endif
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif

// secret API
DRSP_EXPORT
int
drsp_evaluate_function(DrSpreadCtx* ctx, SheetHandle func, size_t nargs, const StringView*_Null_unspecified args, DrSpreadResult* outval);

static
SheetHandle _Null_unspecified*_Nullable
drsp_sheet_get_dependants(DrSpreadCtx* ctx, SheetHandle h, size_t* n);

static _Bool drsp_sheet_is_dirty(DrSpreadCtx* ctx, SheetHandle h);


#define EXPECT_NO_LEAKS() do { \
    int leaks = drsp_report_leaks(); \
    TestExpectFalse(leaks); \
}while(0)


static TestFunc TestParsing;
static TestFunc TestSpreadsheet1;
static TestFunc TestSpreadsheet2;
static TestFunc TestBinOps;
static TestFunc TestUnOps;
static TestFunc TestFuncs;
static TestFunc TestFuncsV;
static TestFunc TestFuncsRowArray;
static TestFunc TestMod;
static TestFunc TestBugs;
static TestFunc TestBugs2;
static TestFunc TestBugs3;
static TestFunc TestDirectlyRecursiveShouldError;
static TestFunc TestIndirectlyRecursiveShouldError;
static TestFunc TestMultisheet;
static TestFunc TestColFunc;
static TestFunc TestRanges;
static TestFunc TestBadRanges;
static TestFunc TestNames;
static TestFunc TestComplexMultisheet;
static TestFunc TestCaching;
static TestFunc TestUserFunctions;
static TestFunc TestShortColNames;
static TestFunc TestDeleteColNames;
static TestFunc TestExtraDimensional;
static TestFunc TestEditing;
static TestFunc TestNamedCells;
static TestFunc TestDependants;

int main(int argc, char*_Null_unspecified*_Null_unspecified argv){
    if(!test_funcs_count){ // wasm calls main more than once.
        RegisterTest(TestParsing);
        RegisterTest(TestRanges);
        RegisterTest(TestBadRanges);
        RegisterTest(TestSpreadsheet1);
        RegisterTest(TestSpreadsheet2);
        RegisterTest(TestBinOps);
        RegisterTest(TestUnOps);
        RegisterTest(TestFuncs);
        RegisterTest(TestFuncsV);
        RegisterTest(TestFuncsRowArray);
        RegisterTest(TestMod);
        RegisterTest(TestBugs);
        RegisterTest(TestBugs2);
        RegisterTest(TestBugs3);
        RegisterTest(TestDirectlyRecursiveShouldError);
        RegisterTest(TestIndirectlyRecursiveShouldError);
        RegisterTest(TestMultisheet);
        RegisterTest(TestColFunc);
        RegisterTest(TestNames);
        RegisterTest(TestComplexMultisheet);
        RegisterTest(TestCaching);
        RegisterTest(TestUserFunctions);
        RegisterTest(TestShortColNames);
        RegisterTest(TestDeleteColNames);
        RegisterTest(TestExtraDimensional);
        RegisterTest(TestEditing);
        RegisterTest(TestNamedCells);
        #ifndef DRSP_TEST_DYLINK
        RegisterTest(TestDependants);
        #endif
    }
    int ret = test_main(argc, argv, NULL);
    return ret;
}
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#pragma clang diagnostic ignored "-Wgnu-auto-type"
#endif

#ifndef arrlen
#define arrlen(x) (sizeof(x)/sizeof(x[0]))
#endif

static inline
_Bool
streq(const char* a, const char* b){
    return strcmp(a,b) == 0;
}

static
struct
TestStats
test_spreadsheet(const char* caller, const char* input, const SheetRow* expected, size_t expected_len, int expected_nerr){
#define __func__ caller
    struct TestStats TEST_stats = {0};
    SpreadSheet sheet = {0};
    int err = read_csv_from_string(&sheet, input);
    TestAssertFalse(err);
    TestAssertEquals(sheet.rows, expected_len);
    SheetOps ops = sheet_ops();
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    SheetHandle sheethandle = (SheetHandle)&sheet;
    int e = drsp_set_sheet_name(ctx, sheethandle, "$this", 5);
    TestAssertFalse(e);
    for(intptr_t r = 0; r < sheet.rows; r++){
        const SheetRow* row = &sheet.cells[r];
        for(int c = 0; c < row->n; c++){
            if(!row->lengths[c]) continue;
            e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
            // don't bloat the stats
            if(e) TestAssertFalse(e);
            e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
            if(e) TestAssertFalse(e);
        }
    }
    int nerr = drsp_evaluate_formulas(ctx);
    TestExpectEquals(nerr, expected_nerr);
    for(size_t i = 0; i < expected_len; i++){
        const SheetRow* display_row = &sheet.display[i];
        const SheetRow* expected_row = &expected[i];
        TestAssertEquals(display_row->n, expected_row->n);
        for(int j = 0; j < display_row->n; j++){
            TEST_stats.executed++;
            const char* lhs = display_row->data[j];
            const char* rhs = expected_row->data[j];
            if(!streq(lhs, rhs)){
                TEST_stats.failures++;
                  TestReport("Test condition failed");
                  TestReport("row %zu, col %d", i, j);
                  TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                          _test_color_green, lhs, _test_color_reset,
                          _test_color_red, _test_color_reset,
                          _test_color_green, rhs, _test_color_reset);
            }
        }
    }
    drsp_destroy_ctx(ctx);

    ctx = drsp_create_ctx(&ops);
    e = drsp_set_sheet_name(ctx, sheethandle, "$this", 5);
    TestAssertFalse(e);
    for(intptr_t r = 0; r < sheet.rows; r++){
        const SheetRow* row = &sheet.cells[r];
        for(int c = 0; c < row->n; c++){
            e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
            if(!row->lengths[c]) continue;
            if(e){ // don't bloat the stats
                TestAssertFalse(e);
            }
        }
    }
    nerr = 0;
    for(int i = 0; i < sheet.rows; i++){
        const SheetRow* expected_row = &expected[i];
        const SheetRow* inp_row = &sheet.cells[i];
        TestAssertEquals(inp_row->n, expected_row->n);
        for(int col = 0; col < expected_row->n; col++){
            const char* expected_text = expected_row->data[col];
            StringView exp = {strlen(expected_text), expected_text};
            const char* inp_txt = inp_row->data[col];
            size_t inp_len = inp_row->lengths[col];
            StringView inp = stripped2(inp_txt, inp_len);
            if(!inp.length) continue;
            if(inp.text[0] != '=') continue;
            DrSpreadResult val;
            int err = drsp_evaluate_string(ctx, sheethandle, inp_txt, inp_len, &val, i, col);
            nerr += err;
            if(!err){
                switch(val.kind){
                    case DRSP_RESULT_NULL:
                        TestExpectEquals(exp.length, 0);
                        break;
                    case DRSP_RESULT_NUMBER:{
                        DoubleResult dr = parse_double(exp.text, exp.length);
                        TestAssertFalse(dr.errored);
                        double n = dr.result;
                        TestExpectEquals(n, val.d);
                    }break;
                    case DRSP_RESULT_STRING:{
                        StringView v = {val.s.length, val.s.text};
                        TestExpectEquals2(sv_equals, v, exp);
                    } break;
                    default:
                        TestExpectFalse(1);
                        break;
                }
            }
        }
    }
    drsp_destroy_ctx(ctx);
    TestExpectEquals(nerr, expected_nerr);
    cleanup_sheet(&sheet);
    EXPECT_NO_LEAKS();
#undef __func__
    return TEST_stats;
}

#define ROW1(a) {1, \
    ((const char*[]){""a}), \
    ((size_t[]){(sizeof(a))-1}) \
}
#define ROW2(a, b) {2, \
    ((const char*[]){""a, ""b}), \
    ((size_t[]){sizeof(a)-1, sizeof(b)-1}) \
}
#define ROW3(a, b, c) {3, \
    ((const char*[]){""a, ""b, ""c}), \
    ((size_t[]){sizeof(a)-1, sizeof(b)-1, sizeof(c)-1}) \
}
#define ROW4(a, b, c, d) {4, \
    ((const char*[]){""a, ""b, ""c, ""d}), \
    ((size_t[]){sizeof(a)-1, sizeof(b)-1, sizeof(c)-1, sizeof(d)-1}) \
}
#define ROW5(a, b, c, d, e) {5, \
    ((const char*[]){""a, ""b, ""c, ""d, ""e}), \
    ((size_t[]){sizeof(a)-1, sizeof(b)-1, sizeof(c)-1, sizeof(d)-1, sizeof(e)-1}) \
}
#define ROW8(a, b, c, d, e, f, g, h) {8, \
    ((const char*[]){""a, ""b, ""c, ""d, ""e, ""f, ""g, ""h}), \
    ((size_t[]){sizeof(a)-1, sizeof(b)-1, sizeof(c)-1, sizeof(d)-1, sizeof(e)-1, sizeof(f)-1, sizeof(g)-1, sizeof(h)-1}) \
}
#define ROW13(a, b, c, d, e, f, g, h, i, j, k, l, m) {13, \
    ((const char*[]){""a, ""b, ""c, ""d, ""e, ""f, ""g, ""h, ""i, ""j, ""k, ""l, ""m}), \
    ((size_t[]){sizeof(a)-1, sizeof(b)-1, sizeof(c)-1, sizeof(d)-1, sizeof(e)-1, sizeof(f)-1, sizeof(g)-1, sizeof(h)-1, sizeof(i)-1, sizeof(j)-1, sizeof(k)-1, sizeof(l)-1, sizeof(m)-1}) \
}
#define ROW16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {16, \
    ((const char*[]){""a, ""b, ""c, ""d, ""e, ""f, ""g, ""h, ""i, ""j, ""k, ""l, ""m, ""n, ""o, ""p}), \
    ((size_t[]){sizeof(a)-1, sizeof(b)-1, sizeof(c)-1, sizeof(d)-1, sizeof(e)-1, sizeof(f)-1, sizeof(g)-1, sizeof(h)-1, sizeof(i)-1, sizeof(j)-1, sizeof(k)-1, sizeof(l)-1, sizeof(m)-1, sizeof(n)-1, sizeof(o)-1, sizeof(p)-1}) \
}
// Macro magic to overload a macro by arity
#define GET_ARG_20(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,N,...) N
#define COUNT_MACRO_ARGS(...) GET_ARG_20( __VA_ARGS__, 19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,I_CANNOT_SEE_ZERO_ARGS)

#define ROW(...) SELECT_ROW(COUNT_MACRO_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define SELECT_ROW_IMPL(n) ROW##n
#define SELECT_ROW(n) SELECT_ROW_IMPL(n)

TestFunction(TestParsing){
    const char* input =
        // Supporting syntax like other spreadsheets.
        "=r(a1)\n"
        "=r(a$)\n"
        "=r(a1:b1)\n"
        "=r(a1:b)\n"
        "=r(a:b)\n"
        "=r(a1:a3)\n"
        "=r(a1:a1)\n"
        "=r(a$:a1)\n"
        "=r(a1:a$)\n"
        "=r(a$:b$)\n"

        "=r(a:a5)\n"
        "=r(a:5)\n"
        "=r(a5:a)\n"
        "=r(a5:)\n"
        "=r(:a5)\n"

        "=r(c:c)\n"
        "=r(c:)\n"
        "=r(c)\n"
        "=r(:c)\n"

        // allow spaces in identifiers
        "=r(a b1)\n"
        "=r(a b$)\n"
        "=r(a b1:b b1)\n"
        "=r(a b1:a b3)\n"
        "=r(a b1:a b1)\n"
        "=r(a b$:a b1)\n"
        "=r(a b1:a b$)\n"
        "=r(a b$:b b$)\n"

        "=r(a b:a b5)\n"
        "=r(a b:5)\n"
        "=r(a b5:a b)\n"
        "=r(a b5:)\n"
        "=r(:a b5)\n"

        "=r(c b:c b)\n"
        "=r(c b:)\n"
        "=r(c b)\n"
        "=r(:c b)\n"

        // [col, 1] -> 0d
        "=r([a,1])\n"
        "=r(['a', 1])\n"
        "=r([\"a\", 1])\n"
        "=r(['a', $])\n"
        "=r([\"$\", 3])\n"
        "=r(['$', 3])\n"

        // [col] -> 1d column
        "=r([a])\n"
        "=r(['a'])\n"
        "=r([\"a\"])\n"

        // [col, :3] -> 1d column
        "=r(['a', :2])\n"
        "=r([a, :2])\n"
        "=r([\"a\", :2])\n"

        // [col, 1:3] -> 1d column
        "=r([a, 1:2])\n"
        "=r(['a', 1:2])\n"
        "=r([\"a\", 1:2])\n"

        // [col, :] -> 1d column
        "=r(['a', :])\n"
        "=r([a, :])\n"
        "=r([\"a\", :])\n"

        // [col, 1:] -> 1d column
        "=r([a, 2:])\n"
        "=r(['a', 2:])\n"
        "=r([\"a\", 2:])\n"
        // [col:col, 1]   -> 1d row
        "=r([a:b, 3])\n"

        // [col:, 1]      -> 1d row
        "=r([a:, 3])\n"

        // [:col, 1]      -> 1d row
        "=r([:z, 3])\n"

        // [:, 1]         -> 1d row
        "=r([:, 3])\n"

        // Foreign versions
        // [f, col, 1] -> 0df
        "=r([f, a,1])\n"
        "=r([f, 'a', 1])\n"
        "=r([f, \"a\", 1])\n"
        "=r([f, 'a', $])\n"
        "=r([f, \"$\", 3])\n"
        "=r([f, '$', 3])\n"

        // [f, col] -> 1d column f
        "=r([f, a])\n"
        "=r([f, 'a'])\n"
        "=r([f, \"a\"])\n"

        // [f, col, :3] -> 1d column f
        "=r([f, 'a', :2])\n"
        "=r([f, a, :2])\n"
        "=r([f, \"a\", :2])\n"

        // [f, col, 1:3] -> 1d column f
        "=r([f, a, 1:2])\n"
        "=r([f, 'a', 1:2])\n"
        "=r([f, \"a\", 1:2])\n"

        // [f, col, :] -> 1d column f
        "=r([f, 'a', :])\n"
        "=r([f, a, :])\n"
        "=r([f, \"a\", :])\n"

        // [f, col, 1:] -> 1d column f
        "=r([f, a, 2:])\n"
        "=r([f, 'a', 2:])\n"
        "=r([f, \"a\", 2:])\n"
        // [f, col:col, 1]   -> 1d row f
        "=r([f, a:b, 3])\n"

        // [f, col:, 1]      -> 1d row f
        "=r([f, a:, 3])\n"

        // [f, :col, 1]      -> 1d row f
        "=r([f, :z, 3])\n"

        // [f, :, 1]         -> 1d row f
        "=r([f, :, 3])\n"


        "=1+2+\n" // parse error

    ;
    // NOTE: We print out the internal 0-based offsets instead
    //       of the user-facing 1-based offsets.
    SheetRow expected[] = {
        ROW("R0([a, 0])"),
        ROW("R0([a, $])"),
        ROW("R1R([a:b, 0])"),
        ROW("R1R([a:b, 0])"),
        ROW("R1R([a:b, 0])"),
        ROW("R1C([a, 0:2])"),
        ROW("R1C([a, 0:0])"),
        ROW("R1C([a, $:0])"),
        ROW("R1C([a, 0:$])"),
        ROW("R1R([a:b, $])"),

        ROW("R1C([a, 0:4])"),
        ROW("R1C([a, 0:4])"),
        ROW("R1C([a, 4:-1])"),
        ROW("R1C([a, 4:-1])"),
        ROW("R1C([a, 0:4])"),

        ROW("R1C([c, 0:-1])"),
        ROW("R1C([c, 0:-1])"),
        ROW("R1C([c, 0:-1])"),
        ROW("R1C([c, 0:-1])"),

        // spaces in identifiers
        ROW("R0([a b, 0])"),
        ROW("R0([a b, $])"),
        ROW("R1R([a b:b b, 0])"),
        ROW("R1C([a b, 0:2])"),
        ROW("R1C([a b, 0:0])"),
        ROW("R1C([a b, $:0])"),
        ROW("R1C([a b, 0:$])"),
        ROW("R1R([a b:b b, $])"),

        ROW("R1C([a b, 0:4])"),
        ROW("R1C([a b, 0:4])"),
        ROW("R1C([a b, 4:-1])"),
        ROW("R1C([a b, 4:-1])"),
        ROW("R1C([a b, 0:4])"),

        ROW("R1C([c b, 0:-1])"),
        ROW("R1C([c b, 0:-1])"),
        ROW("R1C([c b, 0:-1])"),
        ROW("R1C([c b, 0:-1])"),

        // [col, 1] -> 0d
        ROW("R0([a, 0])"),
        ROW("R0([a, 0])"),
        ROW("R0([a, 0])"),
        ROW("R0([a, $])"),
        ROW("R0([$, 2])"),
        ROW("R0([$, 2])"),

        // [col] -> 1d column
        ROW("R1C([a, 0:-1])"),
        ROW("R1C([a, 0:-1])"),
        ROW("R1C([a, 0:-1])"),

        // [col, :3] -> 1d column
        ROW("R1C([a, 0:1])"),
        ROW("R1C([a, 0:1])"),
        ROW("R1C([a, 0:1])"),

        // [col, 1:3] -> 1d column
        ROW("R1C([a, 0:1])"),
        ROW("R1C([a, 0:1])"),
        ROW("R1C([a, 0:1])"),

        // [col, :] -> 1d column
        ROW("R1C([a, 0:-1])"),
        ROW("R1C([a, 0:-1])"),
        ROW("R1C([a, 0:-1])"),

        // [col, 1:] -> 1d column
        ROW("R1C([a, 1:-1])"),
        ROW("R1C([a, 1:-1])"),
        ROW("R1C([a, 1:-1])"),

        // [col:col, 1]   -> 1d row
        ROW("R1R([a:b, 2])"),

        // [col:, 1]      -> 1d row
        ROW("R1R([a:, 2])"),

        // [:col, 1]      -> 1d row
        ROW("R1R([:z, 2])"),

        // [:, 1]         -> 1d row
        ROW("R1R([:, 2])"),

        // foreign versions
        // [f, col, 1] -> 0d f
        ROW("R0F([f, a, 0])"),
        ROW("R0F([f, a, 0])"),
        ROW("R0F([f, a, 0])"),
        ROW("R0F([f, a, -2147483647])"),
        ROW("R0F([f, $, 2])"),
        ROW("R0F([f, $, 2])"),

        // [f, col] -> 1d column f
        ROW("R1CF([f, a, 0:-1])"),
        ROW("R1CF([f, a, 0:-1])"),
        ROW("R1CF([f, a, 0:-1])"),

        // [f, col, :3] -> 1d column f
        ROW("R1CF([f, a, 0:1])"),
        ROW("R1CF([f, a, 0:1])"),
        ROW("R1CF([f, a, 0:1])"),

        // [f, col, 1:3] -> 1d column f
        ROW("R1CF([f, a, 0:1])"),
        ROW("R1CF([f, a, 0:1])"),
        ROW("R1CF([f, a, 0:1])"),

        // [f, col, :] -> 1d column f
        ROW("R1CF([f, a, 0:-1])"),
        ROW("R1CF([f, a, 0:-1])"),
        ROW("R1CF([f, a, 0:-1])"),

        // [f, col, 1:] -> 1d column f
        ROW("R1CF([f, a, 1:-1])"),
        ROW("R1CF([f, a, 1:-1])"),
        ROW("R1CF([f, a, 1:-1])"),

        // [f, col:col, 1]   -> 1d row f
        ROW("R1RF([f, a:b, 2])"),

        // [f, col:, 1]      -> 1d row f
        ROW("R1RF([f, a:, 2])"),

        // [f, :col, 1]      -> 1d row f
        ROW("R1RF([f, :z, 2])"),

        // [f, :, 1]         -> 1d row f
        ROW("R1RF([f, :, 2])"),

        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 1);
}

TestFunction(TestRanges){
    const char* input =
        " 0 | =[a,1] | = a1\n"
        " 1 | =['a', 1]\n"
        " 2 | =['a', $]\n"
        " 3 | =[\"$\", 3]\n"
        " 4 | =['$', 3]\n"
        " 5 | =sum([a]) | =sum(a) | =sum([a:, 1])\n"
        " 6 | =sum(['a', :])\n"
        " 7 | =sum(['a', 1:2]) | =sum(a1:a2)\n"
        " 8 | =sum(['a', :2])\n"
        " 9 | =sum(['a', 2:])\n"
        "10 | =sum([a, 2:])\n"
    ;
    SheetRow expected[] = {
        [ 0] = ROW("0", "0", "0"),
        [ 1] = ROW("1", "0"),
        [ 2] = ROW("2", "2"),
        [ 3] = ROW("3", "2"),
        [ 4] = ROW("4", "2"),
        [ 5] = ROW("5", "55", "55", "0"),
        [ 6] = ROW("6", "55"),
        [ 7] = ROW("7", "1", "1"),
        [ 8] = ROW("8", "1"),
        [ 9] = ROW("9", "55"),
        [10] = ROW("10", "55"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestBadRanges){
    const char* input =
        // "=a b 1\n"
        // "=[a,]\n"
        "=[,a,]\n"
        "=[:]\n"
        "=[:, :]\n"
        "=[,:, :]\n"
        "=[:,:, 2]\n"
        "=[1]\n"
        "=[$]\n"
        "=[]\n"
        "=['$', '3', '3']\n"
        "=sum(['a])\n"
        "=sum(['a', '3':'3'])\n"
        "=sum([3, 1:2])\n"

        // "=[f, a,]\n"
        "=[f, ,a,]\n"
        "=[f, ,:, :]\n"
        "=[f, :,:, 2]\n"
        // "=[f, ]\n"
        "=[f, '$', '3', '3']\n"
        "=sum([f, 'a])\n"
        "=sum([f, 'a', '3':'3'])\n"
        "=sum([f,3, 1:2])\n"
    ;
    SheetRow expected[] = {
        // ROW("error"),
        // ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),

        // ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        // ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), arrlen(expected));
}

TestFunction(TestSpreadsheet1){
    const char* input =
        "=sum(c)                   | Axe         | 10\n"
        "=find('Food', b)          | Torch       | 1\n"
        "=tlu('Plate Armor', b, c) | Plate Armor | 50\n"
        "=min(c:2)                 | Food        | 1 per potato\n"
    ;
    SheetRow expected[] = {
        ROW("61", "Axe",         "10"),
        ROW( "4", "Torch",        "1"),
        ROW("50", "Plate Armor", "50"),
        ROW( "1", "Food",        "1 per potato"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestSpreadsheet2){
    const char* input =
        // a       |     b       |    c             |  d
        "=b1       | 2           | 3                | =d3\n"
        "=a1+b1    | = a2+2      | =a2              | 4\n"
        "2         | =-(a1 > -1) | 3                | = sum(b3:4) * sum(a4:1)\n"
        "=a3*b1    | 4           | =1-2---3*4/2-8-1 | = sum(a:11)\n"
        "=sum(a:4) | = sum(d)    | = sum(b)\n"
    ;
    SheetRow expected[] = {
        ROW("2", "2", "3", "36"),
        ROW("4", "6", "4",  "4"),
        ROW("2", "-1", "3", "36"),
        ROW("4", "4",  "-16", "24"),
        ROW("12", "100", "111"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestBinOps){
    const char* input =
        "1 | 2 | 3 | 4 \n"
        "a | b | c \n"
        "='1'\n"

        "=1+1   | =1-1    | =4/2       | =3*4 \n"
        "=2 = 1 | =2 == 1 | =2 != 1\n"
        "=1 < 2 | =1 > 2  | =1 >= 1    | =1 <= 2\n"

        "='abc' != 'abc' | ='abc' = ''  | = 'abc' = 'abc' \n"
        "= '1' = 2 | = '1' < 2 | = '1' < '2'\n"

        "=f(a('1')  = '1')| = f('1'  = a('1'))| = f(a('1')  = a('1'))\n"
        "=f(a('1') != '1')| = f('1' != a('1'))| = f(a('1') != a('1'))\n"

        "=f(a(1)+1)   | =f(a(1)-1)    | =f(a(4)/2)       | =f(a(3)*4) \n"
        "=f(a(2) = 1) | =f(a(2) == 1) | =f(a(2) != 1)\n"
        "=f(a(1) < 2) | =f(a(1) > 2)  | =f(a(1) >= 1)    | =f(a(1) <= 2)\n"

        "=f(1+a(1))   | =f(1-a(1))    | =f(4/a(2))       | =f(3*a(4)) \n"
        "=f(2 = a(1)) | =f(2 == a(1)) | =f(2 != a(1))\n"
        "=f(1 < a(2)) | =f(1 > a(2))  | =f(1 >= a(1))    | =f(1 <= a(2))\n"

        "=f(a(1)+a(1))   | =f(a(1)-a(1))    | =f(a(4)/a(2))       | =f(a(3)*a(4)) \n"
        "=f(a(2) = a(1)) | =f(a(2) == a(1)) | =f(a(2) != a(1))\n"
        "=f(a(1) < a(2)) | =f(a(1) > a(2))  | =f(a(1) >= a(1))    | =f(a(1) <= a(2))\n"

        // using range references instead of computed arrays
        "=f(a3:a3  = '1')| = f('1'  = a3:a3)| = f(a3:a3  = a3:a3)\n"
        "=f(a3:a3 != '1')| = f('1' != a3:a3)| = f(a3:a3 != a3:a3)\n"

        "=f(a1:a1+1)   | =f(a1:a1-1)    | =f(d1:d1/2)       | =f(c1:c1*4) \n"
        "=f(b1:b1 = 1) | =f(b1:b1 == 1) | =f(b1:b1 != 1)\n"
        "=f(a1:a1 < 2) | =f(a1:a1 > 2)  | =f(a1:a1 >= 1)    | =f(a1:a1 <= 2)\n"

        "=f(1+a1:a1)   | =f(1-a1:a1)    | =f(4/b1:b1)       | =f(3*d1:d1) \n"
        "=f(2 = a1:a1) | =f(2 == a1:a1) | =f(2 != a1:a1)\n"
        "=f(1 < b1:b1) | =f(1 > b1:b1)  | =f(1 >= a1:a1)    | =f(1 <= b1:b1)\n"

        "=f(a1:a1+a1:a1)   | =f(a1:a1-a1:a1)    | =f(d1:d1/b1:b1)       | =f(c1:c1*d1:d1) \n"
        "=f(b1:b1 = a1:a1) | =f(b1:b1 == a1:a1) | =f(b1:b1 != a1:a1)\n"
        "=f(a1:a1 < b1:b1) | =f(a1:a1 > b1:b1)  | =f(a1:a1 >= a1:a1)    | =f(a1:a1 <= b1:b1)\n"

        // This exercises the swapping code path
        "=f(a1:a1 + a(1)) | =f(a1:a1 - a(1))  | =f(a1:a1 * a(1)) | =f(a1:a1 / a(1))\n"
        "=f(a1:a1 < a(1)) | =f(a1:a1 <= a(1)) | =f(a1:a1 > a(1)) | =f(a1:a1 >= a(1))\n"
        "=f(a1:a1 = a(1)) | =f(a1:a1 != a(1))\n"

        // Arrays of different lengths
        "=f(a1:a1 = a(1, 2)) | =f(a1:a2 = a(1))\n"

    ;
    SheetRow expected[] = {
        ROW("1", "2", "3", "4"),
        ROW("a", "b", "c"),
        ROW("1"),

        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),

        ROW("0", "", "1"),
        ROW("error", "error", "error"),

        ROW("1", "1", "1"),
        ROW("0", "0", "0"),

        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),

        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),

        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),

        ROW("1", "1", "1"),
        ROW("0", "0", "0"),

        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),

        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),

        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),

        ROW("2", "0", "1", "1"),
        ROW("0", "1", "0", "1"),
        ROW("1", "0"),

        ROW("error", "error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 5);
}
TestFunction(TestUnOps){
    const char* input =
        "=!1      | =-2    | =+2\n"
        // Unary minus gets handled in the parser for number
        // literals, so we need an expression.
        "=-!!(1+1-1)| =--(1+1)   | =-+(1+1)\n"
        "=!!1     | =-!1   | =+!2\n"
        "=!!!1    | =-!!1  | =+!!2\n"
        "=-!-!-!1 | =-!+!1 | =+!-!2\n"
        "=-!-!-! | =-!+! | =+!-!\n"
    ;
    SheetRow expected[] = {
        ROW("0", "-2", "2"),
        ROW("-1", "2", "-2"),
        ROW("1",  "0", "0"),
        ROW("0", "-1", "1"),
        ROW("0", "-1", "1"),
        ROW("error", "error", "error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 3);
}

TestFunction(TestFuncs){
    const char* input =
        "=sum(b)          | -1.5\n"
        "=avg(b)          | 3.5\n"
        "=min(b)          | 44      | hello\n"
        "=max(b)          | 14\n"
        "=mod(b4)        | \n"
        "=abs(b1)        | \n"
        "=floor(b1)      | \n"
        "=ceil(b1)       | \n"
        "=trunc(b1)      | \n"
        "=round(b1)      | \n"
        "=tlu(44, b, c) | \n"
        "=find(44, b) | \n"
        "=count(b)\n"
        "\n"
        "=ceil(-12.1)\n"
        "=floor(-12.1)\n"
        "=round(-12.1)\n"
        "=trunc(-12.1)\n"
        "\n"
        "=ceil(-12.9)\n"
        "=floor(-12.9)\n"
        "=round(-12.9)\n"
        "=trunc(-12.9)\n"
        "\n"
        "=ceil(12.1)\n"
        "=floor(12.1)\n"
        "=round(12.1)\n"
        "=trunc(12.1)\n"
        "\n"
        "=ceil(12.9)\n"
        "=floor(12.9)\n"
        "=round(12.9)\n"
        "=trunc(12.9)\n"
        "\n"
        "=num(1, 2)\n"
        "=num('', 2)\n"
        "=num('')\n"
        "=num('3 people')\n"
        "=num('4 hippos', -1)\n"
        "=num('lmao', 7)\n"
        "=num('3.1lbs', 7)\n"
        "=num('.2 carrots', 7)\n"
        "=num('+1', 7)\n"
        "=num('-.2', 7)\n"

        "=try(ceil('a'), 'b')\n"
        "=try(ceil(1), 'b')\n"
        "=pow(2, 3)\n"
        "=eval('pow(2,4)')\n"
        "=call('pow', 3, 5)\n"
        "=sqrt(9)\n"
        "=log(100)/log(10)\n"
        // log10, log2 don't parse atm
        // "=log10(100)\n"
        // "=log2(8)\n"
        "=prod(b)\n"

        "=if(1, 2, 3)\n"
        "=if('', 2, 3)\n"
        "=if('a', 2, 3)\n"

        "=max(1, 2)\n"
        "=max(1, 2, 3)\n"
        "=max(1, 2, 3, 4)\n"

        "=min(-1, -2)\n"
        "=min(-1, -2, -3)\n"
        "=min(-1, -2, -3, -4)\n"

        "=cat('a', 'b')\n"
        "=cat('1234567890', 'abcdefghijklmnopqrstuvwxyz')\n"
        "=cat('12345678901234567890', 'abcdefghijklmnopqrstuvwxyz')\n"
        "=cat('a', cat('b', cat('c', cat('d', 'e'))))\n"
        "=cat('a', 'b', 'c')\n"
        "=cat('a', 'b', 'c', 'd')\n"
        "=cat('a', '')\n"
        "=cat('', 'b')\n"
        "=cat('', '')\n"
        "=CAT('', '')\n"
        "=CaT('', '')\n"

        "=MiN(mAX(-1, -2), 1)\n"
        "=mean(b)\n"

        // nonexistent
        "=h()\n"
        "=ha()\n"
        "=hah()\n"
        "=haha()\n"
        "=hahah()\n"
        "=hahaha()\n"

    ;
    SheetRow expected[] = {
        ROW("60", "-1.5"),
        ROW("15", "3.5"),
        ROW("-1.5", "44", "hello"),
        ROW("44", "14"),
        ROW("2", ""),
        ROW("1.5", ""),
        ROW("-2", ""),
        ROW("-1", ""),
        ROW("-1", ""),
        ROW("-2", ""),
        ROW("hello", ""),
        ROW("3", ""),
        ROW("4"),

        ROW(""),
        ROW("-12"),
        ROW("-13"),
        ROW("-12"),
        ROW("-12"),
        ROW(""),
        ROW("-12"),
        ROW("-13"),
        ROW("-13"),
        ROW("-12"),
        ROW(""),
        ROW("13"),
        ROW("12"),
        ROW("12"),
        ROW("12"),
        ROW(""),
        ROW("13"),
        ROW("12"),
        ROW("13"),
        ROW("12"),
        ROW(""),
        ROW("1"),
        ROW("2"),
        ROW("0"),
        ROW("3"),
        ROW("4"),
        ROW("7"),
        ROW("3.1"),
        ROW("0.2"),
        ROW("1"),
        ROW("-0.2"),

        ROW("b"),
        ROW("1"),
        ROW("8"),
        ROW("16"),
        ROW("243"),
        ROW("3"),
        ROW("2"),
        // log10, log2 don't parse atm
        // ROW("2"),
        // ROW("3"),
        ROW("-3234"),

        ROW("2"),
        ROW("3"),
        ROW("2"),

        ROW("2"),
        ROW("3"),
        ROW("4"),

        ROW("-2"),
        ROW("-3"),
        ROW("-4"),

        ROW("ab"),
        ROW("1234567890abcdefghijklmnopqrstuvwxyz"),
        ROW("12345678901234567890abcdefghijklmnopqrstuvwxyz"),
        ROW("abcde"),
        ROW("abc"),
        ROW("abcd"),
        ROW("a"),
        ROW("b"),
        ROW(""),
        ROW(""),
        ROW(""),

        ROW("-1"),
        ROW("15"),

        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 6);
}
TestFunction(TestFuncsV){
    const char* input =
        "0\n"
        "1\n"
        "2\n"
        "3\n"
        "4\n"
        "5\n"

        "=f(a(1))\n"
        "=f(mod(a(13)))\n"

        "=f(abs(a(-13)))\n"
        "=f(sqrt(a(9)))\n"
        "=f(log(a(100)))/log(10)\n"

        "=f(trunc(a(13.1)))\n"
        "=f(floor(a(13.1)))\n"
        "=f(ceil( a(13.1)))\n"
        "=f(round(a(13.1)))\n"

        "=f(trunc(a(-13.1)))\n"
        "=f(floor(a(-13.1)))\n"
        "=f(ceil( a(-13.1)))\n"
        "=f(round(a(-13.1)))\n"

        "=f(trunc(a(13.5)))\n"
        "=f(floor(a(13.5)))\n"
        "=f(ceil( a(13.5)))\n"
        "=f(round(a(13.5)))\n"

        "=f(trunc(a(-13.5)))\n"
        "=f(floor(a(-13.5)))\n"
        "=f(ceil( a(-13.5)))\n"
        "=f(round(a(-13.5)))\n"

        "=f(pow(a(2), 3))\n"
        "=f(pow(a(3), a(4)))\n"

        "=prod(a(1, 2, 3, 4))\n"
        "=sum(a(1, 2, 3, 4))\n"

        "=sum(if(a(0,1,0), 3, 4))\n"
        "=sum(if(a(0,1,0), a(10, 11, 12), 4))\n"
        "=sum(if(a(0,1,0), 2, a(41, 42, 43)))\n"
        "=sum(if(a(0,1,0), a(31, 32, 33), a(15, 25, 35)))\n"

        "=sum(num(a('', 1), 2))\n"

        "=f(cat(a('a'), a('b')))\n"
        "=f(cat('a', a('b')))\n"
        "=f(cat(a('a'), 'b'))\n"
        "=f(cat(a('a'), ''))\n"
        "=f(cat(a(''), 'b'))\n"
        "=f(cat(a(''), a('b')))\n"
        "=f(cat(a('a'), a('')))\n"
        "=f(cat('a', a('b'), 'c'))\n"
        "=f(cat('a', a('b'), 'c', a('d')))\n"
        "=f(cat('a', a(''), 'c', a('')))\n"
        "=f(cat('a', '', 'c', a('')))\n"
        "=cat('a', '', 'c', '')\n"

        "=f(eval(a('pow(2,4)')))\n"
        "=sum(eval(a('pow(2,4)', 'pow(2, 3)', '')))\n"

        "=count(a(1, 0, 'a', ''))\n"
        "=avg(a(2, 4, 6))\n"
        "=min(a(2, 4, 6))\n"
        "=max(a(2, 4, 6))\n"

        "=find(2, a(2, 4, 6))\n"
        "=find(1, a(2, 4, 6), 3)\n"
        "=find('', a('2', 4, ''))\n"
        "=find('2', a('2', 4, ''))\n"
        "=tlu(4, a(2, '4', 4, 6), a(7, 8, 9, 10))\n"
        "=tlu('4', a(4, '4', '6'), a('a', 'b', 'c'))\n"
        "=tlu('a', [b:b, $], [c:c, $]) | a | 3\n"
        "=tlu(3, [b:b, $], [c:c, $]) | 3 | a\n"
        "=tlu(2, a(1), a(1), 9)\n"
        // NOTE: last arg is a scalar and is for every missing value
        "=f(tlu(a(2), a('', 1), a('', 7), 9))\n"
        "=f(tlu(a(2), a('', 2), a('', 7), 9))\n"
        "=f(tlu(a(''), a('', 2), a('', 7), 9))\n"
        "=f(tlu(a(3), a(1,3), a(7)))\n"


        "=f(if(a1:a2, 1, 2)) | =f(if(a1:a2, 1, a3:a4)) | =f(if(a2:a3, a5:a6, 3))\n"
    ;
    SheetRow expected[] = {
        ROW("0"),
        ROW("1"),
        ROW("2"),
        ROW("3"),
        ROW("4"),
        ROW("5"),

        ROW("1"),
        ROW("1"),

        ROW("13"),
        ROW("3"),
        ROW("2"),

        ROW("13"),
        ROW("13"),
        ROW("14"),
        ROW("13"),

        ROW("-13"),
        ROW("-14"),
        ROW("-13"),
        ROW("-13"),

        ROW("13"),
        ROW("13"),
        ROW("14"),
        ROW("14"),

        ROW("-13"),
        ROW("-14"),
        ROW("-13"),
        ROW("-14"),

        ROW("8"),
        ROW("81"),

        ROW("24"),
        ROW("10"),

        ROW("11"),
        ROW("19"),
        ROW("86"),
        ROW("82"),

        ROW("3"),

        ROW("ab"),
        ROW("ab"),
        ROW("ab"),
        ROW("a"),
        ROW("b"),
        ROW("b"),
        ROW("a"),
        ROW("abc"),
        ROW("abcd"),
        ROW("ac"),
        ROW("ac"),
        ROW("ac"),

        ROW("16"),
        ROW("24"),

        ROW("3"), // skips the empty string, includes 0
        ROW("4"),
        ROW("2"),
        ROW("6"),

        ROW("1"),
        ROW("3"),
        ROW("3"),
        ROW("1"),
        ROW("9"),
        ROW("b"),
        ROW("3", "a", "3"),
        ROW("a", "3", "a"),
        ROW("9"),
        ROW("9"),
        ROW("7"),
        ROW(""),
        ROW("error"),

        ROW("2", "2", "4"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 1);
}
TestFunction(TestFuncsRowArray){
    const char* input =
    //   a     b    c       d       e      f      g   h    i    j    k    l    m      n   o   p
        "1  | 13 | 13.1 | -13.1 | 13.5 | -13.5 | 1  | 2  | 3  | 4  | 0  | 1  | 0\n"
        "10 | 11 | 12   | 41    | 42   | 43    | 31 | 32 | 33 | 15 | 25 | 35 | a    | b | c | d\n"
        "pow(2,4) | pow(2,3) |\n"
        "   | 1  | 0    | a     |      | 2     | 4   | 6\n"

        "=f(row('$', '$', 1)) | =f(row('$', 'a', 1)) | =f([a:a,1]) | = f(row('', '', 1))\n"
        "=f(col('$', 1, 1))   | =f([a,1:1]) | = f(col('', 1, 1))\n"
        "=f(mod(row('b', 'b', 1)))\n"

        "=f(trunc(row('c', 'c', 1)))\n"
        "=f(floor(row('c', 'c', 1)))\n"
        "=f(ceil( row('c', 'c', 1)))\n"
        "=f(round(row('c', 'c', 1)))\n"

        "=f(trunc(row('d', 'd', 1)))\n"
        "=f(floor(row('d', 'd', 1)))\n"
        "=f(ceil( row('d', 'd', 1)))\n"
        "=f(round(row('d', 'd', 1)))\n"

        "=f(trunc(row('e', 'e', 1)))\n"
        "=f(floor(row('e', 'e', 1)))\n"
        "=f(ceil( row('e', 'e', 1)))\n"
        "=f(round(row('e', 'e', 1)))\n"

        "=f(trunc(row('f', 'f', 1)))\n"
        "=f(floor(row('f', 'f', 1)))\n"
        "=f(ceil( row('f', 'f', 1)))\n"
        "=f(round(row('f', 'f', 1)))\n"

        "=f(pow(row('h', 'h', 1), 3))\n"
        "=f(pow(row('i','i', 1), row('j', 'j', 1)))\n"

        "=prod(row('g', 'j', 1))\n"
        "=sum( row('g', 'j', 1))\n"

        "=sum(if(row('k','m',1), 3, 4))\n"
        "=sum(if(row('k','m',1), row('a', 'c', 2), 4))\n"
        "=sum(if(row('k','m',1), 2, row('d', 'f', 2)))\n"
        "=sum(if(row('k','m',1), row('g', 'i', 2), row('j', 'l', 2)))\n"

        "=sum(num(row('a', 'b', 4), 2))\n"

        "=f(cat(row('m', 'm', 2), row('n', 'n', 2)))\n"
        "=f(cat('a', row('n', 'n', 2)))\n"
        "=f(cat(row('m', 'm', 2), 'b'))\n"
        "=f(cat('a', row('n', 'n', 2), 'c'))\n"
        "=f(cat('a', row('n', 'n', 2), 'c', row('p', 'p', 2)))\n"

        "=f(eval(row('a', 'a', 3)))\n"
        "=sum(eval(row('a', 'c', 3)))\n"

        "=count(row('b', 'e', 4))\n"
        "=avg(row('f', 'h', 4))\n"
        "=min(row('f', 'h', 4))\n"
        "=max(row('f', 'h', 4))\n"

        "=find(2, row('f', 'h', 4))\n"
        "=find('b', row('m', 'p', 2))\n"
        "=find('', row('m', 'z', 2))\n"
        "=find('', col('p', 2))\n"
        "=find('', column('p', 2))\n"

        "=f(row('b', 'c', 4)+row('f', 'g', 4))\n"
        "=sum(row('b', 'c', 4)+row('f', 'g', 4))\n"
    ;
    SheetRow expected[] = {
        // Designated initializers are so you can figure out which row
        // when a test fails.
        [ 0] = ROW("1", "13", "13.1", "-13.1", "13.5", "-13.5", "1", "2", "3", "4", "0", "1", "0"),
        [ 1] = ROW("10", "11", "12", "41", "42", "43", "31", "32", "33", "15", "25", "35", "a", "b", "c", "d"),
        [ 2] = ROW("pow(2,4)", "pow(2,3)", ""),
        [ 3] = ROW("", "1", "0", "a", "", "2", "4", "6"),

        [ 4] = ROW("1", "1", "1", "1"),
        [ 5] = ROW("1", "1", "1"),
        [ 6] = ROW("1"),

        [ 7] = ROW("13"),
        [ 8] = ROW("13"),
        [ 9] = ROW("14"),
        [10] = ROW("13"),

        [11] = ROW("-13"),
        [12] = ROW("-14"),
        [13] = ROW("-13"),
        [14] = ROW("-13"),

        [15] = ROW("13"),
        [16] = ROW("13"),
        [17] = ROW("14"),
        [18] = ROW("14"),

        [19] = ROW("-13"),
        [20] = ROW("-14"),
        [21] = ROW("-13"),
        [22] = ROW("-14"),

        [23] = ROW("8"),
        [24] = ROW("81"),

        [25] = ROW("24"),
        [26] = ROW("10"),

        [27] = ROW("11"),
        [28] = ROW("19"),
        [29] = ROW("86"),
        [30] = ROW("82"),

        [31] = ROW("3"),

        [32] = ROW("ab"),
        [33] = ROW("ab"),
        [34] = ROW("ab"),
        [35] = ROW("abc"),
        [36] = ROW("abcd"),

        [37] = ROW("16"),
        [38] = ROW("24"),

        [39] = ROW("3"), // skips the empty string, includes 0
        [40] = ROW("4"),
        [41] = ROW("2"),
        [42] = ROW("6"),

        [43] = ROW("1"),
        [44] = ROW("2"),
        [45] = ROW("5"),
        [46] = ROW("2"),
        [47] = ROW("2"),

        [48] = ROW("3"),
        [49] = ROW("7"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestMod){
    const char* input =
        "=mod(3)\n"
        "=mod(4)\n"
        "=mod(5)\n"
        "=mod(6)\n"
        "=mod(7)\n"
        "=mod(8)\n"
        "=mod(9)\n"
        "=mod(10)\n"
        "=mod(11)\n"
        "=mod(12)\n"
        "=mod(13)\n"
        "=mod(14)\n"
        "=mod(15)\n"
        "=mod(16)\n"
        "=mod(17)\n"
        "=mod(18)\n"
        "=mod(19)\n"
        "=mod(20)\n"
    ;
    SheetRow expected[] = {
        ROW("-4"),
        ROW("-3"),
        ROW("-3"),
        ROW("-2"),
        ROW("-2"),
        ROW("-1"),
        ROW("-1"),
        ROW("0"),
        ROW("0"),
        ROW("1"),
        ROW("1"),
        ROW("2"),
        ROW("2"),
        ROW("3"),
        ROW("3"),
        ROW("4"),
        ROW("4"),
        ROW("5"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestBugs){
    const char* input =
        "Plate   | 5 | Plate | =tlu(c1, a, b)\n"
        "Chain   | 3 |       | =tlu(c2, a, b, 2)\n"
        "Leather | 1 | =     | =tlu(c3, a, b, 4)\n"
    ;
    SheetRow expected[] = {
        ROW("Plate",   "5", "Plate", "5"),
        ROW("Chain" ,  "3", "",      "2"),
        ROW("Leather", "1", "error", "error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 2);
}
TestFunction(TestBugs2){
    const char* input =
        "Plate   | 5 | Plate | =tlu(c$, a, b)\n"
        "Chain   | 3 |       | =tlu(c$, a, b, 2)\n"
        "Leather | 1 | =     | =tlu(c$, a, b, 4)\n"
    ;
    SheetRow expected[] = {
        ROW("Plate",   "5", "Plate", "5"),
        ROW("Chain" ,  "3", "",      "2"),
        ROW("Leather", "1", "error", "error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 2);
}
TestFunction(TestDirectlyRecursiveShouldError){
    const char* input =
        "=a$\n"
    ;
    SheetRow expected[] = {
        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 1);
}
TestFunction(TestIndirectlyRecursiveShouldError){
    const char* input =
        "=a2\n"
        "=a1\n"
    ;
    SheetRow expected[] = {
        ROW("error"),
        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 2);
}

static
struct
TestStats
test_multi_spreadsheet(const char* caller, const char* name1, const char* input1, const char* name2, const char* input2, const SheetRow* expected, size_t expected_len, int expected_nerr){
#define __func__ caller
    struct TestStats TEST_stats = {0};
    MultiSpreadSheet collection = {0};
    int err;
    err = read_csv_from_string(multisheet_alloc(&collection), input1);
    TestAssertFalse(err);
    collection.sheets[0].name = (StringView){strlen(name1), name1};
    err = read_csv_from_string(multisheet_alloc(&collection), input2);
    TestAssertFalse(err);
    collection.sheets[1].name = (StringView){strlen(name2), name2};
    TestAssertEquals(collection.sheets[0].rows, expected_len);

    SheetOps ops = multisheet_ops(&collection);
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    SheetHandle handles[] = {
        (SheetHandle)&collection.sheets[0],
        (SheetHandle)&collection.sheets[1],
    };
    int e;
    e = drsp_set_sheet_name(ctx, handles[0], name1, strlen(name1));
    TestAssertFalse(e);
    e = drsp_set_sheet_name(ctx, handles[1], name2, strlen(name2));
    TestAssertFalse(e);
    for(int i = 0; i < 2; i++){
        SpreadSheet* sheet = &collection.sheets[i];
        SheetHandle sheethandle = (SheetHandle)sheet;
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
                if(!row->lengths[c]) continue;
                if(e){ // don't bloat the stats
                    TestAssertFalse(e);
                }
            }
        }
    }
    int nerr = drsp_evaluate_formulas(ctx);
    TestExpectEquals(nerr, expected_nerr);
    for(size_t i = 0; i < expected_len; i++){
        const SheetRow* display_row = &collection.sheets[0].display[i];
        const SheetRow* expected_row = &expected[i];
        TestAssertEquals(display_row->n, expected_row->n);
        for(int j = 0; j < display_row->n; j++){
            TEST_stats.executed++;
            const char* lhs = display_row->data[j];
            const char* rhs = expected_row->data[j];
            if(!streq(lhs, rhs)){
                TEST_stats.failures++;
                  TestReport("Test condition failed");
                  TestReport("row %zu, col %d", i, j);
                  TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                          _test_color_green, lhs, _test_color_reset,
                          _test_color_red, _test_color_reset,
                          _test_color_green, rhs, _test_color_reset);
            }
        }
    }
    drsp_destroy_ctx(ctx);
    ctx = drsp_create_ctx(&ops);
    e = drsp_set_sheet_name(ctx, handles[0], name1, strlen(name1));
    TestAssertFalse(e);
    e = drsp_set_sheet_name(ctx, handles[1], name2, strlen(name2));
    TestAssertFalse(e);
    for(int i = 0; i < 2; i++){
        SpreadSheet* sheet = &collection.sheets[i];
        SheetHandle sheethandle = (SheetHandle)sheet;
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
                if(!row->lengths[c]) continue;
                if(e){ // don't bloat the stats
                    TestAssertFalse(e);
                }
            }
        }
    }
    nerr = 0;
    for(int i = 0; i < collection.sheets[0].rows; i++){
        const SheetRow* expected_row = &expected[i];
        const SheetRow* inp_row = &collection.sheets[0].cells[i];
        TestAssertEquals(inp_row->n, expected_row->n);
        for(int col = 0; col < expected_row->n; col++){
            const char* expected_text = expected_row->data[col];
            StringView exp = {strlen(expected_text), expected_text};
            const char* inp_txt = inp_row->data[col];
            size_t inp_len = inp_row->lengths[col];
            StringView inp = stripped2(inp_txt, inp_len);
            if(!inp.length) continue;
            if(inp.text[0] != '=') continue;
            DrSpreadResult val;
            int err = drsp_evaluate_string(ctx, handles[0], inp_txt, inp_len, &val, i, col);
            nerr += err;
            if(!err){
                switch(val.kind){
                    case DRSP_RESULT_NULL:
                        TestExpectEquals(exp.length, 0);
                        break;
                    case DRSP_RESULT_NUMBER:{
                        DoubleResult dr = parse_double(exp.text, exp.length);
                        TestAssertFalse(dr.errored);
                        double n = dr.result;
                        TestExpectEquals(n, val.d);
                    }break;
                    case DRSP_RESULT_STRING:{
                        StringView v = {val.s.length, val.s.text};
                        TestExpectEquals2(sv_equals, v, exp);
                    } break;
                    default:
                        TestExpectFalse(1);
                        break;
                }
            }
        }
    }
    drsp_destroy_ctx(ctx);
    TestExpectEquals(nerr, expected_nerr);
    cleanup_multisheet(&collection);
    EXPECT_NO_LEAKS();

#undef __func__
    return TEST_stats;
}
TestFunction(TestBugs3){
    TESTBEGIN();
    const char* input =
        "a\n"
        "b\n"
        "=n=n00:n\n"
    ;
    MultiSpreadSheet ms = {0};
    {
        int err = read_multi_csv_from_string(&ms, input);
        TestAssertFalse(err);
        TestAssertEquals(ms.n, 1);
        TestAssertEquals(ms.sheets[0].rows, 1);
        TestAssertEquals(ms.sheets[0].cells[0].n, 1);
    }
    SheetOps ops = multisheet_ops(&ms);
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    for(int i = 0; i < ms.n; i++){
        SpreadSheet* sheet = &ms.sheets[i];
        int e = drsp_set_sheet_name(ctx, (SheetHandle)sheet, sheet->name.text, sheet->name.length);
        TestAssertFalse(e);
        for(int i = 0; i < sheet->colnames.n; i++){
            int e = drsp_set_col_name(ctx, (SheetHandle)sheet, i, sheet->colnames.data[i], sheet->colnames.lengths[i]);
            TestAssertFalse(e);
        }
        SheetHandle sheethandle = (SheetHandle)sheet;
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
                if(!row->lengths[c]) continue;
                if(e){ // don't bloat the stats
                    TestAssertFalse(e);
                }
            }
        }
    }
    int err = drsp_evaluate_formulas(ctx);
    TestAssertFalse(err);
    const SheetRow* disp = &ms.sheets[0].display[0];
    TestAssertEquals(disp->n, 1);
    StringView s = {disp->lengths[0], disp->data[0]};
    TestExpectEquals2(sv_equals, s, SV("[[array]]"));
    drsp_destroy_ctx(ctx);
    cleanup_multisheet(&ms);
    EXPECT_NO_LEAKS();
    TESTEND();
}
TestFunction(TestColFunc){
    const char* name1 = "root";
    const char* input1 =
        "3 | = sum(col('other', 'a'))\n"
        "4 | = sum(col('other', 'a', 2))\n"
        "  | = sum(col('other', 'a', 1, 1))\n"
        "  | = sum(col('a'))\n"
        "  | = sum(col('a', 2))\n"
        "  | = sum(col('a', 1, 1))\n"
    ;
    const char* name2 = "other";
    const char* input2 =
        " 1\n"
        " 2\n"
    ;
    SheetRow expected[] = {
        ROW("3", "3"),
        ROW("4", "2"),
        ROW("", "1"),
        ROW("", "7"),
        ROW("", "4"),
        ROW("", "3"),
    };
    return test_multi_spreadsheet(
        __func__,
        name1, input1,
        name2, input2,
        expected, arrlen(expected),
        0
    );
}
TestFunction(TestMultisheet){
    const char* name1 = "root";
    const char* input1 =
        "=cell('other', 'a', 1)-1\n"
        "=cell(a8, 'a', 2)\n"
        "=cell(a7, 2)\n"
        "=cell('other', 'b', 2)\n"
        "=cell('other', 'b', find(0, a:3))\n"
        "=cell('other', 'b', find('0', a:3))\n"
        "a\n"
        "other | =[other, a, 1] | =sum([other, a])\n"
    ;
    const char* name2 = "other";
    const char* input2 =
        " 1 | 2\n"
        " 3 | 4\n"
    ;
    SheetRow expected[] = {
        [0] = ROW("0"),
        [1] = ROW("3"),
        [2] = ROW("3"),
        [3] = ROW("4"),
        [4] = ROW("2"),
        [5] = ROW("error"),
        [6] = ROW("a"),
        [7] = ROW("other","1", "4"),
    };
    return test_multi_spreadsheet(
        __func__,
        name1, input1,
        name2, input2,
        expected, arrlen(expected),
        1
    );
}
TestFunction(TestNames){
    const char* name1 = "root";
    const char* input1 =
        "=[foo/bar, b, 1]\n"
    ;
    const char* name2 = "foo/bar";
    const char* input2 =
        " 1 | 2\n"
        " 3 | 4\n"
    ;
    SheetRow expected[] = {
        ROW("2"),
    };
    return test_multi_spreadsheet(
        __func__,
        name1, input1,
        name2, input2,
        expected, arrlen(expected),
        0
    );
}

TestFunction(TestComplexMultisheet){
    TESTBEGIN();
    #define FORMULA "sum(tlu(col(cat([Character, $], '/Inventory'), 'Item'), [Items, Item], [Items, Weight])*num(col(cat([Character, $], '/Inventory'), 'Quant'), 1))"
    const char* input =

        "Summary\n"
        // -------
        "Character | Encumbrance | Encumbrance2\n"
        "Gandalf   | =" FORMULA "  | =eval(tlu('Encumbrance', [Formula, Name], [Formula, Formula]))\n"
        "Frodo     | =" FORMULA "  | =eval(tlu('Encumbrance', [Formula, Name], [Formula, Formula]))\n"
        "Strider   | =" FORMULA "  | =eval(tlu('Encumbrance', [Formula, Name], [Formula, Formula]))\n"
        "---\n"

        "Items\n"
        // -------
        "Item    | Weight\n"
        "Axe     | 1\n"
        "Hammer  | 2\n"
        "Maul    | 4\n"
        "Plate   | 8\n"
        "Chain   | 6\n"
        "Mithril | 1\n"
        "Leather | 3\n"
        "Sword   | 1\n"
        "Torch   | 0.1\n"
        "Staff   | 1\n"
        "Shield  | 1\n"
        "---\n"

        "Gandalf/Inventory\n"
        // -------
        "Item  | Quant\n"
        "Axe\n"
        "Staff\n"
        "---\n"

        "Frodo/Inventory\n"
        // -------
        "Item  | Quant\n"
        "Sword\n"
        "Torch | 12\n"
        "Mithril\n"
        "---\n"

        "Strider/Inventory\n"
        // -------
        "Item  | Quant\n"
        "Sword\n"
        "Shield\n"
        "Leather\n"
        "Torch | 20\n"
        "Axe\n"
        "---\n"

        "Formula\n"
        // -------
        "Name        | Formula\n"
        "Encumbrance | " FORMULA "\n"
        ;
    #undef FORMULA
    MultiSpreadSheet ms = {0};
    {
        int err = read_multi_csv_from_string(&ms, input);
        TestAssertFalse(err);
    }
    SheetOps ops = multisheet_ops(&ms);
    struct test_case {
        StringView sv;
        double value;
    } cases [] = {
        { SV("tlu('Gandalf', [Character], [Encumbrance])"),   2. },
        { SV("tlu('Frodo',   [Character], [Encumbrance])"),   3.2},
        { SV("tlu('Strider', [Character], [Encumbrance])"),   8. },
        { SV("sum([Encumbrance])"),                          13.2},
        { SV("tlu('Gandalf', [Character], [Encumbrance2])"),  2. },
        // tlu is not case insensitive, but the ranges are
        { SV("tlu('Gandalf', [character], [encumbrance2])"),  2. },
        { SV("tlu('Frodo',   [Character], [Encumbrance2])"),  3.2},
        { SV("tlu('Strider', [Character], [Encumbrance2])"),  8. },
        { SV("sum([Encumbrance2])"),                         13.2},
        { SV("sum([Items, Weight] > 2)"),                     4. },
        { SV("sum([Items, Weight])"),                        28.1},
        // Columns are supposed to be case-insensitive
        { SV("sum([Items, weight])"),                        28.1},
        // test the alias
        { SV("[Overview, Encumbrance, 1]"),                   2. },
        // case insensitive
        { SV("[overview, encumbrance, 1]"),                   2. },
        { SV("cell('overview', 'encumbrance', 1)"),           2. },
        { SV("cell('Overview', 'Encumbrance', 1)"),           2. },
        { SV("f(col('overview', 'encumbrance', 1, 1))"),      2. },
        { SV("f(row('overview', 'encumbrance', 'encumbrance', 1))"),  2. },
    };
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    {
        SpreadSheet* sheet = &ms.sheets[0];
        // can't alias a sheet that doesn't already exist
        int err = drsp_set_sheet_alias(ctx, (SheetHandle)sheet, "Overview", sizeof("Overview")-1);
        TestExpectTrue(err);
    }
    for(int i = 0; i < ms.n; i++){
        SpreadSheet* sheet = &ms.sheets[i];
        int e = drsp_set_sheet_name(ctx, (SheetHandle)sheet, sheet->name.text, sheet->name.length);
        TestAssertFalse(e);
        for(int i = 0; i < sheet->colnames.n; i++){
            int e = drsp_set_col_name(ctx, (SheetHandle)sheet, i, sheet->colnames.data[i], sheet->colnames.lengths[i]);
            TestAssertFalse(e);
        }
        SheetHandle sheethandle = (SheetHandle)sheet;
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
                if(!row->lengths[c]) continue;
                if(e){ // don't bloat the stats
                    TestAssertFalse(e);
                }
            }
        }
    }
    {
        SpreadSheet* sheet = &ms.sheets[0];
        int err = drsp_set_sheet_alias(ctx, (SheetHandle)sheet, "Overview", sizeof("Overview")-1);
        TestAssertFalse(err);
    }
    for(size_t i = 0; i < arrlen(cases); i++){
        struct test_case* c = &cases[i];
        DrSpreadResult val = {0};
        StringView sv = c->sv;
        SpreadSheet* sheet = &ms.sheets[0];
        int err = drsp_evaluate_string(ctx, (SheetHandle)sheet, sv.text, sv.length, &val, -1, -1);
        TestExpectFalse(err);
        TestExpectEquals((uintptr_t)val.kind, DRSP_RESULT_NUMBER);
        TestExpectEquals(val.d, c->value); // suck it, "always use an epsilon" bots.
    }
    {   // Test that deleting a sheet works (and asan doesn't get mad).
        DrSpreadResult val = {0};
        int err = drsp_evaluate_string(ctx, (SheetHandle)&ms.sheets[0], "[Encumbrance, 1]", sizeof("[Encumbrance, 1]")-1, &val, -1, -1);
        TestAssertFalse(err);
        TestExpectEquals((uintptr_t)val.kind, DRSP_RESULT_NUMBER);
        TestExpectEquals(val.d, 2.);
        int e = drsp_del_sheet(ctx, (SheetHandle)&ms.sheets[1]);
        TestAssertFalse(e);
        err = drsp_evaluate_string(ctx, (SheetHandle)&ms.sheets[0], "[Encumbrance, 1]", sizeof("[Encumbrance, 1]")-1, &val, -1, -1);
        TestAssert(err);
    }
    drsp_destroy_ctx(ctx);
    ctx = drsp_create_ctx(&ops);
    for(int i = 0; i < ms.n; i++){
        SpreadSheet* sheet = &ms.sheets[i];
        int e = drsp_set_sheet_name(ctx, (SheetHandle)sheet, sheet->name.text, sheet->name.length);
        TestAssertFalse(e);
        for(int i = 0; i < sheet->colnames.n; i++){
            int e = drsp_set_col_name(ctx, (SheetHandle)sheet, i, sheet->colnames.data[i], sheet->colnames.lengths[i]);
            TestAssertFalse(e);
        }
        SheetHandle sheethandle = (SheetHandle)sheet;
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
                if(!row->lengths[c]) continue;
                if(e){ // don't bloat the stats
                    TestAssertFalse(e);
                }
            }
        }
    }
    {
        int err = drsp_evaluate_formulas(ctx);
        TestExpectEquals(err, 0);
    }
    StringView expected[] = {
        SV("2"),
        SV("3.2"),
        SV("8"),
    };
    for(size_t i = 0; i < arrlen(expected); i++){
        SpreadSheet* sheet = &ms.sheets[0];
        const SheetRow* disp = &sheet->display[i];
        TestAssertEquals(disp->n, 3);
        StringView weight = {strlen(disp->data[1]), disp->data[1]};
        TestExpectEquals2(sv_equals, weight, expected[i]);
        weight = (StringView){strlen(disp->data[2]), disp->data[2]};
        TestExpectEquals2(sv_equals, weight, expected[i]);
    }
    drsp_destroy_ctx(ctx);
    cleanup_multisheet(&ms);
    EXPECT_NO_LEAKS();
    TESTEND();
}

TestFunction(TestCaching){
    struct TestStats TEST_stats = {0};
    SpreadSheet sheet = {0};
    int err = read_csv_from_string(&sheet,
        "=1\n"
        "=0/0\n"
        "\n"
        "a\n"
        "=asd()\n"
    );
    SheetRow expected[] = {
        ROW("1"),
        ROW("nan"),
        ROW(""),
        ROW("a"),
        ROW("error"),
    };
    SheetRow expected2[] = {
        ROW(""),
        ROW(""),
        ROW(""),
        ROW(""),
        ROW(""),
    };
    TestAssertFalse(err);
    TestAssertEquals(sheet.rows, arrlen(expected));
    SheetOps ops = sheet_ops();
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    SheetHandle sheethandle = (SheetHandle)&sheet;
    int e = drsp_set_sheet_name(ctx, sheethandle, "$this", 5);
    TestAssertFalse(e);
    for(intptr_t r = 0; r < sheet.rows; r++){
        const SheetRow* row = &sheet.cells[r];
        for(int c = 0; c < row->n; c++){
            if(!row->lengths[c]) continue;
            e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
            // don't bloat the stats
            if(e) TestAssertFalse(e);
        }
    }
    int nerr = drsp_evaluate_formulas(ctx);
    TestExpectEquals(nerr, 1);
    for(size_t i = 0; i < arrlen(expected); i++){
        const SheetRow* display_row = &sheet.display[i];
        const SheetRow* expected_row = &expected[i];
        TestAssertEquals(display_row->n, expected_row->n);
        for(int j = 0; j < display_row->n; j++){
            TEST_stats.executed++;
            const char* lhs = display_row->data[j];
            const char* rhs = expected_row->data[j];
            if(!streq(lhs, rhs)){
                TEST_stats.failures++;
                  TestReport("Test condition failed");
                  TestReport("row %zu, col %d", i, j);
                  TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                          _test_color_green, lhs, _test_color_reset,
                          _test_color_red, _test_color_reset,
                          _test_color_green, rhs, _test_color_reset);
            }
        }
    }
    // set a cell to make it think the sheet is dirty
    {
        const SheetRow* row = &sheet.cells[4];
        e = drsp_set_cell_str(ctx, sheethandle, 4, 0, row->data[0], row->lengths[0]);
        // don't bloat the stats
        if(e) TestAssertFalse(e);
    }
    cleanup_sheet(&sheet);
    sheet = (SpreadSheet){0};
    err = read_csv_from_string(&sheet,
        "=1\n"
        "=0/0\n"
        "\n"
        "a\n"
        "=asd()\n");
    TestAssertFalse(err);
    TestAssertEquals(sheet.rows, arrlen(expected));
    // At this point, the ctx thinks our display is full of the values, but we
    // have reset it to blank.
    nerr = drsp_evaluate_formulas(ctx);
    TestExpectEquals(nerr, 1);
    for(size_t i = 0; i < arrlen(expected2); i++){
        const SheetRow* display_row = &sheet.display[i];
        const SheetRow* expected_row = &expected2[i];
        TestAssertEquals(display_row->n, expected_row->n);
        for(int j = 0; j < display_row->n; j++){
            TEST_stats.executed++;
            const char* lhs = display_row->data[j];
            const char* rhs = expected_row->data[j];
            if(!streq(lhs, rhs)){
                TEST_stats.failures++;
                  TestReport("Test condition failed");
                  TestReport("row %zu, col %d", i, j);
                  TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                          _test_color_green, lhs, _test_color_reset,
                          _test_color_red, _test_color_reset,
                          _test_color_green, rhs, _test_color_reset);
            }
        }
    }
    drsp_destroy_ctx(ctx);
    cleanup_sheet(&sheet);
    EXPECT_NO_LEAKS();
    return TEST_stats;
}

TestFunction(TestUserFunctions){
    TESTBEGIN();
    MultiSpreadSheet ms = {0};
    SheetOps ops = multisheet_ops(&ms);
    SpreadSheet* f = multisheet_alloc(&ms);
    SpreadSheet* s = multisheet_alloc(&ms);
    f = &ms.sheets[0];
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    TestAssert(ctx);
    SheetHandle func = (SheetHandle)f;
    int err;
    err = drsp_set_sheet_name(ctx, func, "example", sizeof("example")-1);
    TestAssertFalse(err);

    unsigned flags;
    err = drsp_set_sheet_flags(ctx, func, DRSP_SHEET_FLAGS_IS_FUNCTION);
    TestAssertFalse(err);
    flags = drsp_get_sheet_flags(ctx, func);
    TestExpectEquals(flags, DRSP_SHEET_FLAGS_IS_FUNCTION);

    err = drsp_set_sheet_flag(ctx, func, DRSP_SHEET_FLAGS_IS_FUNCTION, 0);
    TestAssertFalse(err);
    flags = drsp_get_sheet_flags(ctx, func);
    TestExpectEquals(flags, DRSP_SHEET_FLAGS_NONE);

    err = drsp_set_sheet_flag(ctx, func, DRSP_SHEET_FLAGS_IS_FUNCTION, 1);
    TestAssertFalse(err);
    flags = drsp_get_sheet_flags(ctx, func);
    TestExpectEquals(flags, DRSP_SHEET_FLAGS_IS_FUNCTION);

    intptr_t param_rows[] = {0, 0};
    intptr_t param_cols[arrlen(param_rows)] = {1, 0};
    err = drsp_set_function_params(ctx, func, arrlen(param_rows), param_rows, param_cols);
    TestAssertFalse(err);
    #define X(a) ""a, sizeof(""a)-1
    err = drsp_set_cell_str(ctx, func, param_rows[0], param_cols[0], X("=1"));
    TestAssertFalse(err);
    err = drsp_set_cell_str(ctx, func, 0, 0, X("13"));
    TestAssertFalse(err);
    err = drsp_set_cell_str(ctx, func, 1, 1, X("=(a1+b1)*2"));
    #undef X
    TestAssertFalse(err);
    err = drsp_set_function_output(ctx, func, 1, 1);
    TestAssertFalse(err);
    SheetRow expected[] = {
        ROW("13", "1"),
        ROW("", "28"),
    };
    for(size_t i = 0; i < arrlen(expected); i++){
        SheetRow d = {0};
        const SheetRow* e = &expected[i];
        for(int j = 0; j < e->n; j++){
            sheet_row_push(&d, drsp_strdup(""));
        }
        sheet_push_row(f, (SheetRow){0}, d);
    }
    err = drsp_evaluate_formulas(ctx);
    TestExpectFalse(err);

    for(size_t i = 0; i < arrlen(expected); i++){
        const SheetRow* d = &f->display[i];
        const SheetRow* e = &expected[i];
        TestAssertEquals(d->n, e->n);
        for(int j = 0; j < d->n; j++){
            TEST_stats.executed++;
            const char* lhs = d->data[j];
            const char* rhs = e->data[j];
            if(!streq(lhs, rhs)){
                TEST_stats.failures++;
                  TestReport("Test condition failed");
                  TestReport("row %zu, col %d", i, j);
                  TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                          _test_color_green, lhs, _test_color_reset,
                          _test_color_red, _test_color_reset,
                          _test_color_green, rhs, _test_color_reset);
            }
        }
    }

    DrSpreadResult r = {.kind=-1};
    StringView args[] = {
        SV("3"),
        SV("=2"),
    };
    err = drsp_evaluate_function(ctx, func, arrlen(args), args, &r);
    TestExpectFalse(err);
    TestExpectEquals((uintptr_t)r.kind, DRSP_RESULT_NUMBER);
    TestExpectEquals(r.d, 10.);
    SheetHandle sheet = (SheetHandle)s;
    err = drsp_set_sheet_name(ctx, sheet, "foo", 3);
    TestAssertFalse(err);
    err = drsp_set_cell_str(ctx, sheet, 0, 0, "=example(1, 2)", sizeof("=example(1, 2)")-1);
    TestAssertFalse(err);
    err = drsp_set_cell_str(ctx, sheet, 0, 1, "=example()", sizeof("=example()")-1);
    TestAssertFalse(err);
    err = drsp_set_cell_str(ctx, sheet, 0, 2, "=example('1', '2')", sizeof("=example('1', '2')")-1);
    TestAssertFalse(err);
    r = (DrSpreadResult){.kind=-1};
    err = drsp_evaluate_string(ctx, sheet, "=a1", 3, &r, 0, 0);
    TestAssertFalse(err);
    TestExpectEquals((uintptr_t)r.kind, DRSP_RESULT_NUMBER);
    TestExpectEquals(r.d, 6.);
    {
        SheetRow r = {0};
        sheet_row_push(&r, drsp_strdup(""));
        sheet_row_push(&r, drsp_strdup(""));
        sheet_row_push(&r, drsp_strdup(""));
        sheet_push_row(s, (SheetRow){0}, r);
    }
    err = drsp_evaluate_formulas(ctx);
    TestAssertEquals(err, 2);
    TestAssertEquals(s->display[0].n, 3);
    TestExpectEquals2(streq, s->display[0].data[0], "6");
    TestExpectEquals2(streq, s->display[0].data[1], "error");
    TestExpectEquals2(streq, s->display[0].data[2], "error");

    err = drsp_clear_function_params(ctx, func);
    TestAssertFalse(err);

    err = drsp_del_sheet(ctx, func);
    TestAssertFalse(err);
    // deleting twice should fail
    err = drsp_del_sheet(ctx, func);
    TestAssert(err);
    cleanup_multisheet(&ms);

    drsp_destroy_ctx(ctx);

    EXPECT_NO_LEAKS();
    TESTEND();
}

TestFunction(TestShortColNames){
    TESTBEGIN();
    // We're going to guard a lot of the assertions in this testcase
    // as this test assumes the setup code already works.
    const char* input =
        "sheet\n"
        "xp | gp | \n"
        " 1 | 2 | 3 \n"
        " =sum(gp) | =xp1+gp1 | =c1+xp1\n"
    ;
    SheetRow expected[] = {
        ROW("1", "2", "3"),
        ROW("5", "3", "4"),
    };
    MultiSpreadSheet ms = {0};
    {
        int err = read_multi_csv_from_string(&ms, input);
        if(err) TestAssertFalse(err);
        if(ms.n != 1) TestAssertEquals(ms.n, 1);
    }

    {
        SheetOps ops = multisheet_ops(&ms);
        DrSpreadCtx* ctx = drsp_create_ctx(&ops);
        SpreadSheet* sheet = &ms.sheets[0];
        TestAssertEquals(sheet->colnames.n, 3);
        SheetHandle sh = (SheetHandle)sheet;
        int e = drsp_set_sheet_name(ctx, sh, sheet->name.text, sheet->name.length);
        if(e) TestAssertFalse(e);
        for(int i = 0; i < sheet->colnames.n; i++){
            e = drsp_set_col_name(ctx, sh, i, sheet->colnames.data[i], sheet->colnames.lengths[i]);
            if(e) TestAssertFalse(e);
        }
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                e = drsp_set_cell_str(ctx, sh, r, c, row->data[c], row->lengths[c]);
                if(e) TestAssertFalse(e);
            }
        }
        e = drsp_evaluate_formulas(ctx);
        if(e) TestAssertFalse(e);
        drsp_destroy_ctx(ctx);
    }
    if(arrlen(expected) != ms.sheets[0].rows)
        TestAssertEquals(arrlen(expected), ms.sheets[0].rows);
    for(size_t i = 0; i < arrlen(expected); i++){
        SpreadSheet* sheet = &ms.sheets[0];
        const SheetRow* disp = &sheet->display[i];
        const SheetRow* exp = &expected[i];
        if(disp->n != exp->n) TestAssertEquals(disp->n, exp->n);
        for(int j = 0; j < disp->n; j++){
            StringView d = {disp->lengths[j], disp->data[j]};
            StringView e = {exp->lengths[j], exp->data[j]};
            TEST_stats.executed++;
            if(!sv_equals(d, e)){

                TEST_stats.failures++;
                TestReport("row %zu, col: %d | (actual) \"%.*s\" != \"%.*s\" (expected)", i, j, (int)d.length, d.text, (int)e.length, e.text);
            }
        }
    }
    cleanup_multisheet(&ms);

    EXPECT_NO_LEAKS();
    TESTEND();
}

TestFunction(TestDeleteColNames){
    TESTBEGIN();
    int err;
    const char* input =
        "sheet\n"
        "frobnicate | barawax\n"
        "=barawax1 | 2\n"
        "=sum(barawax) | 3\n"
    ;
    MultiSpreadSheet ms = {0};
    err = read_multi_csv_from_string(&ms, input);
    if(err) TestAssertFalse(err);
    TestAssertEquals(ms.n, 1);
    SpreadSheet* sheet = &ms.sheets[0];
    TestAssertEquals(sheet->rows, 2);
    SheetHandle sh = (SheetHandle)sheet;
    SheetOps ops = multisheet_ops(&ms);
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    TestAssert(ctx);
    err = drsp_set_sheet_name(ctx, sh, sheet->name.text, sheet->name.length);
    if(err) TestAssertFalse(err);

    for(intptr_t r = 0; r < sheet->rows; r++){
        const SheetRow* row = &sheet->cells[r];
        for(int c = 0; c < row->n; c++){
            if(!row->lengths[c]) continue;
            err = drsp_set_cell_str(ctx, sh, r, c, row->data[c], row->lengths[c]);
            // don't bloat the stats
            if(err) TestAssertFalse(err);
        }
    }
    for(int c = 0; c < sheet->colnames.n; c++){
        err = drsp_set_col_name(ctx, sh, c, sheet->colnames.data[c], sheet->colnames.lengths[c]);
        if(err) TestAssertFalse(err);
    }
    err = drsp_evaluate_formulas(ctx);
    TestAssertFalse(err);

    // rename to hello
    err = drsp_set_col_name(ctx, sh, 1, "hello", 5);
    TestAssertFalse(err);

    err = drsp_evaluate_formulas(ctx);
    TestExpectEquals(err, 2);

    // rename back to barawax, but upper case
    err = drsp_set_col_name(ctx, sh, 1, "BARAWAX", 7);
    TestAssertFalse(err);

    err = drsp_evaluate_formulas(ctx);
    TestExpectEquals(err, 0);

    // remove column name
    err = drsp_set_col_name(ctx, sh, 1, "", 0);
    TestAssertFalse(err);

    err = drsp_evaluate_formulas(ctx);
    TestExpectEquals(err, 2);

    cleanup_multisheet(&ms);
    drsp_destroy_ctx(ctx);
    EXPECT_NO_LEAKS();
    TESTEND();
}


static
int
test_set_display_number(void* m, SheetHandle hnd, intptr_t row, intptr_t col, double val){
    (void)hnd;
    if(row != DRSP_IDX_EXTRA_DIMENSIONAL) return 0;
    double* v = m;
    v[col] = val;
    return 0;
}

static
int
test_set_display_error(void* m, SheetHandle hnd, intptr_t row, intptr_t col, const char* mess, size_t len){
    (void)m, (void)hnd, (void)row, (void)col, (void)mess, (void)len;
    if(row != DRSP_IDX_EXTRA_DIMENSIONAL) return 0;
    return 1;
}

static
int
test_set_display_string(void* m, SheetHandle hnd, intptr_t row, intptr_t col, const char* mess, size_t len){
    (void)m, (void)hnd, (void)row, (void)col, (void)mess, (void)len;
    if(row != DRSP_IDX_EXTRA_DIMENSIONAL) return 0;
    return 1;
}


TestFunction(TestExtraDimensional){
    TESTBEGIN();
    int err;
    const char* input =
        "sheet\n"
        "foo | bar\n"
        "2 | 5\n"
        "3 | 6\n"
    ;
    MultiSpreadSheet ms = {0};
    err = read_multi_csv_from_string(&ms, input);
    if(err) TestAssertFalse(err);
    TestAssertEquals(ms.n, 1);
    SpreadSheet* sheet = &ms.sheets[0];
    TestAssertEquals(sheet->rows, 2);
    SheetHandle sh = (SheetHandle)sheet;
    double nums[12] = {0.};
    SheetOps ops = {
        .ctx = nums,
        .set_display_error=test_set_display_error,
        .set_display_string=test_set_display_string,
        .set_display_number=test_set_display_number,
    };
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    TestAssert(ctx);
    err = drsp_set_sheet_name(ctx, sh, sheet->name.text, sheet->name.length);
    if(err) TestAssertFalse(err);

    for(intptr_t r = 0; r < sheet->rows; r++){
        const SheetRow* row = &sheet->cells[r];
        for(int c = 0; c < row->n; c++){
            if(!row->lengths[c]) continue;
            err = drsp_set_cell_str(ctx, sh, r, c, row->data[c], row->lengths[c]);
            // don't bloat the stats
            if(err) TestAssertFalse(err);
        }
    }
    TestAssertEquals(sheet->colnames.n, 2);
    TestAssertEquals2(sv_equals, SV("foo"), ((StringView){.text=sheet->colnames.data[0], .length=sheet->colnames.lengths[0]}));
    TestAssertEquals2(sv_equals, SV("bar"), ((StringView){.text=sheet->colnames.data[1], .length=sheet->colnames.lengths[1]}));
    for(int c = 0; c < sheet->colnames.n; c++){
        err = drsp_set_col_name(ctx, sh, c, sheet->colnames.data[c], sheet->colnames.lengths[c]);
        if(err) TestAssertFalse(err);
    }
    struct {
        StringView txt;
        double expected;
    }cases[] = {
        {SV("=sum(a)"),    5.},
        {SV("=sum(b)"),   11.},
        {SV("=sum(foo)"),  5.},
        {SV("=sum(bar)"), 11.},
        {SV("=1+1"),       2.},
    };
    _Static_assert(arrlen(nums) >= arrlen(cases), "");
    for(size_t i = 0; i < arrlen(cases); i++){
        int err = drsp_set_extra_dimensional_str(ctx, sh, i, cases[i].txt.text, cases[i].txt.length);
        if(err) TestAssertFalse(err);
    }

    err = drsp_evaluate_formulas(ctx);
    TestAssertFalse(err);
    for(size_t i = 0; i < arrlen(cases); i++)
        TestExpectEquals(nums[i], cases[i].expected);

    cleanup_multisheet(&ms);
    drsp_destroy_ctx(ctx);
    EXPECT_NO_LEAKS();
    TESTEND();
}

TestFunction(TestEditing){
    TESTBEGIN();
    const char* input =
        "1 | =sum(a)\n"
        "2 | =cat('a', 'b')\n"
    ;
    SpreadSheet sheet = {0};
    int err = read_csv_from_string(&sheet, input);
    TestAssertFalse(err);
    SheetOps ops = sheet_ops();
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    TestAssert(ctx);
    SheetHandle sheethandle = (SheetHandle)&sheet;
    err = drsp_set_sheet_name(ctx, sheethandle, "$this", 5);
    TestAssertFalse(err);

    for(intptr_t r = 0; r < sheet.rows; r++){
        const SheetRow* row = &sheet.cells[r];
        for(int c = 0; c < row->n; c++){
            if(!row->lengths[c]) continue;
            err = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
            // don't bloat the stats
            if(err) TestAssertFalse(err);
        }
    }
    int nerr = drsp_evaluate_formulas(ctx);
    TestExpectEquals(nerr, 0);
    {
        SheetRow expected[] = {
            ROW("1", "3"),
            ROW("2", "ab"),
        };
        TestAssertEquals(sheet.rows, arrlen(expected));
        for(size_t i = 0; i < arrlen(expected); i++){
            const SheetRow* display_row = &sheet.display[i];
            const SheetRow* expected_row = &expected[i];
            TestAssertEquals(display_row->n, expected_row->n);
            for(int j = 0; j < display_row->n; j++){
                TEST_stats.executed++;
                const char* lhs = display_row->data[j];
                const char* rhs = expected_row->data[j];
                if(!streq(lhs, rhs)){
                    TEST_stats.failures++;
                      TestReport("Test condition failed");
                      TestReport("row %zu, col %d", i, j);
                      TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                              _test_color_green, lhs, _test_color_reset,
                              _test_color_red, _test_color_reset,
                              _test_color_green, rhs, _test_color_reset);
                }
            }
        }
    }
    #define SET_CELL(r, c, x) drsp_set_cell_str(ctx, sheethandle, r, c, x, sizeof(x)-1)
    err = SET_CELL(1, 0, "3");
    if(err) TestAssertFalse(err);

    {
        SheetRow expected[] = {
            ROW("1", "4"),
            ROW("3", "ab"),
        };
        nerr = drsp_evaluate_formulas(ctx);
        TestExpectEquals(nerr, 0);
        for(size_t i = 0; i < arrlen(expected); i++){
            const SheetRow* display_row = &sheet.display[i];
            const SheetRow* expected_row = &expected[i];
            TestAssertEquals(display_row->n, expected_row->n);
            for(int j = 0; j < display_row->n; j++){
                TEST_stats.executed++;
                const char* lhs = display_row->data[j];
                const char* rhs = expected_row->data[j];
                if(!streq(lhs, rhs)){
                    TEST_stats.failures++;
                      TestReport("Test condition failed");
                      TestReport("row %zu, col %d", i, j);
                      TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                              _test_color_green, lhs, _test_color_reset,
                              _test_color_red, _test_color_reset,
                              _test_color_green, rhs, _test_color_reset);
                }
            }
        }
    }

    err = SET_CELL(1, 1, "=CAT('', '')");
    if(err) TestAssertFalse(err);

    {
        SheetRow expected[] = {
            ROW("1", "4"),
            ROW("3", ""),
        };
        nerr = drsp_evaluate_formulas(ctx);
        TestExpectEquals(nerr, 0);
        for(size_t i = 0; i < arrlen(expected); i++){
            const SheetRow* display_row = &sheet.display[i];
            const SheetRow* expected_row = &expected[i];
            TestAssertEquals(display_row->n, expected_row->n);
            for(int j = 0; j < display_row->n; j++){
                TEST_stats.executed++;
                const char* lhs = display_row->data[j];
                const char* rhs = expected_row->data[j];
                if(!streq(lhs, rhs)){
                    TEST_stats.failures++;
                      TestReport("Test condition failed");
                      TestReport("row %zu, col %d", i, j);
                      TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                              _test_color_green, lhs, _test_color_reset,
                              _test_color_red, _test_color_reset,
                              _test_color_green, rhs, _test_color_reset);
                }
            }
        }
    }

    err = SET_CELL(1, 1, "=CAT(''");
    if(err) TestAssertFalse(err);

    {
        SheetRow expected[] = {
            ROW("1", "4"),
            ROW("3", "error"),
        };
        nerr = drsp_evaluate_formulas(ctx);
        TestExpectEquals(nerr, 1);
        for(size_t i = 0; i < arrlen(expected); i++){
            const SheetRow* display_row = &sheet.display[i];
            const SheetRow* expected_row = &expected[i];
            TestAssertEquals(display_row->n, expected_row->n);
            for(int j = 0; j < display_row->n; j++){
                TEST_stats.executed++;
                const char* lhs = display_row->data[j];
                const char* rhs = expected_row->data[j];
                if(!streq(lhs, rhs)){
                    TEST_stats.failures++;
                      TestReport("Test condition failed");
                      TestReport("row %zu, col %d", i, j);
                      TestReport("'%s%s%s' %s!=%s '%s%s%s'",
                              _test_color_green, lhs, _test_color_reset,
                              _test_color_red, _test_color_reset,
                              _test_color_green, rhs, _test_color_reset);
                }
            }
        }
    }

    drsp_destroy_ctx(ctx);
    cleanup_sheet(&sheet);
    EXPECT_NO_LEAKS();
    TESTEND();
    #undef SET_CELL
}

TestFunction(TestNamedCells){
    TESTBEGIN();
    const char* input =
        "1 | 2\n"
        "3 | 4\n"
    ;
    SpreadSheet sheet = {0};
    int err = read_csv_from_string(&sheet, input);
    TestAssertFalse(err);
    SheetOps ops = sheet_ops();
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    TestAssert(ctx);
    SheetHandle handle = (SheetHandle)&sheet;
    err = drsp_set_sheet_name(ctx, handle, "a", 1);
    TestAssertFalse(err);
    for(intptr_t r = 0; r < sheet.rows; r++){
        const SheetRow* row = &sheet.cells[r];
        for(int c = 0; c < row->n; c++){
            if(!row->lengths[c]) continue;
            err = drsp_set_cell_str(ctx, handle, r, c, row->data[c], row->lengths[c]);
            // don't bloat the stats
            if(err) TestAssertFalse(err);
        }
    }
    {
        DrSpreadResult val = {0};
        StringView qs[] = {
            SV("foo"),
            SV("cell('foo')"),
            SV("[a, foo]"),
            SV("cell('a', 'foo')"),
        };
        for(size_t i = 0; i < arrlen(qs); i++){
            StringView q = qs[i];
            err = drsp_evaluate_string(ctx, handle, q.text, q.length, &val, -1, -1);
            TestExpectTrue(err);
        }

        StringView name = SV("foo");
        err = drsp_set_named_cell(ctx, handle, name.text, name.length, 0, 0);
        TestAssertFalse(err);

        for(size_t i = 0; i < arrlen(qs); i++){
            StringView q = qs[i];
            err = drsp_evaluate_string(ctx, handle, q.text, q.length, &val, -1, -1);
            if(err){
                TestAssertFalse(err);
            }
            TestAssertFalse(err);
            TestAssertEquals((int)val.kind, DRSP_RESULT_NUMBER);
            TestAssertEquals(val.d, 1.);
        }

        err = drsp_set_named_cell(ctx, handle, name.text, name.length, 1, 1);
        TestAssertFalse(err);

        for(size_t i = 0; i < arrlen(qs); i++){
            StringView q = qs[i];
            err = drsp_evaluate_string(ctx, handle, q.text, q.length, &val, -1, -1);
            TestAssertFalse(err);
            TestAssertEquals((int)val.kind, DRSP_RESULT_NUMBER);
            TestAssertEquals(val.d, 4.);
        }

        err = drsp_clear_named_cell(ctx, handle, name.text, name.length);
        TestAssertFalse(err);

        for(size_t i = 0; i < arrlen(qs); i++){
            StringView q = qs[i];
            err = drsp_evaluate_string(ctx, handle, q.text, q.length, &val, -1, -1);
            TestExpectTrue(err);
        }

        err = drsp_set_named_cell(ctx, handle, name.text, name.length, 0, 1);
        TestAssertFalse(err);

        for(size_t i = 0; i < arrlen(qs); i++){
            StringView q = qs[i];
            err = drsp_evaluate_string(ctx, handle, q.text, q.length, &val, -1, -1);
            TestAssertFalse(err);
            TestAssertEquals((int)val.kind, DRSP_RESULT_NUMBER);
            TestAssertEquals(val.d, 2.);
        }

        err = drsp_set_named_cell(ctx, handle, name.text, name.length, 0, 1);
        TestAssertFalse(err);

        for(size_t i = 0; i < arrlen(qs); i++){
            StringView q = qs[i];
            err = drsp_evaluate_string(ctx, handle, q.text, q.length, &val, -1, -1);
            TestAssertFalse(err);
            TestAssertEquals((int)val.kind, DRSP_RESULT_NUMBER);
            TestAssertEquals(val.d, 2.);
        }

        err = drsp_clear_named_cell(ctx, handle, name.text, name.length);
        TestAssertFalse(err);

        // Unclear if clearing twice should not be an error.
        err = drsp_clear_named_cell(ctx, handle, name.text, name.length);
        TestAssertFalse(err);

    }
    drsp_destroy_ctx(ctx);
    cleanup_sheet(&sheet);
    EXPECT_NO_LEAKS();
    TESTEND();
}

TestFunction(TestDependants){
    TESTBEGIN();
    const char* input =

        "Sheet1\n"
        // -------
        "\n"
        "=cell('Sheet2', 'a', 1)\n"
        "=3\n"
        "---\n"

        "Sheet2\n"
        // -------
        "\n"
        "2\n"
        "=cell('Sheet1', 'a', 1)\n"
        "---\n"

        "Sheet3\n"
        // -------
        "\n"
        "=cell('Sheet1', 'a', 2)\n"
        " \n"
        "---\n"

        "Sheet4\n"
        "\n"
        "=cell('Sheet2', 'a', 1)\n"
        "=a1\n"
        "---\n"
        ;
    MultiSpreadSheet ms = {0};
    {
        int err = read_multi_csv_from_string(&ms, input);
        TestAssertFalse(err);
    }
    SheetOps ops = multisheet_ops(&ms);
    DrSpreadCtx* ctx = drsp_create_ctx(&ops);
    for(int i = 0; i < ms.n; i++){
        SpreadSheet* sheet = &ms.sheets[i];
        int e = drsp_set_sheet_name(ctx, (SheetHandle)sheet, sheet->name.text, sheet->name.length);
        TestAssertFalse(e);
        for(int i = 0; i < sheet->colnames.n; i++){
            int e = drsp_set_col_name(ctx, (SheetHandle)sheet, i, sheet->colnames.data[i], sheet->colnames.lengths[i]);
            TestAssertFalse(e);
        }
        SheetHandle sheethandle = (SheetHandle)sheet;
        for(intptr_t r = 0; r < sheet->rows; r++){
            const SheetRow* row = &sheet->cells[r];
            for(int c = 0; c < row->n; c++){
                e = drsp_set_cell_str(ctx, sheethandle, r, c, row->data[c], row->lengths[c]);
                if(!row->lengths[c]) continue;
                if(e){ // don't bloat the stats
                    TestAssertFalse(e);
                }
            }
        }
    }
    {
        int nerr = drsp_evaluate_formulas(ctx);
        TestExpectEquals(nerr, 0);
        for(int i = 0; i < ms.n; i++){
            SpreadSheet* sheet = &ms.sheets[i];
            TestAssertFalse(drsp_sheet_is_dirty(ctx, (SheetHandle)sheet));
        }
    }
    {
        SheetRow expected[4][2] = {
            {
                ROW("2"),
                ROW("3"),
            },
            {
                ROW("2"),
                ROW("2"),
            },
            {
                ROW("3"),
                ROW(""),
            },
            {
                ROW("2"),
                ROW("2"),
            },
        };
        TestAssertEquals(ms.n, sizeof expected / sizeof expected[0]);
        for(int i = 0; i < ms.n; i++){
            SpreadSheet* sheet = &ms.sheets[i];
            if(sizeof expected[i]/ sizeof expected[i][0] != sheet->rows){
                TestPrintValue("i", i);
                TestPrintValue("sheet->rows", sheet->rows);
                for(int r = 0; r < sheet->rows; r++){
                    TestPrintValue("r", r);
                    const SheetRow* c =  &sheet->cells[r];
                    TestPrintValue("c->n", c->n);
                    for(int j = 0; j < c->n; j++){
                        TestPrintValue("j", j);
                        TestPrintValue("c->data[j]", c->data[j]);
                    }
                }
            }
            TestAssertEquals(sizeof expected[i]/ sizeof expected[i][0], sheet->rows);
            for(int r = 0; r < sheet->rows; r++){
                const SheetRow* d = &sheet->display[r];
                const SheetRow* e = &expected[i][r];
                TestAssertEquals(d->n, e->n);
                for(int j = 0; j < d->n; j++){
                    TestExpectEquals(d->lengths[j], e->lengths[j]);
                    TestExpectEquals2(streq, d->data[j], e->data[j]);
                }
            }
        }
        {
            size_t n = 0;
            SheetHandle* handles = drsp_sheet_get_dependants(ctx, (SheetHandle)&ms.sheets[0], &n);
            TestExpectEquals(n, 2);
            TestArrayContains(SheetHandle, handles, handles+n, (SheetHandle)&ms.sheets[1]);
            TestArrayContains(SheetHandle, handles, handles+n, (SheetHandle)&ms.sheets[2]);
        }
        {
            size_t n = 0;
            SheetHandle* handles = drsp_sheet_get_dependants(ctx, (SheetHandle)&ms.sheets[1], &n);
            TestExpectEquals(n, 2);
            TestArrayContains(SheetHandle, handles, handles+n, (SheetHandle)&ms.sheets[0]);
            TestArrayContains(SheetHandle, handles, handles+n, (SheetHandle)&ms.sheets[3]);
        }
        {
            size_t n = 0;
            SheetHandle* handles = drsp_sheet_get_dependants(ctx, (SheetHandle)&ms.sheets[2], &n);
            (void)handles;
            TestExpectEquals(n, 0);
        }
        {
            size_t n = 0;
            SheetHandle* handles = drsp_sheet_get_dependants(ctx, (SheetHandle)&ms.sheets[3], &n);
            (void)handles;
            TestExpectEquals(n, 0);
        }
    }
    {
        DrspAtom a = drsp_atomize(ctx, "4", 1);
        TestAssert(a);
        int err = drsp_set_cell_atom(ctx, (SheetHandle)&ms.sheets[0], 0, 0, a);
        TestAssertFalse(err);
        int nerr = drsp_evaluate_formulas(ctx);
        TestExpectEquals(nerr, 0);
    }
    {
        SheetRow expected[4][2] = {
            {
                ROW("4"),
                ROW("3"),
            },
            {
                ROW("2"),
                ROW("4"),
            },
            {
                ROW("3"),
                ROW(""),
            },
            {
                ROW("2"),
                ROW("2"),
            },
        };
        TestAssertEquals(ms.n, sizeof expected / sizeof expected[0]);
        for(int i = 0; i < ms.n; i++){
            SpreadSheet* sheet = &ms.sheets[i];
            if(sizeof expected[i]/ sizeof expected[i][0] != sheet->rows){
                TestPrintValue("i", i);
                TestPrintValue("sheet->rows", sheet->rows);
                for(int r = 0; r < sheet->rows; r++){
                    TestPrintValue("r", r);
                    const SheetRow* c =  &sheet->cells[r];
                    TestPrintValue("c->n", c->n);
                    for(int j = 0; j < c->n; j++){
                        TestPrintValue("j", j);
                        TestPrintValue("c->data[j]", c->data[j]);
                    }
                }
            }
            TestAssertEquals(sizeof expected[i]/ sizeof expected[i][0], sheet->rows);
            for(int r = 0; r < sheet->rows; r++){
                const SheetRow* d = &sheet->display[r];
                const SheetRow* e = &expected[i][r];
                TestAssertEquals(d->n, e->n);
                for(int j = 0; j < d->n; j++){
                    TestExpectEquals(d->lengths[j], e->lengths[j]);
                    TestExpectEquals2(streq, d->data[j], e->data[j]);
                }
            }
        }
    }
    drsp_destroy_ctx(ctx);
    cleanup_multisheet(&ms);
    EXPECT_NO_LEAKS();
    TESTEND();
}


#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __wasm__
#pragma pop_macro("__FILE__")
#endif

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#ifdef DRSP_TEST_DYLINK
#include "drspread_allocators.c"
#else
#include "drspread.c"
#ifdef __clang__
#pragma clang assume_nonnull begin
#endif
static
SheetHandle _Null_unspecified*_Nullable
drsp_sheet_get_dependants(DrSpreadCtx* ctx, SheetHandle h, size_t* n){
    SheetData* d = sheet_lookup_by_handle(ctx, h);
    assert(d);

    if(!d) {*n = 0; return NULL;}
    *n = d->dependants.count;
    return d->dependants.data;
}
static
_Bool
drsp_sheet_is_dirty(DrSpreadCtx* ctx, SheetHandle h){
    SheetData* d = sheet_lookup_by_handle(ctx, h);
    assert(d);
    return d->dirty;
}
#ifdef __clang__
#pragma clang assume_nonnull end
#endif
#endif
