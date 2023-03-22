//
// Copyright © 2023, David Priver
//
declare let cells: Array<Array<string|number>>;
const column_names = 'abcdefghijklmnopqrstuvwxyz';
let display:Array<Array<string>> = [
];
function prep():void{
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
}
prep();

function display_number(i:number, row:number, col:number, val:number):void{
    if((val | 0) == val)
        display[row][col] = ""+val;
    else
        display[row][col] = val.toFixed(2);
}

function display_string(i:number, row:number, col:number, val:string):void{
    display[row][col] = val;
}

function display_error(i:number, row:number, col:number):void{
    display[row][col] = 'err';
}

function name_to_col_idx(i:number, s:string):number{
    return -1;
}


let table:HTMLTableElement;
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
    raw = document.createElement('table');
    document.body.appendChild(raw);
    raw.style.display = 'none';

    table = document.createElement('table');
    document.body.appendChild(table);

    pre = document.createElement('pre');
    document.body.appendChild(pre);
    const input = document.createElement('input');
    input.spellcheck = false;
    document.body.appendChild(input);
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
            const txt = cells[r][c];
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
                if(!t) cells[r][c] = t;
                // @ts-ignore
                else   cells[r][c] = !isNaN(t)?+t:t;
                get_ctx().set_str(0, r, c, t);
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
                if(ctx){
                    // @ts-ignore
                    ex.drsp_destroy_ctx(ctx.id);
                    ctx = undefined;
                }
                show();
            };
            inp.focus();
        };
    }
}
type DrSpreadCtx = {
    evaluate_formulas: () => void;
    evaluate_string: (sheet:number, s:string) => number | string;
    set_str:(sheet:number, row:number, col:number, s:string) => void;
    make_sheet:(sheet:number, name:string) => void;
    set_sheet_alias:(sheet:number, name:string) => void;
    set_col_name:(sheet:number, idx: number, name:string) => void;
    del_sheet:(sheet:number) => void;
};
type DrSpreadExports = {
    memory: WebAssembly.Memory;
    strlen: (p:number) => number;
    drsp_create_ctx: () => number;
    drsp_destroy_ctx: (ctx:number) => number;
    drsp_evaluate_formulas: (ctx:number) => number;
    drsp_evaluate_string: (ctx:number, sheet:number, ptext:number, txtlen:number, result:number, caller_row:number, caller_col:number) => number;
    drsp_set_cell_str:(ctx:number, sheet:number, row:number, col:number, ptxt:number, txtlen:number) => number;
    drsp_set_sheet_name:(ctx:number, sheet:number, ptxt:number, txtlen:number) => number;
    drsp_set_sheet_alias:(ctx:number, sheet:number, ptxt:number, txtlen:number) => number;
    drsp_set_col_name:(ctx:number, sheet:number, idx:number, ptxt:number, txtlen:number) => number;
    drsp_del_sheet:(ctx:number, sheet:number) => number;
    reset_memory: () => void;
    wasm_str_buff: {value:number};
    wasm_result: {value:number};
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
