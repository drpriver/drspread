//
// Copyright © 2023, David Priver
//

#include "spreadsheet.h"
#include "drspread.h"
#include "testing.h"
#ifdef __wasm__
#pragma push_macro("__FILE__")
#pragma clang diagnostic ignored "-Wbuiltin-macro-redefined"
#define __FILE__ "<a href=TestDrSpread.c>TestDrSpread.c</a>"
#endif

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
static TestFunc TestDirectlyRecursiveShouldError;
static TestFunc TestIndirectlyRecursiveShouldError;
static TestFunc TestMultisheet;
static TestFunc TestColFunc;
static TestFunc TestRanges;
static TestFunc TestBadRanges;
static TestFunc TestNames;
static TestFunc TestComplexMultisheet;

int main(int argc, char** argv){
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
        RegisterTest(TestDirectlyRecursiveShouldError);
        RegisterTest(TestIndirectlyRecursiveShouldError);
        RegisterTest(TestMultisheet);
        RegisterTest(TestColFunc);
        RegisterTest(TestNames);
        RegisterTest(TestComplexMultisheet);
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
    int nerr = drsp_evaluate_formulas(ctx, sheethandle, NULL, 0);
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
#undef __func__
    return TEST_stats;
}

#define ROW(...) { \
    arrlen(((const char*[]){__VA_ARGS__})),  \
    ((const char*[]){__VA_ARGS__}), \
    ((size_t[]){0}), \
}
TestFunction(TestParsing){
    const char* input =
        // Supporting syntax like other spreadsheets.
        "=r(a1)\n"
        "=r(a1:b1)\n"
        "=r(a1:a3)\n"
        "=r(a1:a1)\n"

        "=r(a:a5)\n"
        "=r(a:5)\n"
        "=r(a5:a)\n"
        "=r(a5:)\n"

        "=r(c:c)\n"
        "=r(c:)\n"
        "=r(c)\n"
        //  This one not supported yet
        #if 0
        "=r(:c)\n"
        #endif

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

    ;
    // NOTE: We print out the internal 0-based offsets instead
    //       of the user-facing 1-based offsets.
    SheetRow expected[] = {
        ROW("R0([a, 0])"),
        ROW("R1R([a:b, 0])"),
        ROW("R1C([a, 0:2])"),
        ROW("R1C([a, 0:0])"),

        ROW("R1C([a, 0:4])"),
        ROW("R1C([a, 0:4])"),
        ROW("R1C([a, 4:-1])"),
        ROW("R1C([a, 4:-1])"),

        ROW("R1C([c, 0:-1])"),
        ROW("R1C([c, 0:-1])"),
        ROW("R1C([c, 0:-1])"),
        // Not supported yet
        #if 0
        ROW("R1C([c, 0:-1])"),
        #endif

        // [col, 1] -> 0d
        ROW("R0([a, 0])"),
        ROW("R0([a, 0])"),
        ROW("R0([a, 0])"),
        ROW("R0([a, -2147483647])"),
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
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestRanges){
    const char* input =
        " 0 | =[a,1] | = a1\n"
        " 1 | =['a', 1]\n"
        " 2 | =['a', $]\n"
        " 3 | =[\"$\", 3]\n"
        " 4 | =['$', 3]\n"
        " 5 | =sum([a]) | =sum(a)\n"
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
        [ 5] = ROW("5", "55", "55"),
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
        "=[a,]\n"
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

        "=[f, a,]\n"
        "=[f, ,a,]\n"
        "=[f, ,:, :]\n"
        "=[f, :,:, 2]\n"
        "=[f, ]\n"
        "=[f, '$', '3', '3']\n"
        "=sum([f, 'a])\n"
        "=sum([f, 'a', '3':'3'])\n"
        "=sum([f,3, 1:2])\n"
    ;
    SheetRow expected[] = {
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
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), arrlen(expected));
}

