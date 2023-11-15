"use strict";
function drspread(wasm_path, sheet_set_display_number, sheet_set_display_string_, sheet_set_display_error) {
    let winst;
    let exports;
    let mem;
    let memview;
    const decoder = new TextDecoder();
    const encoder = new TextEncoder();
    function wasm_string_to_js(p, len) {
        const sub = mem.subarray(p, p + len);
        const text = decoder.decode(sub);
        return text;
    }
    function write4(p, val) {
        memview.setInt32(p, val, true);
    }
    function read4(p) {
        return memview.getInt32(p, true);
    }
    function readdouble(p) {
        const d = memview.getFloat64(p, true);
        return d;
    }
    const imports = {
        env: {
            round: (num) => {
                return Math.sign(num) * Math.round(Math.abs(num));
            },
            abort: () => {
                throw new Error();
            },
            log: Math.log,
            pow: Math.pow,
            sheet_set_display_number,
            sheet_set_display_string: function (id, row, col, p, len) {
                const s = wasm_string_to_js(p, len);
                sheet_set_display_string_(id, row, col, s);
            },
            sheet_set_display_error,
        },
    };
    return fetch(wasm_path)
        .then(response => response.arrayBuffer())
        .then(bytes => WebAssembly.instantiate(bytes, imports))
        .then(x => {
        winst = x.instance;
        exports = winst.exports;
        const m = exports.memory;
        m.grow(1024);
        mem = new Uint8Array(m.buffer);
        memview = new DataView(mem.buffer);
        function create_ctx() {
            const result = {
                id: exports.drsp_create_ctx(),
                evaluate_formulas: () => {
                    const ctx = result.id;
                    const e = exports.drsp_evaluate_formulas(ctx);
                    return e;
                },
                evaluate_string: (sheet, s) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(s);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    const err = exports.drsp_evaluate_string(ctx, sheet, exports.wasm_str_buff.value, encoded.length, exports.wasm_result.value, -1, -1);
                    if (err)
                        return "error";
                    const kind = read4(exports.wasm_result.value);
                    let v = "error";
                    switch (kind) {
                        case 0:
                            v = "";
                            break;
                        case 1:
                            v = readdouble(exports.wasm_result.value + 8);
                            break;
                        case 2:
                            v = wasm_string_to_js(read4(exports.wasm_result.value + 12), read4(exports.wasm_result.value + 8));
                            break;
                        default: break;
                    }
                    return v;
                },
                set_str: (sheet, row, col, s) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(s);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    const e = exports.drsp_set_cell_str(ctx, sheet, row, col, exports.wasm_str_buff.value, encoded.length);
                    return e;
                },
                set_extra_str: (sheet, id, s) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(s);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    const e = exports.drsp_set_extra_dimensional_str(ctx, sheet, id, exports.wasm_str_buff.value, encoded.length);
                    return e;
                },
                make_sheet: (sheet, name) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(name);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    const e = exports.drsp_set_sheet_name(ctx, sheet, exports.wasm_str_buff.value, encoded.length);
                    return e;
                },
                set_sheet_alias: (sheet, name) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(name);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    const e = exports.drsp_set_sheet_alias(ctx, sheet, exports.wasm_str_buff.value, encoded.length);
                    return e;
                },
                set_col_name: (sheet, idx, name) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(name);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    const e = exports.drsp_set_col_name(ctx, sheet, idx, exports.wasm_str_buff.value, encoded.length);
                    return e;
                },
                del_sheet: (sheet) => {
                    const e = exports.drsp_del_sheet(result.id, sheet);
                    return e;
                },
                set_flags: (sheet, flags) => {
                    const e = exports.drsp_set_sheet_flags(result.id, sheet, flags);
                    return e;
                },
                get_flags: (sheet) => {
                    return exports.drsp_get_sheet_flags(result.id, sheet);
                },
                set_flag: (sheet, flag, on) => {
                    const e = exports.drsp_set_sheet_flag(result.id, sheet, flag, on | 0);
                    return e;
                },
                clear_func_params: (func) => {
                    const ctx = result.id;
                    const e = exports.drsp_clear_function_params(ctx, func);
                    return e;
                },
                set_func_output: (func, row, col) => {
                    const ctx = result.id;
                    const e = exports.drsp_set_function_output(ctx, func, row, col);
                    return e;
                },
                set_func_params: (func, rows, cols) => {
                    const ctx = result.id;
                    if (rows.length != cols.length)
                        return 1;
                    const n = rows.length;
                    const rowbuff = exports.wasm_parambuff_row.value;
                    const colbuff = exports.wasm_parambuff_col.value;
                    for (let i = 0; i < n; i++) {
                        write4(rowbuff + i * 4, rows[i]);
                        write4(colbuff + i * 4, cols[i]);
                    }
                    const e = exports.drsp_set_function_params(ctx, func, n, rowbuff, colbuff);
                    return e;
                },
            };
            return result;
        }
        return { exports, make_ctx: create_ctx };
    });
}
