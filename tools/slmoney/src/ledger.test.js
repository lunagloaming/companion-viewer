'use strict';
// Unit tests for the log reading: node --test (npm test). All names are invented.

const test = require('node:test');
const assert = require('node:assert/strict');
const L = require('./ledger');

const HEADER =
    '# Every change of the L$ balance, written by the viewer. Tab-separated:\n' +
    '# time (UTC)\tchange\tbalance after\tfrom or to\tobject or item\tkind\tkey of from or to\n';

const LOG =
    HEADER +
    '2026-11-20T18:00:00.00Z\t0\t1000\t\t\tbalance at login\t\n' +
    '2026-11-20T18:05:00.00Z\t+250\t1250\tTamsin Quillon (tamsin.quillon)\t\tpayment\t11111111-2222-3333-4444-555555555555\n' +
    '2026-11-20T18:10:00.00Z\t-100\t1150\tPixelfern (pixelfern)\tTip Jar\tpaid an object\t\n' +
    'not a line\n' +
    '2026-11-20T18:20:00.00Z\tx\t5\t\t\t\t\n' +
    '\n';

test('account folders: names with one underscore, minus the viewer folders', () => {
    for (const ok of ['luna_witch', 'pixelfern_resident', 'corvid_velvetyne.harbor_light']) {
        assert.ok(L.isAccountFolder(ok), ok);
    }
    for (const no of ['cache', 'logs', 'data', 'user_settings', 'browser_profile', 'a_b_c', '.hidden_x']) {
        assert.ok(!L.isAccountFolder(no), no);
    }
});

test('display names', () => {
    assert.equal(L.displayName('pixelfern_resident'), 'pixelfern');
    assert.equal(L.displayName('tamsin_quillon'), 'tamsin.quillon');
    assert.equal(L.displayName('corvid_velvetyne.harbor_light'), 'Corvid Velvetyne (harbor light)');
});

test('parsing skips comments, short and malformed lines', () => {
    const { currency, entries } = L.parseLog(LOG);
    assert.equal(currency, 'L$');
    assert.equal(entries.length, 3);
    assert.deepEqual(entries[1], {
        time: Date.parse('2026-11-20T18:05:00.00Z'),
        change: 250,
        balance: 1250,
        name: 'Tamsin Quillon (tamsin.quillon)',
        object: '',
        kind: 'payment',
        key: '11111111-2222-3333-4444-555555555555',
    });
    assert.equal(entries[2].change, -100);
});

test('the header names the currency', () => {
    assert.equal(L.parseLog(HEADER.replace('L$', 'OS$')).currency, 'OS$');
});

test('logs from two viewers merge in time order', () => {
    const a = L.parseLog(HEADER + '2026-11-21T10:00:00Z\t+5\t1155\t\t\tstipend\t\n');
    const merged = L.mergeLogs([a, L.parseLog(LOG)]);
    assert.deepEqual(merged.entries.map((e) => e.balance), [1000, 1250, 1150, 1155]);
    assert.equal(L.summarize('luna_witch', merged).balance, 1155);
});

test('rows: largest first, no log last, then by name', () => {
    const rows = [
        { name: 'b', balance: null },
        { name: 'c', balance: 0 },
        { name: 'a', balance: null },
        { name: 'd', balance: 40 },
        { name: 'e', balance: -3 },
    ].sort(L.compareRows);
    assert.deepEqual(rows.map((r) => r.name), ['d', 'c', 'e', 'a', 'b']);
});

test('an account without a log has no balance', () => {
    const row = L.summarize('pixelfern_resident', L.mergeLogs([]));
    assert.equal(row.balance, null);
    assert.equal(row.asOf, null);
});

test('ledger: income and expenses newest first, logins left out', () => {
    const { income, expenses } = L.splitLedger(L.parseLog(LOG));
    assert.deepEqual(income.map((e) => e.change), [250]);
    assert.deepEqual(expenses.map((e) => e.object), ['Tip Jar']);
});