TestFunction(TestSpreadsheet1){
    const char* input =
        "=sum([c])                     | Axe         | 10\n"
        "=find('Food', [b])            | Torch       | 1\n"
        "=tlu('Plate Armor', [b], [c]) | Plate Armor | 50\n"
        "=min([c, :2])                 | Food        | 1 per potato\n"
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
        // a          |     b          |    c            |  d
        "=[b,1]       | 2              | 3               | =[d, 3]\n"
        "=[a,1]+[b,1] | = [a,2]+2      | =[a,2]          | 4\n"
        "2            | =-([a,1] > -1) | 3               | = sum([b, 3:4]) * sum([a,4:1])\n"
        "=[a,3]*[b,1] | 4              | =1-2---3*4/2-8-1 | = sum([a, :11])\n"
        "=sum([a,:4]) | = sum([d])     | = sum([b])\n"
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
        "=1+1   | =1-1    | =4/2       | =3*4 \n"
        "=2 = 1 | =2 == 1 | =2 != 1\n"
        "=1 < 2 | =1 > 2  | =1 >= 1    | =1 <= 2\n"
    ;
    SheetRow expected[] = {
        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}
TestFunction(TestUnOps){
    const char* input =
        "=!1      | =-2    | =+2\n"
        "=!!1     | =-!1   | =+!2\n"
        "=!!!1    | =-!!1  | =+!!2\n"
        "=-!-!-!1 | =-!+!1 | =+!-!2\n"
        "=-!-!-! | =-!+! | =+!-!\n"
    ;
    SheetRow expected[] = {
        ROW("0", "-2", "2"),
        ROW("1",  "0", "0"),
        ROW("0", "-1", "1"),
        ROW("0", "-1", "1"),
        ROW("error", "error", "error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 3);
}

TestFunction(TestFuncs){
    const char* input =
        "=sum([b])          | -1.5\n"
        "=avg([b])          | 3.5\n"
        "=min([b])          | 44      | hello\n"
        "=max([b])          | 14\n"
        "=mod([b,4])        | \n"
        "=abs([b,1])        | \n"
        "=floor([b,1])      | \n"
        "=ceil([b,1])       | \n"
        "=trunc([b,1])      | \n"
        "=round([b,1])      | \n"
        "=tlu(44, [b], [c]) | \n"
        "=find(44, [b]) | \n"
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
        "=try(ceil('a'), 'b')\n"
        "=try(ceil(1), 'b')\n"
        "=pow(2, 3)\n"
        "=eval('pow(2,4)')\n"
        "=call('pow', 3, 5)\n"
        "=sqrt(9)\n"
        "=prod([b])\n"

        "=if(1, 2, 3)\n"
        "=if('', 2, 3)\n"

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

    ;
    SheetRow expected[] = {
        [ 0] = ROW("60", "-1.5"),
        [ 1] = ROW("15", "3.5"),
        [ 2] = ROW("-1.5", "44", "hello"),
        [ 3] = ROW("44", "14"),
        [ 4] = ROW("2", ""),
        [ 5] = ROW("1.5", ""),
        [ 6] = ROW("-2", ""),
        [ 7] = ROW("-1", ""),
        [ 8] = ROW("-1", ""),
        [ 9] = ROW("-2", ""),
        [10] = ROW("hello", ""),

        [11] = ROW("3", ""),
        [12] = ROW(""),
        [13] = ROW("-12"),
        [14] = ROW("-13"),
        [15] = ROW("-12"),
        [16] = ROW("-12"),
        [17] = ROW(""),
        [18] = ROW("-12"),
        [19] = ROW("-13"),
        [20] = ROW("-13"),
        [21] = ROW("-12"),
        [22] = ROW(""),
        [23] = ROW("13"),
        [24] = ROW("12"),
        [25] = ROW("12"),
        [26] = ROW("12"),
        [27] = ROW(""),
        [28] = ROW("13"),
        [29] = ROW("12"),
        [30] = ROW("13"),
        [31] = ROW("12"),
        [32] = ROW(""),
        [33] = ROW("1"),
        [34] = ROW("2"),
        [35] = ROW("0"),
        [36] = ROW("b"),
        [37] = ROW("1"),
        [38] = ROW("8"),
        [39] = ROW("16"),
        [40] = ROW("243"),
        [41] = ROW("3"),
        [42] = ROW("-3234"),

        [43] = ROW("2"),
        [44] = ROW("3"),

        [45] = ROW("2"),
        [46] = ROW("3"),
        [47] = ROW("4"),

        [48] = ROW("-2"),
        [49] = ROW("-3"),
        [50] = ROW("-4"),

        [51] = ROW("ab"),
        [52] = ROW("1234567890abcdefghijklmnopqrstuvwxyz"),
        [53] = ROW("12345678901234567890abcdefghijklmnopqrstuvwxyz"),
        [54] = ROW("abcde"),
        [55] = ROW("abc"),
        [56] = ROW("abcd"),
        [57] = ROW("a"),
        [58] = ROW("b"),
        [59] = ROW(""),
        [60] = ROW(""),
        [61] = ROW(""),

        [62] = ROW("-1"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}
TestFunction(TestFuncsV){
    const char* input =
        "=f(a(1))\n"
        "=f(mod(a(13)))\n"

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
        "=f(cat('a', a('b'), 'c'))\n"
        "=f(cat('a', a('b'), 'c', a('d')))\n"

        "=f(eval(a('pow(2,4)')))\n"
        "=sum(eval(a('pow(2,4)', 'pow(2, 3)', '')))\n"

        "=count(a(1, 0, 'a', ''))\n"
        "=avg(a(2, 4, 6))\n"
        "=min(a(2, 4, 6))\n"
        "=max(a(2, 4, 6))\n"

        "=find(2, a(2, 4, 6))\n"
    ;
    SheetRow expected[] = {
        // Designated initializers are so you can figure out which row
        // when a test fails.
        [ 0] = ROW("1"),
        [ 1] = ROW("1"),

        [ 2] = ROW("13"),
        [ 3] = ROW("13"),
        [ 4] = ROW("14"),
        [ 5] = ROW("13"),

        [ 6] = ROW("-13"),
        [ 7] = ROW("-14"),
        [ 8] = ROW("-13"),
        [ 9] = ROW("-13"),

        [10] = ROW("13"),
        [11] = ROW("13"),
        [12] = ROW("14"),
        [13] = ROW("14"),

        [14] = ROW("-13"),
        [15] = ROW("-14"),
        [16] = ROW("-13"),
        [17] = ROW("-14"),

        [18] = ROW("8"),
        [19] = ROW("81"),

        [20] = ROW("24"),
        [21] = ROW("10"),

        [22] = ROW("11"),
        [23] = ROW("19"),
        [24] = ROW("86"),
        [25] = ROW("82"),

        [26] = ROW("3"),

        [27] = ROW("ab"),
        [28] = ROW("ab"),
        [29] = ROW("ab"),
        [30] = ROW("abc"),
        [31] = ROW("abcd"),

        [32] = ROW("16"),
        [33] = ROW("24"),

        [34] = ROW("3"), // skips the empty string, includes 0
        [35] = ROW("4"),
        [36] = ROW("2"),
        [37] = ROW("6"),

        [38] = ROW("1"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}
TestFunction(TestFuncsRowArray){
    const char* input =
    //   a     b    c       d       e      f      g   h    i    j    k    l    m      n   o   p
        "1  | 13 | 13.1 | -13.1 | 13.5 | -13.5 | 1  | 2  | 3  | 4  | 0  | 1  | 0\n"
        "10 | 11 | 12   | 41    | 42   | 43    | 31 | 32 | 33 | 15 | 25 | 35 | a    | b | c | d\n"
        "pow(2,4) | pow(2,3) |\n"
        "   | 1  | 0    | a     |      | 2     | 4   | 6\n"

        "=f(row('a', 'a', 1)) | =f([a:a,1])\n"
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

        [ 4] = ROW("1", "1"),
        [ 5] = ROW("1"),

        [ 6] = ROW("13"),
        [ 7] = ROW("13"),
        [ 8] = ROW("14"),
        [ 9] = ROW("13"),

        [10] = ROW("-13"),
        [11] = ROW("-14"),
        [12] = ROW("-13"),
        [13] = ROW("-13"),

        [14] = ROW("13"),
        [15] = ROW("13"),
        [16] = ROW("14"),
        [17] = ROW("14"),

        [18] = ROW("-13"),
        [19] = ROW("-14"),
        [20] = ROW("-13"),
        [21] = ROW("-14"),

        [22] = ROW("8"),
        [23] = ROW("81"),

        [24] = ROW("24"),
        [25] = ROW("10"),

        [26] = ROW("11"),
        [27] = ROW("19"),
        [28] = ROW("86"),
        [29] = ROW("82"),

        [30] = ROW("3"),

        [31] = ROW("ab"),
        [32] = ROW("ab"),
        [33] = ROW("ab"),
        [34] = ROW("abc"),
        [35] = ROW("abcd"),

        [36] = ROW("16"),
        [37] = ROW("24"),

        [38] = ROW("3"), // skips the empty string, includes 0
        [39] = ROW("4"),
        [40] = ROW("2"),
        [41] = ROW("6"),

        [42] = ROW("1"),

        [43] = ROW("3"),
        [44] = ROW("7"),
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
        "Plate   | 5 | Plate | =tlu([c,1], [a], [b])\n"
        "Chain   | 3 |       | =tlu([c,2], [a], [b], 2)\n"
        "Leather | 1 | =     | =tlu([c,3], [a], [b], 4)\n"
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
        "Plate   | 5 | Plate | =tlu([c,$], [a], [b])\n"
        "Chain   | 3 |       | =tlu([c,$], [a], [b], 2)\n"
        "Leather | 1 | =     | =tlu([c,$], [a], [b], 4)\n"
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
        "=[a,$]\n"
    ;
    SheetRow expected[] = {
        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 1);
}
TestFunction(TestIndirectlyRecursiveShouldError){
    const char* input =
        "=[a,2]\n"
        "=[a,1]\n"
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
    SheetHandle deps[1] = {0};
    int nerr = drsp_evaluate_formulas(ctx, handles[0], deps, arrlen(deps));
    TestExpectEquals(nerr, expected_nerr);
    for(size_t i = 0; i < arrlen(deps); i++){
        // TestAssert(deps[i]);
    }
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

#undef __func__
    return TEST_stats;
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
        "=cell([a,8], 'a', 2)\n"
        "=cell([a,7], 2)\n"
        "=cell('other', 'b', 2)\n"
        "=cell('other', 'b', find(0, [a, :3]))\n"
        "=cell('other', 'b', find('0', [a, :3]))\n"
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
        TestExpectEquals(val.kind, DRSP_RESULT_NUMBER);
        TestExpectEquals(val.d, c->value); // suck it, "always use an epsilon" bots.
    }
    {   // Test that deleting a sheet works (and asan doesn't get mad).
        DrSpreadResult val = {0};
        int err = drsp_evaluate_string(ctx, (SheetHandle)&ms.sheets[0], "[Encumbrance, 1]", sizeof("[Encumbrance, 1]")-1, &val, -1, -1);
        TestAssertFalse(err);
        TestExpectEquals(val.kind, DRSP_RESULT_NUMBER);
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
        SpreadSheet* sheet = &ms.sheets[0];
        int err = drsp_evaluate_formulas(ctx, (SheetHandle)sheet, NULL, 0);
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
    TESTEND();
}



#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __wasm__
#pragma pop_macro("__FILE__")
#endif

#include "drspread.c"
