'use strict';
// Reading the viewers' money logs. Companion Viewer writes one per account,
// <user data folder>/<account>/money_log.txt (indra/newview/fsmoneylog.cpp), newest line last:
//
//   # Every change of the L$ balance, written by the viewer. Tab-separated:
//   # time (UTC)<TAB>change<TAB>balance after<TAB>from or to<TAB>object or item<TAB>kind<TAB>key of from or to
//   2026-11-20T18:04:11.52Z<TAB>+250<TAB>1250<TAB>Some Name (some.name)<TAB><TAB>payment<TAB><uuid>
//
// "balance at login" lines have a change of 0 and only record the balance.
// Everything here is pure (no file access) so it can be unit tested.

const path = require('node:path');

// The viewer's own folders that look like account folders (one underscore).
const NOT_ACCOUNTS = new Set(['user_settings', 'browser_profile']);

// Account folders are "first_last" on Second Life ("name_resident" for single names) and
// "first_last.grid" on other grids, in lower case. The viewer's other folders (cache, logs,
// data, dictionaries...) have no underscore.
function isAccountFolder(name) {
    if (NOT_ACCOUNTS.has(name)) return false;
    return /^[^_.\s]+_[^_.\s]+(\..+)?$/.test(name);
}

// "luna_witch" -> "luna.witch", "pixelfern_resident" -> "pixelfern" (Second Life user names);
// "corvid_velvetyne.harbor_light" -> "Corvid Velvetyne (harbor light)".
function displayName(folder) {
    const dot = folder.indexOf('.');
    const name = dot < 0 ? folder : folder.slice(0, dot);
    if (dot < 0) return name.replace(/_resident$/, '').replace(/_/g, '.');
    const words = name.split('_').map((w) => w.charAt(0).toUpperCase() + w.slice(1));
    return `${words.join(' ')} (${folder.slice(dot + 1).replace(/_/g, ' ')})`;
}

function parseLog(text) {
    let currency = 'L$';
    const entries = [];
    for (const line of text.split(/\r?\n/)) {
        if (line.startsWith('#')) {
            // The header names the grid's currency (OpenSim grids may use another symbol).
            const m = /^# Every change of the (.+?) balance/.exec(line);
            if (m) currency = m[1];
            continue;
        }
        const f = line.split('\t');
        if (f.length < 3) continue;
        const time = Date.parse(f[0]);
        const change = Number.parseInt(f[1], 10);
        const balance = Number.parseInt(f[2], 10);
        if (Number.isNaN(time) || Number.isNaN(change) || Number.isNaN(balance)) continue;
        entries.push({
            time,
            change,
            balance,
            name: f[3] || '',
            object: f[4] || '',
            kind: f[5] || '',
            key: f[6] || '',
        });
    }
    return { currency, entries };
}

// One account's logs (it may have one in more than one viewer's folder), merged in time order.
function mergeLogs(parsed) {
    const entries = parsed.flatMap((p) => p.entries).sort((a, b) => a.time - b.time);
    const currency = parsed.length ? parsed[parsed.length - 1].currency : 'L$';
    return { currency, entries };
}

// The avatar list's row for an account. balance is null when there is no money log
// (shown as "L$-", sorted below L$0).
function summarize(folder, merged) {
    const last = merged.entries[merged.entries.length - 1];
    return {
        folder,
        name: displayName(folder),
        currency: merged.currency,
        balance: last ? last.balance : null,
        asOf: last ? last.time : null,
    };
}

// Largest balance first; accounts without a log last; then by name.
function compareRows(a, b) {
    if (a.balance !== b.balance) {
        if (a.balance === null) return 1;
        if (b.balance === null) return -1;
        return b.balance - a.balance;
    }
    return a.name.localeCompare(b.name);
}

// Income and expenses, newest first. Logins and balance-only lines (change 0) are left out.
function splitLedger(merged) {
    const newestFirst = [...merged.entries].reverse();
    return {
        income: newestFirst.filter((e) => e.change > 0),
        expenses: newestFirst.filter((e) => e.change < 0),
    };
}

