"use strict";
function drspread(wasm_path, sheet_query_cell_kind, sheet_cell_number, sheet_cell_text_, sheet_col_height, sheet_row_width, sheet_set_display_number, sheet_set_display_string_, sheet_set_display_error, sheet_name_to_col_idx_, sheet_next_cell_, sheet_dims_, sheet_name_to_sheet_) {
    let winst;
    let exports;
    let malloc;
    let reset_memory;
    let sheet_evaluate_formulas;
    let sheet_evaluate_string;
    let mem;
    let mem32;
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
    function js_string_to_wasm(s) {
        const encoded = encoder.encode(s);
        const p = malloc(encoded.length + 4);
        write4(p, encoded.length);
        const sub = mem.subarray(p + 4, p + 4 + encoded.length);
        sub.set(encoded);
        return p;
    }
    function read4(p) {
        return memview.getInt32(p, true);
        p /= 4;
        p |= 0;
        return mem32[p];
    }
    function readdouble(p) {
        const d = memview.getFloat64(p, true);
        return d;
    }
    const imports = {
        env: {
            round: Math.round,
            pow: Math.pow,
            sheet_query_cell_kind,
            sheet_cell_number,
            sheet_cell_text: function (id, row, col) {
                return js_string_to_wasm(sheet_cell_text_(id, row, col));
            },
            sheet_col_height,
            sheet_row_width,
            sheet_set_display_number,
            sheet_set_display_string: function (id, row, col, p, len) {
                const s = wasm_string_to_js(p, len);
                sheet_set_display_string_(id, row, col, s);
            },
            sheet_set_display_error,
            sheet_name_to_col_idx: function (id, p, len) {
                const s = wasm_string_to_js(p, len);
                return sheet_name_to_col_idx_(id, s);
            },
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
            sheet_dims: function (id, pncols, pnrows) {
                const [cols, rows] = sheet_dims_(id);
                write4(pncols, cols);
                write4(pnrows, rows);
                return 0;
            },
            sheet_name_to_sheet: function (id, p, len) {
                const s = wasm_string_to_js(p, len);
                return sheet_name_to_sheet_(id, s);
            },
        },
    };
    function evaluate_formulas(id) {
        const now = window.performance.now();
        reset_memory();
        sheet_evaluate_formulas(id);
        reset_memory();
        const after = window.performance.now();
        console.log('evaluate_formulas', after - now);
    }
    function evaluate_string(id, s) {
        reset_memory();
        const p = malloc(16);
        const err = sheet_evaluate_string(id, js_string_to_wasm(s), p);
        if (err) {
            return "error";
        }
        const kind = read4(p);
        switch (kind) {
            case 0: return "";
            case 1: return readdouble(p + 8);
            case 3: return wasm_string_to_js(read4(p + 12), read4(p + 8));
            default: return "error";
        }
    }
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
        mem32 = new Uint32Array(m.buffer);
        malloc = exports.malloc;
        reset_memory = exports.reset_memory;
        sheet_evaluate_formulas = exports.sheet_evaluate_formulas;
        sheet_evaluate_string = exports.sheet_evaluate_string;
        return { evaluate_formulas, evaluate_string, exports };
    });
}
