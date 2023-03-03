"use strict";
let winst;
let exports;
let mem;
let memview;
let malloc;
const decoder = new TextDecoder();
const encoder = new TextEncoder();
let active;
function fwrite_(p, sz, fp) {
    let s = wasm_string_to_js(p, sz);
    s = s.replace(/\x1B\[1m/g, '<span class=bold>');
    s = s.replace(/\x1B\[3m/g, '<span class=italic>');
    s = s.replace(/\x1B\[97m/g, '<span class=gray>');
    s = s.replace(/\x1B\[94m/g, '<span class=blue>');
    s = s.replace(/\x1B\[92m/g, '<span class=green>');
    s = s.replace(/\x1B\[91m/g, '<span class=red>');
    s = s.replace(/\x1B\[0m/g, '</span>');
    s = s.replace(/\x1B\[39;49m/g, '</span>');
    active.innerHTML += s;
    return sz;
}
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
    mem.set(encoded, p + 4);
    return p;
}
function js_string_to_wasm_cstring(s) {
    const encoded = encoder.encode(s);
    const p = malloc(encoded.length + 4);
    mem.set(encoded, p);
    mem[p + encoded.length] = 0;
    return p;
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
        pow: Math.pow,
        fwrite_: fwrite_,
    },
};
fetch('Bin/TestDrSpread.wasm')
    .then(response => response.arrayBuffer())
    .then(bytes => WebAssembly.instantiate(bytes, imports))
    .then(x => {
    winst = x.instance;
    exports = winst.exports;
    const m = exports.memory;
    m.grow(1024);
    mem = new Uint8Array(m.buffer);
    memview = new DataView(mem.buffer);
    malloc = exports.malloc;
    const main = exports.main;
    let number = 0;
    function run() {
        const args = ['TestWeb'];
        const txt = document.getElementById('args').value.split(' ');
        for (const arg of txt)
            if (arg)
                args.push(arg);
        exports.reset_memory();
        const argv = exports.calloc(args.length + 1, 4);
        for (let i = 0; i < args.length; i++) {
            const p = js_string_to_wasm_cstring(args[i]);
            write4(argv + i * 4, p);
        }
        const out = document.getElementById('stdout');
        const deets = document.createElement('details');
        deets.open = true;
        const summ = document.createElement('summary');
        summ.innerText = `Run ${++number}\n`;
        active = document.createElement('pre');
        deets.append(summ, active);
        out.insertAdjacentElement('afterbegin', deets);
        main(args.length, argv);
    }
    if (document.readyState === "complete" || document.readyState === 'interactive') {
        const button = document.getElementById('run');
        button.onclick = function () {
            run();
        };
        const input = document.getElementById('args');
        input.onkeydown = function (e) {
            if (e.key == 'Enter')
                button.click();
        };
        input.focus();
        run();
    }
    else {
        document.addEventListener('DOMContentLoaded', function () {
            const button = document.getElementById('run');
            button.onclick = function () {
                run();
            };
            const input = document.getElementById('args');
            input.onkeydown = function (e) {
                if (e.key == 'Enter')
                    button.click();
            };
            input.focus();
            run();
        });
    }
});
document.addEventListener('DOMContentLoaded', function () {
    const close = document.getElementById('close');
    close.onclick = function () {
        for (const d of document.getElementsByTagName('details'))
            d.open = false;
    };
    const open = document.getElementById('open');
    open.onclick = function () {
        for (const d of document.getElementsByTagName('details'))
            d.open = true;
    };
});
