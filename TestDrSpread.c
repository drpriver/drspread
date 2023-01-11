//
// Copyright © 2023, David Priver
//
#include "drspread.h"
#include "spreadsheet.h"
#include "testing.h"

static TestFunc TestSpreadsheet1;
static TestFunc TestSpreadsheet2;
static TestFunc TestBinOps;
static TestFunc TestUnOps;
static TestFunc TestFuncs;
static TestFunc TestMod;

int main(int argc, char** argv){
    RegisterTest(TestSpreadsheet1);
    RegisterTest(TestSpreadsheet2);
    RegisterTest(TestBinOps);
    RegisterTest(TestUnOps);
    RegisterTest(TestFuncs);
    RegisterTest(TestMod);
    int ret = test_main(argc, argv, NULL);
    return ret;
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#pragma clang diagnostic ignored "-Wgnu-auto-type"

#ifndef arrlen
#define arrlen(x) (sizeof(x)/sizeof(x[0]))
#endif

static inline
_Bool
streq(const char* a, const char* b){
    return strcmp(a,b) == 0;
}
#define ROW(...) (\
        (struct Row){ \
            arrlen(((const char*[]){__VA_ARGS__})),  \
            ((const char*[]){__VA_ARGS__}), \
        })


// We just leak the spreadsheets, it's really not that much data
static
struct
TestStats
test_spreadsheet(const char* caller, const char* input, const struct Row* expected, size_t expected_len, int expected_nerr){
#define __func__ caller
    struct TestStats TEST_stats = {0};
    SpreadSheet sheet = {0};
    int err = read_csv_from_string(&sheet, input);
    TestAssertFalse(err);
    TestAssertEquals(sheet.rows, expected_len);

    SheetOps ops = {
        .ctx = &sheet,
        .next_cell=&next,
        .cell_txt=&txt,
        .set_display_number=&display_number,
        .set_display_error=&display_error,
        .set_display_string=&display_string,
        .name_to_col_idx=&get_name_to_col_idx,
        .query_cell_kind=&cell_kind,
        .cell_number=&cell_number,
        .row_width=&get_row_width,
        .col_height=&get_col_height,
        .dims=&get_dims,
    };
    int nerr = drsp_evaluate_formulas(&ops);
    TestExpectEquals(nerr, expected_nerr);
    for(size_t i = 0; i < expected_len; i++){
        const struct Row* display_row = &sheet.display[i];
        const struct Row* expected_row = &expected[i];
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
#undef __func__
    return TEST_stats;
}



TestFunction(TestSpreadsheet1){
    const char* input =
        "=sum([c])                     | Axe         | 10\n"
        "=find('Food', [b])            | Torch       | 1\n"
        "=tlu('Plate Armor', [b], [c]) | Plate Armor | 50\n"
        "=min([c, :2])                 | Food        | 1 per potato\n"
    ;
    struct Row expected[] = {
        ROW("61", "Axe",         "10"),
        ROW( "3", "Torch",        "1"),
        ROW("50", "Plate Armor", "50"),
        ROW( "1", "Food",        "1 per potato"),
        ROW(""),
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
    struct Row expected[] = {
        ROW("2", "2", "3", "36"),
        ROW("4", "6", "4",  "4"),
        ROW("2", "-1", "3", "36"),
        ROW("4", "4",  "-16", "24"),
        ROW("12", "100", "111"),
        ROW(""),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestBinOps){
    const char* input =
        "=1+1   | =1-1    | =4/2       | =3*4 \n"
        "=2 = 1 | =2 == 1 | =2 != 1\n"
        "=1 < 2 | =1 > 2  | =1 >= 1    | =1 <= 2\n"
    ;
    struct Row expected[] = {
        ROW("2", "0", "2", "12"),
        ROW("0", "0", "1"),
        ROW("1", "0", "1", "1"),
        ROW(""),
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
    struct Row expected[] = {
        ROW("0", "-2", "2"),
        ROW("1",  "0", "0"),
        ROW("0", "-1", "1"),
        ROW("0", "-1", "1"),
        ROW("error", "error", "error"),
        ROW(""),
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
    ;
    struct Row expected[] = {
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
        // NOTE: find returns an OFFSET, not an index
        ROW("2", ""),
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
    struct Row expected[] = {
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
        ROW(""),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

#pragma clang diagnostic pop

#include "drspread.c"
