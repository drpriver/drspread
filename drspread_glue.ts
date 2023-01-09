const enum CellKind {
    CELL_EMPTY = 0, // Empty Cell
    CELL_NUMBER = 1,
    CELL_FORMULA = 2,
    CELL_OTHER = 3,
}
function drspread(
    wasm_path:string,
    sheet_cell_kind:(id:number, row:number, col:number)=>CellKind,
    sheet_cell_number:(id:number, row:number, col:number)=>number,
    sheet_cell_text_:(id:number, row:number, col:number) => string,
    sheet_col_height:(id:number, col:number)=>number,
    sheet_row_width:(id:number, row:number)=>number,
    sheet_set_display_number:(id:number, row:number, col:number, val:number)=>void,
    sheet_set_display_string_:(id:number, row:number, col:number, s:string)=>void,
    sheet_set_display_error:(id:number, row:number, col:number)=>void,
    sheet_name_to_col_idx_:(id:number, s:string) => number,
    sheet_next_cell_:(id:number, prev_row:number, prev_col:number)=>[number, number],
    sheet_dims_:(id:number)=>[number, number],
):Promise<{
    evaluate_formulas: (id: number) => void; 
    evaluate_string: (id: number, s: string) => number; 
    exports: WebAssembly.Exports;
}>
{

let winst: WebAssembly.Instance;
let exports: WebAssembly.Exports;
let malloc: (sz:number)=>number;
let reset_memory:()=>void;
let sheet_evaluate_formulas:(id:number)=>void;
let sheet_evaluate_string:(id:number, p:number)=>number;
let mem: Uint8Array;
let mem32: Uint32Array;
const decoder = new TextDecoder();
const encoder = new TextEncoder();
function wasm_string_to_js(p:number, len:number):string{
    const sub = mem.subarray(p, p+len);
    const text = decoder.decode(sub);
    return text;
}
function write4(p:number, val:number):void{
    p /= 4;
    p |= 0;
    mem32[p] = val;
}

function js_string_to_wasm(s:string):number{
    const encoded = encoder.encode(s);
    const p = malloc(encoded.length+4);
    write4(p, encoded.length);
    const sub = mem.subarray(p+4, p+4+encoded.length);
    sub.set(encoded);
    return p;
}
function read4(p:number):number{
    p /= 4;
    p |= 0;
    return mem32[p];
}
const imports = {
    env:{
        round: Math.round,
        sheet_cell_kind,
        sheet_cell_number,
        sheet_cell_text:function(id:number, row:number, col:number):number{
            return js_string_to_wasm(sheet_cell_text_(id, row, col));
        },
        sheet_col_height,
        sheet_row_width,
        sheet_set_display_number,
        sheet_set_display_string:function(id:number, row:number, col:number, p:number, len:number):void{
            const s = wasm_string_to_js(p, len);
            sheet_set_display_string_(id, row, col, s);
        },
        sheet_set_display_error,
        sheet_name_to_col_idx:function(id:number, p:number, len:number):number{
            const s = wasm_string_to_js(p, len);
            return sheet_name_to_col_idx_(id, s);
        },
        sheet_next_cell:function(id:number, prow:number, pcol:number):number{
            const prev_row = read4(prow);
            const prev_col = read4(pcol);
            const [r, c] = sheet_next_cell_(id, prev_row, prev_col);
            if(r ==4294967295 && c == 4294967295)
                return 1;
            write4(prow, r);
            write4(pcol, c);
            return 0;
        },
        sheet_dims:function(id:number, pncols:number, pnrows:number):number{
            const [cols, rows] = sheet_dims_(id);
            write4(pncols, cols);
            write4(pnrows, rows);
            return 0;
        },
    },
};

function evaluate_formulas(id:number){
    const now = window.performance.now();
    reset_memory();
    sheet_evaluate_formulas(id);
    reset_memory();
    const after = window.performance.now();
    console.log('evaluate_formulas', after-now);
}

function evaluate_string(id:number, s:string):number{
    const now = window.performance.now();
    reset_memory();
    const result = sheet_evaluate_string(id, js_string_to_wasm(s));
    reset_memory();
    const after = window.performance.now();
    console.log('evaluate_string', after-now);
    return result;
}

return fetch(wasm_path)
    .then(response => response.arrayBuffer())
    .then(bytes => WebAssembly.instantiate(bytes, imports))
    .then(x=>{
        winst = x.instance;
        exports = winst.exports;
        const m = exports.memory as WebAssembly.Memory;
        m.grow(1024);
        mem = new Uint8Array(m.buffer);
        mem32 = new Uint32Array(m.buffer);
        malloc = exports.malloc as any;
        reset_memory = exports.reset_memory as any;
        sheet_evaluate_formulas = exports.sheet_evaluate_formulas as any;
        sheet_evaluate_string = exports.sheet_evaluate_string as any;
        return {evaluate_formulas, evaluate_string, exports};
    });
}
