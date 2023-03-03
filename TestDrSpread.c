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
static TestFunc TestComplexMultisheet;

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
    RegisterTest(TestComplexMultisheet);
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
test_spreadsheet(const char* caller, const char* input, const SheetRow* expected, size_t expected_len, int expected_nerr){
#define __func__ caller
    struct TestStats TEST_stats = {0};
    SpreadSheet sheet = {0};
    int err = read_csv_from_string(&sheet, input);
    TestAssertFalse(err);
    TestAssertEquals(sheet.rows, expected_len);

    SheetOps ops = sheet_ops();
    int nerr = drsp_evaluate_formulas((SheetHandle)&sheet, &ops, NULL, 0);
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

    nerr = 0;
    for(int i = 0; i < sheet.rows; i++){
        const SheetRow* expected_row = &expected[i];
        const SheetRow* inp_row = &sheet.cells[i];
        TestAssertEquals(inp_row->n, expected_row->n);
        for(int col = 0; col < expected_row->n; col++){
            const char* expected_text = expected_row->data[col];
            StringView expected = {strlen(expected_text), expected_text};
            const char* inp_txt = inp_row->data[col];
            size_t inp_len = inp_row->lengths[col];
            StringView inp = stripped2(inp_txt, inp_len);
            if(!inp.length) continue;
            if(inp.text[0] != '=') continue;
            DrSpreadResult val;
            int err = drsp_evaluate_string((SheetHandle)&sheet, &ops, inp_txt, inp_len, &val, i, col);
            nerr += err;
            if(!err){
                switch(val.kind){
                    case DRSP_RESULT_NULL:
                        TestExpectEquals(expected.length, 0);
                        break;
                    case DRSP_RESULT_NUMBER:{
                        DoubleResult dr = parse_double(expected.text, expected.length);
                        TestAssertFalse(dr.errored);
                        double n = dr.result;
                        TestExpectEquals(n, val.d);
                    }break;
                    case DRSP_RESULT_STRING:{
                        StringView v = {val.s.length, val.s.text};
                        TestExpectEquals2(sv_equals, v, expected);
                        free((char*)val.s.text);
                    } break;
                    default:
                        TestExpectFalse(1);
                        break;
                }
            }
        }
    }
    TestExpectEquals(nerr, expected_nerr);
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
    SheetRow expected[] = {
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
    SheetRow expected[] = {
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
              // NOTE: find returns an OFFSET, not an index
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
        [37] = ROW("8"),
        [38] = ROW("16"),
        [39] = ROW("243"),
        [40] = ROW("3"),
        [41] = ROW("-3234"),

        [42] = ROW("2"),
        [43] = ROW("3"),

        [44] = ROW("2"),
        [45] = ROW("3"),
        [46] = ROW("4"),

        [47] = ROW("-2"),
        [48] = ROW("-3"),
        [49] = ROW("-4"),

        [50] = ROW("ab"),
        [51] = ROW("1234567890abcdefghijklmnopqrstuvwxyz"),
        [52] = ROW("12345678901234567890abcdefghijklmnopqrstuvwxyz"),
        [53] = ROW("abcde"),
        [54] = ROW("abc"),
        [55] = ROW("abcd"),
        [56] = ROW("a"),
        [57] = ROW("b"),
        [58] = ROW(""),
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

        "=_f(cat(_a('a'), _a('b')))\n"
        "=_f(cat('a', _a('b')))\n"
        "=_f(cat(_a('a'), 'b'))\n"
        "=_f(cat('a', _a('b'), 'c'))\n"
        "=_f(cat('a', _a('b'), 'c', _a('d')))\n"

        "=_f(eval(_a('pow(2,4)')))\n"
        "=sum(eval(_a('pow(2,4)', 'pow(2, 3)', '')))\n"
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
TestFunction(TestBugs3){
    const char* input =
        "=[a,$]\n"
    ;
    SheetRow expected[] = {
        ROW("error"),
    };
    return test_spreadsheet(__func__, input, expected, arrlen(expected), 1);
}
TestFunction(TestBugs4){
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
    SheetHandle deps[1] = {0};
    int nerr = drsp_evaluate_formulas((SheetHandle)&collection.sheets[0], &ops, deps, arrlen(deps));
    TestExpectEquals(nerr, expected_nerr);
    for(size_t i = 0; i < arrlen(deps); i++){
        TestAssert(deps[i]);
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
    nerr = 0;
    for(int i = 0; i < collection.sheets[0].rows; i++){
        const SheetRow* expected_row = &expected[i];
        const SheetRow* inp_row = &collection.sheets[0].cells[i];
        TestAssertEquals(inp_row->n, expected_row->n);
        for(int col = 0; col < expected_row->n; col++){
            const char* expected_text = expected_row->data[col];
            StringView expected = {strlen(expected_text), expected_text};
            const char* inp_txt = inp_row->data[col];
            size_t inp_len = inp_row->lengths[col];
            StringView inp = stripped2(inp_txt, inp_len);
            if(!inp.length) continue;
            if(inp.text[0] != '=') continue;
            DrSpreadResult val;
            int err = drsp_evaluate_string((SheetHandle)&collection.sheets[0], &ops, inp_txt, inp_len, &val, i, col);
            nerr += err;
            if(!err){
                switch(val.kind){
                    case DRSP_RESULT_NULL:
                        TestExpectEquals(expected.length, 0);
                        break;
                    case DRSP_RESULT_NUMBER:{
                        DoubleResult dr = parse_double(expected.text, expected.length);
                        TestAssertFalse(dr.errored);
                        double n = dr.result;
                        TestExpectEquals(n, val.d);
                    }break;
                    case DRSP_RESULT_STRING:{
                        StringView v = {val.s.length, val.s.text};
                        TestExpectEquals2(sv_equals, v, expected);
                        free((char*)val.s.text);
                    } break;
                    default:
                        TestExpectFalse(1);
                        break;
                }
            }
        }
    }
    TestExpectEquals(nerr, expected_nerr);

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
    int err = read_multi_csv_from_string(&ms, input);
    TestAssertFalse(err);
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
        { SV("tlu('Frodo',   [Character], [Encumbrance2])"),  3.2},
        { SV("tlu('Strider', [Character], [Encumbrance2])"),  8. },
        { SV("sum([Encumbrance2])"),                         13.2},
    };
    SpreadSheet* sheet = &ms.sheets[0];
    for(size_t i = 0; i < arrlen(cases); i++){
        struct test_case* c = &cases[i];
        DrSpreadResult val = {0};
        StringView sv = c->sv;
        err = drsp_evaluate_string((SheetHandle)sheet, &ops, sv.text, sv.length, &val, -1, -1);
        TestExpectFalse(err);
        TestExpectEquals(val.kind, DRSP_RESULT_NUMBER);
        TestExpectEquals(val.d, c->value); // suck it, "always use an epsilon" bots.
    }
    err = drsp_evaluate_formulas((SheetHandle)sheet, &ops, NULL, 0);
    TestExpectEquals(err, 0);
    StringView expected[] = {
        SV("2"),
        SV("3.2"),
        SV("8"),
    };
    for(size_t i = 0; i < arrlen(expected); i++){
        const SheetRow* disp = &sheet->display[i];
        TestAssertEquals(disp->n, 3);
        StringView weight = {strlen(disp->data[1]), disp->data[1]};
        TestExpectEquals2(sv_equals, weight, expected[i]);
        weight = (StringView){strlen(disp->data[2]), disp->data[2]};
        TestExpectEquals2(sv_equals, weight, expected[i]);
    }

    TESTEND();
}



#pragma clang diagnostic pop

#include "drspread.c"
