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
static TestFunc TestFuncsV;
static TestFunc TestMod;
static TestFunc TestBugs;
static TestFunc TestBugs2;
static TestFunc TestBugs3;
static TestFunc TestBugs4;
static TestFunc TestMultisheet;
static TestFunc TestColFunc;
static TestFunc TestRanges;
static TestFunc TestBadRanges;
static TestFunc TestNames;

int main(int argc, char** argv){
    RegisterTest(TestRanges);
    RegisterTest(TestBadRanges);
    RegisterTest(TestSpreadsheet1);
    RegisterTest(TestSpreadsheet2);
    RegisterTest(TestBinOps);
    RegisterTest(TestUnOps);
    RegisterTest(TestFuncs);
    RegisterTest(TestFuncsV);
    RegisterTest(TestMod);
    RegisterTest(TestBugs);
    RegisterTest(TestBugs2);
    RegisterTest(TestBugs3);
    RegisterTest(TestBugs4);
    RegisterTest(TestMultisheet);
    RegisterTest(TestColFunc);
    RegisterTest(TestNames);
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

static inline
_Bool
streq2(const char* a, const char* b, size_t blen){
    StringView l = {strlen(a), a};
    StringView r = {blen, b};
    return sv_equals(l, r);
}


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
        .ctx = NULL,
        .next_cell=&next,
        .cell_txt=&txt,
        .set_display_number=&display_number,
        .set_display_error=&display_error,
        .set_display_string=&display_string,
        .name_to_col_idx=&get_name_to_col_idx,
        .row_width=&get_row_width,
        .col_height=&get_col_height,
        .dims=&get_dims,
    };
    int nerr = drsp_evaluate_formulas((SheetHandle)&sheet, &ops, NULL, 0);
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

#define ROW(...) { \
    arrlen(((const char*[]){__VA_ARGS__})),  \
    ((const char*[]){__VA_ARGS__}), \
    ((size_t[]){0}), \
}

