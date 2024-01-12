//
// Copyright © 2023-2024, David Priver <david@davidpriver.com>
//
let func_cells: Array<Array<string|number>> = [['', ''], ['', ''], ['', '']];
let extra_cells: Array<string|number> = ['=1+1', '', ''];
declare let cells: Array<Array<string|number>>;
const column_names = 'abcdefghijklmnopqrstuvwxyz';
let func_display: Array<Array<string>> = [
];
let extra_display: Array<string> = [];
let display:Array<Array<string>> = [
];
function prep():void{
    func_display = [];
    for(let r = 0; r < func_cells.length; r++){
        const row = func_cells[0];
        const d: Array<string> = [];
        func_display.push(d);
        for(let c = 0; c < row.length; c++){
            const val = row[c];
            if(typeof val === 'string' && val[0] == '='){
                d.push('');
            }
            else {
                d.push(''+val);
            }
        }
    }
    display = [];
    for(let r = 0; r < cells.length; r++){
        const row = cells[r];
        const d:Array<string> = [];
        display.push(d);
        for(let c = 0; c < row.length; c++){
            const val = row[c];
            if(typeof val === 'string' && val[0] == '='){
                d.push('');
            }
            else {
                d.push(''+val);
            }
        }
    }
    extra_display = [];
    for(let c = 0; c < extra_cells.length; c++){
        const val = extra_cells[c];
        if(typeof val === 'string' && val[0] == '=')
            extra_display.push('');
        else
            extra_display.push(''+val);
    }
}
prep();

function display_number(i:number, row:number, col:number, val:number):void{
    if(i == 0){
        if(row == DRSP.IDX_EXTRA_DIMENSIONAL)
            extra_display[col] = '' + val;
        else if((val | 0) == val)
            display[row][col] = ""+val;
        else
            display[row][col] = val.toFixed(2);
    }
    else{
        if((val | 0) == val)
            func_display[row][col] = ""+val;
        else
            func_display[row][col] = val.toFixed(2);
    }
}
function display_string(i:number, row:number, col:number, val:string):void{
    if(i == 0){
        if(row == DRSP.IDX_EXTRA_DIMENSIONAL)
            extra_display[col] = val;
        else
            display[row][col] = val;
    }
    else {
        func_display[row][col] = val;
    }
}
function display_error(i:number, row:number, col:number):void{
    if(i == 0){
        if(row == DRSP.IDX_EXTRA_DIMENSIONAL){
            console.log({i, row, col});
            extra_display[col] = 'err';
        }
        else
            display[row][col] = 'err';
    }
    else {
        func_display[row][col] = 'err';
    }
}

let table:HTMLTableElement;
let functable:HTMLTableElement;
let extratable: HTMLTableElement;
let raw:HTMLTableElement;
let pre:HTMLPreElement;
let ex:DrSpreadExports;
let mk_ctx: () => DrSpreadCtx;
let ctx: DrSpreadCtx | undefined;
function get_ctx(): DrSpreadCtx{
    if(ctx) return ctx;
    // ex.reset_memory();
    ctx = mk_ctx();
    ctx.make_sheet(0, '$this');
    for(let i = 0; i < cells.length; i++){
        for(let j = 0; j < cells[i].length; j++){
            const s = cells[i][j];
            ctx.set_str(0, i, j, ""+s);
        }
    }
    for(let i = 0; i < extra_cells.length; i++){
        const s = extra_cells[i];
        ctx.set_extra_str(0, i, ''+s);
    }
    ctx.make_sheet(1, 'func');
    ctx.set_flag(1, DrspSheetFlags.IS_FUNCTION, true);
    for(let i = 0; i < func_cells.length; i++){
        for(let j = 0; j < func_cells[i].length; j++){
            const s = func_cells[i][j];
            ctx.set_str(1, i, j, ""+s);
        }
    }
    ctx.set_func_params(1, [0, 0], [0, 1]);
    ctx.set_func_output(1, func_cells.length-1, 0);
    return ctx;
}

function process(v:string):void{
  const n = get_ctx().evaluate_string(0, v);
  pre.textContent += '\n> '+v;
  pre.textContent += '\n' + n;
}

function make_elems():void{
    const d = document.createElement('div');
    const showraw = document.createElement('button');
    showraw.textContent = 'Show Raw';
    showraw.onclick = ()=>{
        table.style.display = 'none';
        raw.style.display = 'table';
    };
    const showcalc = document.createElement('button');
    showcalc.onclick = ()=>{
        raw.style.display = 'none';
        table.style.display = 'table';
    };
    showcalc.textContent = 'Show Result';
    document.body.appendChild(d);
    d.appendChild(showraw);
    d.appendChild(showcalc);

    pre = document.createElement('pre');
    document.body.appendChild(pre);
    const input = document.createElement('input');
    input.spellcheck = false;
    document.body.appendChild(input);

    raw = document.createElement('table');
    document.body.appendChild(raw);
    raw.style.display = 'none';


    functable = document.createElement('table');
    document.body.appendChild(functable);
    const h = document.createElement('h4');
    h.textContent = 'Summary Cells';
    document.body.appendChild(h);
    extratable = document.createElement('table');
    document.body.appendChild(extratable);
    table = document.createElement('table');
    document.body.appendChild(table);

    input.onkeydown = function(e){
        if(e.key == 'Enter'){
            e.preventDefault();
            e.stopPropagation();
            let v = input.value.trim();
            if(!v) return;
            process(v);
            input.value = '';
        }
    }
}

