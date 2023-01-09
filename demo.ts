declare let cells: Array<Array<string|number>>;
const column_names = 'abcdefghijklmnopqrstuvwxyz';
let display:Array<Array<string>> = [
];
let to_iterate:Array<[number, number]> = [];
function prep():void{
    display = [];
    to_iterate = [];
    for(let r = 0; r < cells.length; r++){
        const row = cells[r];
        const d:Array<string> = [];
        display.push(d);
        for(let c = 0; c < row.length; c++){
            const val = row[c];
            if(typeof val === 'string' && val[0] == '='){
                to_iterate.push([r, c]);
                d.push('');
            }
            else {
                d.push(''+val);
            }
        }
    }
}
prep();
function cell_kind(i:number, row:number, col:number):number{
    let result = 0;
    const cell = cells[row][col];
    switch(typeof cell){
        case 'number':
            result = 1;
            break;
        case 'string':
            if(!cell.length)
                result = 0;
            else
                result = cell[0] == '='?2:3;
            break;
    }
    return result;
}

function cell_number(i:number, row:number, col:number):number{
    return cells[row][col] as number;
}

function cell_text(i:number, row:number, col:number):string{
    return cells[row][col] as string;
}

function col_height(i:number, c:number):number{
    return cells.length;
}

function row_width(i:number, r:number):number{
    return cells[r].length;
}

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
    return column_names.indexOf(s.toLowerCase());
}

function next_cell(i:number, pr:number, pc:number):[number, number]{
    if(i >= to_iterate.length)
        return [4294967295, 4294967295];
    return to_iterate[i];
}

function dims(i:number):[number, number]{
    return [cells[0].length, cells.length];
}

let table:HTMLTableElement;
let raw:HTMLTableElement;
let pre:HTMLPreElement;
let ev_string:(_:number, s:string)=>number;
let ev_formulas:(_:number)=>void;
let ex:WebAssembly.Exports;

function process(v:string):void{
  let n = ev_string(0, v);
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
        for(let c of row){
            if(c == 'err')
                html += '  <td class=error>'+c+'\n';
            else
                html += '  <td>'+c+'\n';
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
        for(let c of row){
            html += '  <td>'+c+'\n';
        }
        html += '</tr>\n';
    }
    raw.innerHTML = html;
}
declare function drspread(
    wasm_path:string,
    sheet_cell_kind:(id:number, row:number, col:number)=>number,
    sheet_cell_number:(id:number, row:number, col:number)=>number,
    sheet_cell_text_:(id:number, row:number, col:number) => string,
    sheet_col_height:(id:number, col:number)=>number,
    sheet_row_width:(id:number, row:number)=>number,
    sheet_set_display_number:(id:number, row:number, col:number, val:number)=>void,
    sheet_set_display_string_:(id:number, row:number, col:number, s:string)=>void,
    sheet_set_display_error:(id:number, row:number, col:number)=>void,
    sheet_name_to_col_idx_:(id:number, s:string) => number,
    sheet_next_cell_:(id:number, i:number, prev_row:number, prev_col:number)=>[number, number],
    sheet_dims_:(id:number)=>[number, number],
):Promise<{
    evaluate_formulas: (id: number) => void; 
    evaluate_string: (id: number, s: string) => number; 
    exports: WebAssembly.Exports;
}>;

drspread(
    '/Bin/drspread.wasm', 
    cell_kind,
    cell_number,
    cell_text,
    col_height,
    row_width,
    display_number,
    display_string,
    display_error,
    name_to_col_idx,
    next_cell,
    dims,
).then(({evaluate_formulas, evaluate_string, exports}) =>{
    ex = exports;
    ev_formulas = evaluate_formulas;
    for(let i = 0; i < 1; i++){
        evaluate_formulas(0);
    }
    ev_string = evaluate_string;
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
          cells = result;
          prep();
          ev_formulas(0);
          show();
        });

    };
});