test('name cache: user name to key and display name', () => {
    const xml = `<llsd><map><key>agents</key><map>
        <key>11111111-2222-3333-4444-555555555555</key>
            <map>
            <key>display_name</key><string>Fern &amp; Moss</string>
            <key>display_name_expires</key><date>2026-11-24T05:56:45.49Z</date>
            <key>username</key><string>pixelfern</string>
            </map>
        <key>66666666-7777-8888-9999-000000000000</key>
            <map><key>username</key><string>tamsin.quillon</string></map>
        </map></map></llsd>`;
    const names = L.parseNameCache(xml);
    assert.deepEqual(names.get('pixelfern'), { id: '11111111-2222-3333-4444-555555555555', displayName: 'Fern & Moss' });
    assert.deepEqual(names.get('tamsin.quillon'), { id: '66666666-7777-8888-9999-000000000000', displayName: '' });
});

test('Second Life user names and full names', () => {
    assert.equal(L.slUsername('pixelfern_resident'), 'pixelfern');
    assert.equal(L.slUsername('corvid_velvetyne.harbor_light'), null);
    assert.equal(L.fullName('pixelfern', 'Fern & Moss'), 'Fern & Moss (pixelfern)');
    assert.equal(L.fullName('tamsin.quillon', 'Tamsin Quillon'), 'tamsin.quillon');
    assert.equal(L.fullName('tamsin.quillon', ''), 'tamsin.quillon');
});

test('profile picture ID from a resident page', () => {
    const page = (id) => `<head><meta name="imageid" content="${id}"></head>`;
    assert.equal(L.profileImageId(page('aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee')), 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee');
    assert.equal(L.profileImageId(page('00000000-0000-0000-0000-000000000000')), null);
    assert.equal(L.profileImageId('<html></html>'), null);
});

test('grouping by counterparty: by key, newest name, largest total first', () => {
    const t = (n) => Date.parse(`2026-11-2${n}T12:00:00Z`);
    const newestFirst = [
        { time: t(9), change: 50, name: 'Fern Moss (pixelfern)', object: '', kind: 'payment', key: 'k1' },
        { time: t(8), change: 300, name: 'Tamsin Quillon', object: 'Tip Jar', kind: 'paid an object', key: 'k2' },
        { time: t(7), change: 100, name: 'Pixelfern (pixelfern)', object: '', kind: 'payment', key: 'k1' },
        { time: t(6), change: 5, name: '', object: '', kind: 'stipend', key: '' },
        { time: t(5), change: 5, name: '', object: '', kind: 'stipend', key: '' },
    ];
    const groups = L.groupByCounterparty(newestFirst);
    assert.deepEqual(groups.map((g) => [g.name, g.total, g.count]), [
        ['Tamsin Quillon', 300, 1],
        ['Fern Moss (pixelfern)', 150, 2],
        ['stipend', 10, 2],
    ]);
    assert.equal(groups[1].last, t(9));
    assert.deepEqual(groups[1].entries.map((e) => e.time), [t(9), t(7)]);
});

test('grouping expenses sorts by size, not sign', () => {
    const e = (change, key) => ({ time: 0, change, name: key, object: '', kind: '', key });
    assert.deepEqual(L.groupByCounterparty([e(-5, 'a'), e(-50, 'b')]).map((g) => g.id), ['b', 'a']);
});

test('data folders per platform', () => {
    assert.equal(L.dataFolders('linux', '/home/x', {})[0], '/home/x/.companionviewer');
    assert.equal(L.dataFolders('darwin', '/Users/x', {})[1], '/Users/x/Library/Application Support/Firestorm');
    assert.equal(L.dataFolders('win32', 'C:\\Users\\x', { APPDATA: 'C:\\R' })[0], 'C:\\R\\CompanionViewer');
    assert.equal(L.cacheFolders('linux', '/home/x', {})[0], '/home/x/.companionviewer/cache');
    assert.equal(L.cacheFolders('darwin', '/Users/x', {})[0], '/Users/x/Library/Caches/CompanionViewer');
    assert.equal(L.cacheFolders('win32', 'C:\\Users\\x', { LOCALAPPDATA: 'C:\\L' })[0], 'C:\\L\\CompanionViewer');
});