function show():void{
    {
    let html = '<thead>';
    html  += '<tr>\n  <th>\n';
    for(let i = 0; i < func_cells[0].length; i++){
        const col = column_names[i];
        html += '  <th>'+col+'\n';
    }
    html += '</tr>';
    html += '</thead>'

    for(let r = 0; r < func_display.length; r++){
        let row = func_display[r];
        html += '<tr>\n';
        html += '  <td>'+(r+1)+'\n';
        for(let i = 0; i < row.length; i++){
            let c = row[i];
            if(c == 'err')
                html += `  <td class=error data-row=${r} data-col=${i} data-func=1 ${!r?'class="input"':i==0&&r==func_display.length-1?'class=output':''}>`+c+'\n';
            else
                html += `  <td data-row=${r} data-col=${i} data-func=1 ${!r?'class="input"':i==0&&r==func_display.length-1?'class=output':''}>`+c+'\n';
        }
        html += '</tr>\n';
    }
    functable.innerHTML = html;
    }
    {
    let html = '<tr>';
    for(let i = 0; i < extra_display.length; i++){
        const val = extra_display[i];
        if(val == 'err')
            html += `  <td class=error data-row=-1 data-col=${i}>`+val + '\n';
        else
            html += `  <td data-row=-1 data-col=${i}>`+val + '\n';
    }
    html += '</tr>';
    extratable.innerHTML = html;
    }
    {
    let html = '<thead>';
    html  += '<tr>\n  <th>\n';
    for(let i = 0; i < cells[0].length; i++){
        const col = column_names[i];
        html += '  <th>'+col+'\n';
    }
    html += '</tr>';
    html += '</thead>'

    for(let r = 0; r < display.length; r++){
        let row = display[r];
        html += '<tr>\n';
        html += '  <td>'+(r+1)+'\n';
        for(let i = 0; i < row.length; i++){
            let c = row[i];
            if(c == 'err')
                html += `  <td class=error data-row=${r} data-col=${i}>`+c+'\n';
            else
                html += `  <td data-row=${r} data-col=${i}>`+c+'\n';
        }
        html += '</tr>\n';
    }
    table.innerHTML = html;
    }
    let html = '<thead>';
    html  += '<tr>\n  <th>\n';
    for(let i = 0; i < cells[0].length; i++){
        const col = column_names[i];
        html += '  <th>'+col+'\n';
    }
    html += '</tr>';
    html += '</thead>'

    for(let r = 0; r < cells.length; r++){
        let row = cells[r];
        html += '<tr>\n';
        html += '  <td>'+(r+1)+'\n';
        for(let i = 0; i < row.length; i++){
            let c = row[i];
            html += `  <td data-row=${r} data-col=${i}>`+c+'\n';
        }
        html += '</tr>\n';
    }
    raw.innerHTML = html;
    for(const td of document.querySelectorAll('td')){
        if(!td.dataset.row) continue;
        if(!td.dataset.col) continue;
        td.onclick = function(e:MouseEvent){
            e.stopPropagation();
            const inp = document.createElement('input');
            const r = +td.dataset.row!;
            const c = +td.dataset.col!;
            const is_func = !!td.dataset.func;
            let txt;
            if(is_func)
                txt = func_cells[r][c];
            else if(r == -1)
                txt = extra_cells[c];
            else
                txt = cells[r][c];
            inp.value = ""+txt;
            td.textContent = '';
            td.appendChild(inp);
            inp.onkeydown = function(e:KeyboardEvent){
                e.stopPropagation();
                switch(e.key){
                    case 'Enter':
                    case 'Escape':
                        inp.blur();
                        e.preventDefault();
                        return;
                }
            };
            inp.onblur = function(){
                const t = inp.value.trim();
                inp.remove();
                if(!t){
                    if(is_func)
                        func_cells[r][c] = t;
                    else if(r == -1)
                        extra_cells[c] = t;
                    else cells[r][c] = t;
                }
                else {
                    // @ts-ignore
                    const T = !isNaN(t)?+t:t;
                    if(is_func)
                        func_cells[r][c] = T;
                    else if(r == -1)
                        extra_cells[c] = T;
                    else cells[r][c] = T;
                }
                if(r == -1)
                    get_ctx().set_extra_str(0, c, t);
                else
                    get_ctx().set_str(is_func?1:0, r, c, t);

                // prep();
                const N = 1;
                // const N = 1000;
                const before = window.performance.now()
                for(let i = 0; i < N; i++){
                  get_ctx().evaluate_formulas();
                }
                const after = window.performance.now();
                console.log('after-before', after-before);
                // @ts-ignore
                console.log('bytes used:', ex.bytes_used());
                /*
                if(ctx){
                    // @ts-ignore
                    ex.drsp_destroy_ctx(ctx.id);
                    ctx = undefined;
                }
                */
                show();
            };
            inp.focus();
        };
    }
}
const enum DrspSheetFlags {
    NONE = 0x0,
    IS_FUNCTION = 0x1,
}
const enum DRSP {IDX_EXTRA_DIMENSIONAL = -2147483645  } // INT32_MIN+3
type DrSpreadCtx = {
    id: number;
    evaluate_formulas: () => number;
    evaluate_string: (sheet:number, s:string) => number | string;
    set_str:(sheet:number, row:number, col:number, s:string) => number;
    set_extra_str:(sheet:number, id:number, s:string) => number;
    make_sheet:(sheet:number, name:string) => number;
    set_sheet_alias:(sheet:number, name:string) => number;
    set_col_name:(sheet:number, idx: number, name:string) => number;
    del_sheet:(sheet:number) => number;
    set_flags:(sheet:number, flags:DrspSheetFlags) => number;
    set_flag:(sheet:number, flag: DrspSheetFlags, on:boolean|number) => number;
    get_flags:(sheet:number) => number;

    set_func_params:(func:number, rows:Array<number>, cols:Array<number>) => number;
    clear_func_params:(func:number) => number;
    set_func_output:(func:number, row:number, col:number) => number;
};
type DrSpreadExports = {
    drsp_create_ctx: () => number;
    drsp_destroy_ctx: (ctx:number) => number;
    drsp_evaluate_formulas: (ctx:number) => number;
    drsp_evaluate_string: (ctx:number, sheet:number, ptext:number, txtlen:number, result:number, caller_row:number, caller_col:number) => number;
    drsp_set_cell_str:(ctx:number, sheet:number, row:number, col:number, ptxt:number, txtlen:number) => number;
    drsp_set_sheet_name:(ctx:number, sheet:number, ptxt:number, txtlen:number) => number;
    drsp_set_sheet_alias:(ctx:number, sheet:number, ptxt:number, txtlen:number) => number;
    drsp_set_col_name:(ctx:number, sheet:number, idx:number, ptxt:number, txtlen:number) => number;
    drsp_del_sheet:(ctx:number, sheet:number) => number;
    drsp_set_sheet_flags:(ctx:number, sheet:number, flags:number) => number;
    drsp_set_sheet_flag:(ctx:number, sheet:number, flag:number, on:number) => number;
    drsp_get_sheet_flags:(ctx:number, sheet:number) => number;
    drsp_set_function_param:(ctx:number, func:number, param_idx:number, row:number, col:number) => number;
    drsp_set_function_params:(ctx:number, func:number, n_params:number, prows:number, pcols:number) => number;
    drsp_clear_function_params:(ctx:number, func:number) => number;
    drsp_set_function_output:(ctx:number, func:number, row:number, col:number) => number;

    memory: WebAssembly.Memory;
    reset_memory: () => void;

    wasm_str_buff: {value:number};
    wasm_result: {value:number};
    wasm_parambuff_row: {value:number};
    wasm_parambuff_col: {value:number};
};
declare function drspread(
    wasm_path:string,
    sheet_set_display_number:(id:number, row:number, col:number, val:number)=>void,
    sheet_set_display_string_:(id:number, row:number, col:number, s:string)=>void,
    sheet_set_display_error:(id:number, row:number, col:number)=>void,
):Promise<{
    make_ctx: () => DrSpreadCtx;
    exports: DrSpreadExports;
}>;

