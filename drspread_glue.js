"use strict";
function drspread(wasm_path, sheet_set_display_number, sheet_set_display_string_, sheet_set_display_error, sheet_next_cell_) {
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
            logline: function (p, l) {
                const len = exports.strlen(p);
                const s = wasm_string_to_js(p, len);
                console.trace(`${s}: ${l}`);
            },
            logsz: function (sz) {
                console.trace(sz);
            },
            round: (num) => {
                return Math.sign(num) * Math.round(Math.abs(num));
            },
            abort: () => {
                throw new Error();
            },
            pow: Math.pow,
            sheet_set_display_number,
            sheet_set_display_string: function (id, row, col, p, len) {
                const s = wasm_string_to_js(p, len);
                sheet_set_display_string_(id, row, col, s);
            },
            sheet_set_display_error,
            sheet_next_cell: function (id, i, prow, pcol) {
                const prev_row = read4(prow);
                const prev_col = read4(pcol);
                const [r, c] = sheet_next_cell_(id, i, prev_row, prev_col);
                if (r == 4294967295 && c == 4294967295)
                    return 1;
                if (r == -1 && c == -1)
                    return 1;
                write4(prow, r);
                write4(pcol, c);
                return 0;
            },
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
                evaluate_formulas: (sheet) => {
                    const ctx = result.id;
                    const handles = exports.wasm_deps_buff.value;
                    exports.drsp_evaluate_formulas(ctx, sheet, handles, 1024);
                    const deps = [];
                    for (let i = 0; i < 1024; i++) {
                        const hnd = read4(handles + i * 4);
                        if (!hnd)
                            break;
                        if (hnd == sheet)
                            continue;
                        deps.push(hnd);
                    }
                    return deps;
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
                    exports.drsp_set_cell_str(ctx, sheet, row, col, exports.wasm_str_buff.value, encoded.length);
                },
                make_sheet: (sheet, name) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(name);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    exports.drsp_set_sheet_name(ctx, sheet, exports.wasm_str_buff.value, encoded.length);
                },
                set_sheet_alias: (sheet, name) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(name);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    exports.drsp_set_sheet_alias(ctx, sheet, exports.wasm_str_buff.value, encoded.length);
                },
                set_col_name: (sheet, idx, name) => {
                    const ctx = result.id;
                    const encoded = encoder.encode(name);
                    mem.set(encoded, exports.wasm_str_buff.value);
                    exports.drsp_set_col_name(ctx, sheet, idx, exports.wasm_str_buff.value, encoded.length);
                },
                del_sheet: (sheet) => {
                    exports.drsp_del_sheet(result.id, sheet);
                },
            };
            return result;
        }
        return { exports, make_ctx: create_ctx };
    });
}