TestFunction(TestRanges){
    const char* input =
        " 0 | =[a,1]\n"
        " 1 | =['a', 1]\n"
        " 2 | =['a', $]\n"
        " 3 | =[\"$\", 3]\n"
        " 4 | =['$', 3]\n"
        " 5 | =sum([a])\n"
        " 6 | =sum(['a', :])\n"
        " 7 | =sum(['a', 1:2])\n"
        " 8 | =sum(['a', :2])\n"
        " 9 | =sum(['a', 2:])\n"
        "10 | =sum([a, 2:])\n"
    ;
    struct Row expected[] = {
        ROW("0", "0"),
        ROW("1", "0"),
        ROW("2", "2"),
        ROW("3", "2"),
        ROW("4", "2"),
        ROW("5", "55"),
        ROW("6", "55"),
        ROW("7", "1"),
        ROW("8", "1"),
        ROW("9", "55"),
        ROW("10", "55"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestBadRanges){
    const char* input =
        " 0 | =[a,]\n"
        " 1 | =[1]\n"
        " 2 | =[$]\n"
        " 3 | =[]\n"
        " 4 | =['$', '3', '3']\n"
        " 5 | =sum(['a])\n"
        " 6 | =sum(['a', '3':'3'])\n"
        " 7 | =sum([3, 1:2])\n"
    ;
    struct Row expected[] = {
        ROW( "0", "error"),
        ROW( "1", "error"),
        ROW( "2", "error"),
        ROW( "3", "error"),
        ROW( "4", "error"),
        ROW( "5", "error"),
        ROW( "6", "error"),
        ROW( "7", "error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 8);
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
    struct Row expected[] = {
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
    struct Row expected[] = {
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
    struct Row expected[] = {
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
        ROW("3", ""),
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
        ROW("b"),
        ROW("8"),
        ROW("16"),
        ROW("243"),
        ROW("3"),
        ROW("-3234"),

        ROW("2"),
        ROW("3"),

        ROW("2"),
        ROW("3"),
        ROW("4"),

        ROW("-2"),
        ROW("-3"),
        ROW("-4"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}
TestFunction(TestFuncsV){
    const char* input =
        "=_f(_a(1))\n"
        "=_f(mod(_a(13)))\n"

        "=_f(trunc(_a(13.1)))\n"
        "=_f(floor(_a(13.1)))\n"
        "=_f(ceil( _a(13.1)))\n"
        "=_f(round(_a(13.1)))\n"

        "=_f(trunc(_a(-13.1)))\n"
        "=_f(floor(_a(-13.1)))\n"
        "=_f(ceil( _a(-13.1)))\n"
        "=_f(round(_a(-13.1)))\n"

        "=_f(trunc(_a(13.5)))\n"
        "=_f(floor(_a(13.5)))\n"
        "=_f(ceil( _a(13.5)))\n"
        "=_f(round(_a(13.5)))\n"

        "=_f(trunc(_a(-13.5)))\n"
        "=_f(floor(_a(-13.5)))\n"
        "=_f(ceil( _a(-13.5)))\n"
        "=_f(round(_a(-13.5)))\n"

        "=_f(pow(_a(2), 3))\n"
        "=_f(pow(_a(3), _a(4)))\n"

        "=prod(_a(1, 2, 3, 4))\n"
        "=sum(_a(1, 2, 3, 4))\n"

        "=sum(if(_a(0,1,0), 3, 4))\n"
        "=sum(if(_a(0,1,0), _a(10, 11, 12), 4))\n"
        "=sum(if(_a(0,1,0), 2, _a(41, 42, 43)))\n"
        "=sum(if(_a(0,1,0), _a(31, 32, 33), _a(15, 25, 35)))\n"

        "=sum(num(_a('', 1), 2))\n"
    ;
    struct Row expected[] = {
        ROW("1"),
        ROW("1"),

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
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 0);
}

TestFunction(TestBugs){
    const char* input =
        "Plate   | 5 | Plate | =tlu([c,1], [a], [b])\n"
        "Chain   | 3 |       | =tlu([c,2], [a], [b], 2)\n"
        "Leather | 1 | =     | =tlu([c,3], [a], [b], 4)\n"
    ;
    struct Row expected[] = {
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
    struct Row expected[] = {
        ROW("Plate",   "5", "Plate", "5"),
        ROW("Chain" ,  "3", "",      "2"),
        ROW("Leather", "1", "error", "error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 2);
}
TestFunction(TestBugs3){
    const char* input =
        "=[a,$]\n"
    ;
    struct Row expected[] = {
        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 1);
}
TestFunction(TestBugs4){
    const char* input =
        "=[a,2]\n"
        "=[a,1]\n"
    ;
    struct Row expected[] = {
        ROW("error"),
        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 2);
}

struct SheetCollection {
    SpreadSheet sheets[2];
    const char* names[2];
};

static
void*_Nullable
collection_name_to_sheet(void* ctx, const char* name, size_t len){
    (void)len;
    struct SheetCollection* collection = ctx;
    if(streq2(collection->names[0], name, len))
        return &collection->sheets[0];
    if(streq2(collection->names[1], name, len))
        return &collection->sheets[1];
    return NULL;
}

static
struct
TestStats
test_multi_spreadsheet(const char* caller, const char* name1, const char* input1, const char* name2, const char* input2, const struct Row* expected, size_t expected_len, int expected_nerr){
#define __func__ caller
    struct TestStats TEST_stats = {0};
    struct SheetCollection collection = {0};
    int err;
    err = read_csv_from_string(&collection.sheets[0], input1);
    TestAssertFalse(err);
    collection.names[0] = name1;
    err = read_csv_from_string(&collection.sheets[1], input2);
    TestAssertFalse(err);
    collection.names[1] = name2;
    TestAssertEquals(collection.sheets[0].rows, expected_len);

    SheetOps ops = {
        .ctx = &collection,
        .next_cell=&next,
        .cell_txt=&txt,
        .set_display_number=&display_number,
        .set_display_error=&display_error,
        .set_display_string=&display_string,
        .name_to_col_idx=&get_name_to_col_idx,
        .row_width=&get_row_width,
        .col_height=&get_col_height,
        .dims=&get_dims,
        .name_to_sheet = &collection_name_to_sheet,
    };
    SheetHandle deps[1] = {0};
    int nerr = drsp_evaluate_formulas((SheetHandle)&collection.sheets[0], &ops, deps, arrlen(deps));
    TestExpectEquals(nerr, expected_nerr);
    for(size_t i = 0; i < arrlen(deps); i++){
        TestAssert(deps[i]);
    }
    for(size_t i = 0; i < expected_len; i++){
        const struct Row* display_row = &collection.sheets[0].display[i];
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
    struct Row expected[] = {
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
    struct Row expected[] = {
        ROW("0"),
        ROW("3"),
        ROW("3"),
        ROW("4"),
        ROW("2"),
        ROW("error"),
        ROW("a"),
        ROW("other","1", "4"),
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
    struct Row expected[] = {
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



#pragma clang diagnostic pop

#include "drspread.c"