drspread(
    '/Bin/drspread.wasm',
    display_number,
    display_string,
    display_error,
).then(({exports, make_ctx}) =>{
    ex = exports;
    mk_ctx = make_ctx;
    // const N = 1000;
    const N = 1;
    window.performance.mark('evaluate');
    const before = window.performance.now()
    for(let i = 0; i < N; i++){
        get_ctx().evaluate_formulas();
    }
    window.performance.mark('done-evaluate');
    window.performance.measure('evaluate', 'evaluate', 'done-evaluate');
    const after = window.performance.now();
    console.log('after-before', after-before);
    // @ts-ignore
    console.log('bytes used:', ex.bytes_used());
    if(document.readyState != 'complete'){
        document.addEventListener('DOMContentLoaded', ()=>{make_elems(); show();});
    }
    else{
        make_elems();
        show();
    }
});
document.addEventListener('DOMContentLoaded', function(){
    const inp = document.getElementById('filepicker') as HTMLInputElement;
    inp.onchange = function(){
        if(!inp.files || inp.files.length == 0) return;
        const file = inp.files[0];
        file.text().then((csv)=>{
          const lines = csv.split('\n');
          const result = [];
          for(const line of lines){
            const row = [];
            for(let cell of line.split('|')){
              cell = cell.trim();
              // @ts-ignore
              if(cell.length && !isNaN(cell))
              // @ts-ignore
                cell = +cell;
              row.push(cell);
            }
            for(let c of row){
              if(c){
                result.push(row);
                break;
              }
            }
          }
          ex.reset_memory();
          ctx = undefined;
          cells = result;
          prep();
          get_ctx().evaluate_formulas();
          show();
        });

    };
});