// Transactions grouped by who sent or received them, largest total first. Entries are
// grouped by the other side's key, so a changed display name stays one group (labelled with
// the newest name); entries without a key by name, then by object or kind (system payments).
// Each group's entries are newest first. Expects entries newest first, as splitLedger gives.
function groupByCounterparty(entries) {
    const groups = new Map();
    for (const e of entries) {
        const id = e.key || `name:${e.name || e.object || e.kind}`;
        let g = groups.get(id);
        if (!g) {
            g = { id, name: e.name || e.object || e.kind || '—', total: 0, count: 0, last: e.time, entries: [] };
            groups.set(id, g);
        }
        g.total += e.change;
        g.count += 1;
        g.entries.push(e);
    }
    return [...groups.values()].sort((a, b) => Math.abs(b.total) - Math.abs(a.total) || a.name.localeCompare(b.name));
}

// The folders viewers keep their user data in, newest viewer first.
function dataFolders(platform, home, env) {
    const names = ['CompanionViewer', 'Firestorm', 'Firestorm_x64'];
    if (platform === 'win32') {
        const appData = env.APPDATA || path.join(home, 'AppData', 'Roaming');
        return names.map((n) => path.win32.join(appData, n));
    }
    if (platform === 'darwin') {
        return names.map((n) => path.posix.join(home, 'Library', 'Application Support', n));
    }
    return names.map((n) => path.posix.join(home, '.' + n.toLowerCase()));
}

// The viewer's name cache (<cache>/[<instance>/]avatar_name_cache.xml, LLSD XML):
//   <key>agents</key><map> <key>AGENT_ID</key><map> ... <key>display_name</key><string>..</string>
//   ... <key>username</key><string>..</string> </map> ... </map>
// Returns user name -> { id, displayName }. Only the fields needed are picked out, so a
// regular expression does instead of a full LLSD parser.
const UUID = '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}';

function unescapeXml(text) {
    return text.replace(/&(lt|gt|quot|apos|amp);/g, (_m, e) => ({ lt: '<', gt: '>', quot: '"', apos: "'", amp: '&' })[e]);
}

function parseNameCache(xml) {
    const names = new Map();
    const agent = new RegExp(`<key>(${UUID})</key>\\s*<map>([\\s\\S]*?)</map>`, 'g');
    for (const [, id, body] of xml.matchAll(agent)) {
        const field = (name) => {
            const m = new RegExp(`<key>${name}</key>\\s*<string>([^<]*)</string>`).exec(body);
            return m ? unescapeXml(m[1]) : '';
        };
        const username = field('username');
        if (username) names.set(username, { id, displayName: field('display_name') });
    }
    return names;
}

// The Second Life user name an account folder belongs to, or null for other grids.
function slUsername(folder) {
    return folder.includes('.') ? null : displayName(folder);
}

// "Display Name (user.name)", or just the user name when the display name adds nothing.
function fullName(username, displayNameText) {
    if (!displayNameText || displayNameText.toLowerCase() === username.replace('.', ' ')) return username;
    return `${displayNameText} (${username})`;
}

// The profile picture's ID from a world.secondlife.com/resident/<id> page, or null.
function profileImageId(html) {
    const m = new RegExp(`<meta name="imageid" content="(${UUID})"`).exec(html);
    return m && !/^[0-]+$/.test(m[1]) ? m[1] : null;
}

// The viewers' cache folders (where the name caches are), in the same order.
function cacheFolders(platform, home, env) {
    const names = ['CompanionViewer', 'Firestorm', 'Firestorm_x64'];
    if (platform === 'win32') {
        const local = env.LOCALAPPDATA || path.join(home, 'AppData', 'Local');
        return names.map((n) => path.win32.join(local, n));
    }
    if (platform === 'darwin') return names.map((n) => path.posix.join(home, 'Library', 'Caches', n));
    return names.map((n) => path.posix.join(home, '.' + n.toLowerCase(), 'cache'));
}

module.exports = {
    groupByCounterparty,
    cacheFolders,
    parseNameCache,
    slUsername,
    fullName,
    profileImageId,
    isAccountFolder,
    displayName,
    parseLog,
    mergeLogs,
    summarize,
    compareRows,
    splitLedger,
    dataFolders,
};
