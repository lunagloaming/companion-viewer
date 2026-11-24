'use strict';
// The window: an "Avatars" tab with every account's balance, and one tab per avatar opened
// by double-clicking it, with its income and expenses. All data comes from window.money
// (preload.js); everything is redrawn when the main process says a log changed.

const tabBar = document.getElementById('tabs');
const paneArea = document.getElementById('panes');
const tabs = new Map(); // 'accounts' or an account folder -> { tab, pane }

const amountText = (currency, amount) =>
    amount === null ? `${currency}-` : `${amount < 0 ? '-' : ''}${currency}${Math.abs(amount).toLocaleString()}`;
const day = (t) => new Date(t).toLocaleDateString();
const clock = (t) => new Date(t).toLocaleTimeString();

function element(tag, className, text) {
    const e = document.createElement(tag);
    if (className) e.className = className;
    if (text !== undefined) e.textContent = text; // text only: names come from the logs
    return e;
}

// A profile picture, or a placeholder with the name's first letter. CSS shows them small
// and square.
function picture(url, name, className) {
    if (!url) return element('div', `${className} placeholder`, name.charAt(0).toUpperCase());
    const img = element('img', className);
    img.src = url;
    img.alt = '';
    img.addEventListener('error', () => img.replaceWith(picture(null, name, className)));
    return img;
}

// columns: [title, class]; rows: { cells: [text or element...], onOpen?: fn }
function table(columns, rows, empty) {
    if (rows.length === 0) return element('p', 'empty', empty);
    const t = element('table');
    const head = t.createTHead().insertRow();
    for (const [title, cls] of columns) head.append(element('th', cls, title));
    const body = t.createTBody();
    for (const row of rows) {
        const tr = body.insertRow();
        row.cells.forEach((cell, i) => {
            const td = element('td', columns[i][1]);
            td.append(cell); // strings become text nodes
            tr.append(td);
        });
        if (row.onOpen) {
            tr.tabIndex = 0;
            tr.className = 'openable';
            tr.addEventListener('dblclick', row.onOpen);
            tr.addEventListener('keydown', (e) => e.key === 'Enter' && row.onOpen());
        }
    }
    return t;
}

function select(id) {
    for (const [key, { tab, pane }] of tabs) {
        const on = key === id;
        tab.classList.toggle('selected', on);
        tab.setAttribute('aria-selected', on);
        pane.hidden = !on;
    }
}

function addTab(id, label, closable) {
    const tab = element('button', 'tab');
    tab.setAttribute('role', 'tab');
    tab.append(element('span', 'label', label));
    const pane = element('section', 'pane');
    tab.addEventListener('click', () => select(id));
    if (closable) {
        const close = element('span', 'close', '×');
        close.title = 'Close';
        close.addEventListener('click', (e) => {
            e.stopPropagation();
            closeTab(id);
        });
        tab.append(close);
        tab.addEventListener('auxclick', (e) => e.button === 1 && closeTab(id)); // middle click
    }
    tabBar.append(tab);
    paneArea.append(pane);
    tabs.set(id, { tab, pane, expanded: new Map() }); // expanded: ledger section -> open group ids
    return pane;
}

function closeTab(id) {
    const { tab, pane } = tabs.get(id);
    const wasSelected = tab.classList.contains('selected');
    tab.remove();
    pane.remove();
    tabs.delete(id);
    if (wasSelected) select('accounts');
}

async function drawAccounts() {
    const rows = await window.money.accounts();
    const pane = tabs.get('accounts').pane;
    pane.replaceChildren(
        table(
            [['', 'picture-cell'], ['Avatar', 'name'], ['Balance', 'amount'], ['Last change', 'when']],
            rows.map((r) => ({
                cells: [
                    picture(r.picture, r.name, 'picture'),
                    r.name,
                    amountText(r.currency, r.balance),
                    r.asOf ? `${day(r.asOf)} ${clock(r.asOf)}` : '',
                ],
                onOpen: () => openLedger(r.folder, r.name),
            })),
            'No viewer accounts found.',
        ),
        element('p', 'hint', 'Double-click an avatar to see its income and expenses.'),
    );
}

// One row per sending or receiving avatar with their total; clicking a row (or Enter)
// expands it to list their transactions, newest first. expanded holds the open groups'
// ids, so a redraw after a log change keeps them open.
function groupTable(groups, who, currency, expanded, empty) {
    if (groups.length === 0) return element('p', 'empty', empty);
    const t = element('table', 'groups');
    const head = t.createTHead().insertRow();
    for (const [title, cls] of [['', 'toggle'], [who, 'name'], ['Count', 'amount'], ['Total', 'amount'], ['Last', 'when']]) {
        head.append(element('th', cls, title));
    }
    for (const g of groups) {
        const body = t.createTBody(); // one per group: its row and, when open, its transactions
        const row = body.insertRow();
        row.className = 'group';
        row.tabIndex = 0;
        const arrow = element('td', 'toggle');
        row.append(
            arrow,
            element('td', 'name', g.name),
            element('td', 'amount', String(g.count)),
            element('td', 'amount', amountText(currency, Math.abs(g.total))),
            element('td', 'when', day(g.last)),
        );
        const details = body.insertRow();
        const cell = element('td', 'details');
        cell.colSpan = 5;
        cell.append(
            table(
                [['Date', 'when'], ['Time', 'when'], ['Amount', 'amount'], ['For', 'detail']],
                g.entries.map((e) => ({
                    cells: [day(e.time), clock(e.time), amountText(currency, Math.abs(e.change)), [e.object, e.kind].filter(Boolean).join(' · ')],
                })),
                '',
            ),
        );
        details.append(cell);
        const show = (open) => {
            details.hidden = !open;
            arrow.textContent = open ? '▾' : '▸';
            row.setAttribute('aria-expanded', open);
        };
        show(expanded.has(g.id));
        const toggle = () => {
            if (expanded.has(g.id)) expanded.delete(g.id);
            else expanded.add(g.id);
            show(expanded.has(g.id));
        };
        row.addEventListener('click', toggle);
        row.addEventListener('keydown', (e) => e.key === 'Enter' && toggle());
    }
    return t;
}

async function drawLedger(folder) {
    const data = await window.money.ledger(folder);
    const pane = tabs.get(folder)?.pane;
    if (!pane) return; // closed while loading
    if (!data) {
        pane.replaceChildren(element('p', 'empty', "This avatar's folder is gone."));
        return;
    }
    const { expanded } = tabs.get(folder);
    const section = (title, who, groups, empty) => {
        const s = element('div', 'ledger');
        if (!expanded.has(title)) expanded.set(title, new Set());
        s.append(element('h2', null, title), groupTable(groups, who, data.currency, expanded.get(title), empty));
        return s;
    };
    const columns = element('div', 'columns');
    columns.append(
        section('Income', 'From', data.income, 'No income logged.'),
        section('Expenses', 'To', data.expenses, 'No expenses logged.'),
    );
    const header = element('header', 'avatar');
    header.append(
        picture(data.picture, data.name, 'picture large'),
        element('h1', null, `${data.name}: ${amountText(data.currency, data.balance)}`),
    );
    pane.replaceChildren(header, columns);
}

function openLedger(folder, name) {
    if (!tabs.has(folder)) {
        addTab(folder, name, true);
        drawLedger(folder);
    }
    select(folder);
}

addTab('accounts', 'Avatars', false);
select('accounts');
drawAccounts();
window.money.onChanged(() => {
    drawAccounts();
    for (const id of tabs.keys()) if (id !== 'accounts') drawLedger(id);
});
