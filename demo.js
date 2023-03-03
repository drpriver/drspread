"use strict";
const column_names = 'abcdefghijklmnopqrstuvwxyz';
let display = [];
let to_iterate = [];
function prep() {
    display = [];
    to_iterate = [];
    for (let r = 0; r < cells.length; r++) {
        const row = cells[r];
        const d = [];
        display.push(d);
        for (let c = 0; c < row.length; c++) {
            const val = row[c];
            if (typeof val === 'string' && val[0] == '=') {
                to_iterate.push([r, c]);
                d.push('');
            }
            else {
                d.push('' + val);
            }
        }
    }
}
prep();
function cell_text(i, row, col) {
    return cells[row][col];
}
function col_height(i, c) {
    return cells.length;
}
function row_width(i, r) {
    return cells[r].length;
}
function display_number(i, row, col, val) {
    if ((val | 0) == val)
        display[row][col] = "" + val;
    else
        display[row][col] = val.toFixed(2);
}
function display_string(i, row, col, val) {
    display[row][col] = val;
}
function display_error(i, row, col) {
    display[row][col] = 'err';
}
function name_to_col_idx(i, s) {
    return column_names.indexOf(s.toLowerCase());
}
function next_cell(id, i, pr, pc) {
    if (i >= to_iterate.length)
        return [-1, -1];
    return to_iterate[i];
}
function dims(i) {
    return [cells[0].length, cells.length];
}
let table;
let raw;
let pre;
let ev_string;
let ev_formulas;
let ex;
function process(v) {
    let n = ev_string(0, v);
    pre.textContent += '\n> ' + v;
    pre.textContent += '\n' + n;
}
function make_elems() {
    const d = document.createElement('div');
    const showraw = document.createElement('button');
    showraw.textContent = 'Show Raw';
    showraw.onclick = () => {
        table.style.display = 'none';
        raw.style.display = 'table';
    };
    const showcalc = document.createElement('button');
    showcalc.onclick = () => {
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
    input.onkeydown = function (e) {
        if (e.key == 'Enter') {
            e.preventDefault();
            e.stopPropagation();
            let v = input.value.trim();
            if (!v)
                return;
            process(v);
            input.value = '';
        }
    };
}
function show() {
    {
        let html = '<thead>';
        html += '<tr>\n  <th>\n';
        for (let i = 0; i < cells[0].length; i++) {
            const col = column_names[i];
            html += '  <th>' + col + '\n';
        }
        html += '</tr>';
        html += '</thead>';
        for (let r = 0; r < display.length; r++) {
            let row = display[r];
            html += '<tr>\n';
            html += '  <td>' + (r + 1) + '\n';
            for (let i = 0; i < row.length; i++) {
                let c = row[i];
                if (c == 'err')
                    html += `  <td class=error data-row=${r} data-col=${i}>` + c + '\n';
                else
                    html += `  <td data-row=${r} data-col=${i}>` + c + '\n';
            }
            html += '</tr>\n';
        }
        table.innerHTML = html;
    }
    let html = '<thead>';
    html += '<tr>\n  <th>\n';
    for (let i = 0; i < cells[0].length; i++) {
        const col = column_names[i];
        html += '  <th>' + col + '\n';
    }
    html += '</tr>';
    html += '</thead>';
    for (let r = 0; r < cells.length; r++) {
        let row = cells[r];
        html += '<tr>\n';
        html += '  <td>' + (r + 1) + '\n';
        for (let i = 0; i < row.length; i++) {
            let c = row[i];
            html += `  <td data-row=${r} data-col=${i}>` + c + '\n';
        }
        html += '</tr>\n';
    }
    raw.innerHTML = html;
    for (const td of document.querySelectorAll('td')) {
        if (!td.dataset.row)
            continue;
        if (!td.dataset.col)
            continue;
        td.onclick = function (e) {
            e.stopPropagation();
            const inp = document.createElement('input');
            const r = +td.dataset.row;
            const c = +td.dataset.col;
            const txt = cells[r][c];
            inp.value = "" + txt;
            td.textContent = '';
            td.appendChild(inp);
            inp.onkeydown = function (e) {
                e.stopPropagation();
                switch (e.key) {
                    case 'Enter':
                    case 'Escape':
                        inp.blur();
                        e.preventDefault();
                        return;
                }
            };
            inp.onblur = function () {
                const t = inp.value.trim();
                inp.remove();
                if (!t)
                    cells[r][c] = t;
                else
                    cells[r][c] = !isNaN(t) ? +t : t;
                prep();
                ev_formulas(0);
                show();
            };
            inp.focus();
        };
    }
}
drspread('/Bin/drspread.wasm', cell_text, col_height, row_width, display_number, display_string, display_error, name_to_col_idx, next_cell, dims, function (s) { return 0; }).then(({ evaluate_formulas, evaluate_string, exports }) => {
    ex = exports;
    ev_formulas = evaluate_formulas;
    const N = 1;
    window.performance.mark('evaluate');
    const before = window.performance.now();
    for (let i = 0; i < N; i++) {
        evaluate_formulas(0);
    }
    window.performance.mark('done-evaluate');
    window.performance.measure('evaluate', 'evaluate', 'done-evaluate');
    const after = window.performance.now();
    console.log('after-before', after - before);
    ev_string = evaluate_string;
    if (document.readyState != 'complete') {
        document.addEventListener('DOMContentLoaded', () => { make_elems(); show(); });
    }
    else {
        make_elems();
        show();
    }
});
document.addEventListener('DOMContentLoaded', function () {
    const inp = document.getElementById('filepicker');
    inp.onchange = function () {
        if (!inp.files || inp.files.length == 0)
            return;
        const file = inp.files[0];
        file.text().then((csv) => {
            const lines = csv.split('\n');
            const result = [];
            for (const line of lines) {
                const row = [];
                for (let cell of line.split('|')) {
                    cell = cell.trim();
                    if (cell.length && !isNaN(cell))
                        cell = +cell;
                    row.push(cell);
                }
                for (let c of row) {
                    if (c) {
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
