'use strict';
// Global Accountant: shows every account's L$ balance and transactions from the viewers' money logs.
// Read only. The main process reads and watches the files; the page (renderer/) only displays.
// Profile pictures of Second Life accounts come from world.secondlife.com, checked weekly.

const { app, BrowserWindow, ipcMain, Menu, net } = require('electron');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const ledger = require('./src/ledger');

const LOG_NAME = 'money_log.txt';
const NAME_CACHE = 'avatar_name_cache.xml';
const PICTURE_DAYS = 7; // how long a profile picture lookup is trusted
const parsedCache = new Map(); // file -> { stamp, parsed }: re-read only files that changed
let accounts = new Map(); // account folder name -> merged log
let names = new Map(); // Second Life user name -> { id, displayName }, from the viewers' name caches
let pictures = {}; // avatar key -> { url (null: no picture), checked (ms) }; kept in pictures.json
let window = null;

// A parsed file, or null if it doesn't exist.
function readCached(file, parse) {
    let stat;
    try {
        stat = fs.statSync(file);
    } catch {
        return null;
    }
    const stamp = `${stat.mtimeMs}:${stat.size}`;
    const cached = parsedCache.get(file);
    if (cached && cached.stamp === stamp) return cached.parsed;
    const parsed = parse(fs.readFileSync(file, 'utf8'));
    parsedCache.set(file, { stamp, parsed });
    return parsed;
}

function isFolder(dir) {
    try {
        return fs.statSync(dir).isDirectory();
    } catch {
        return false;
    }
}

function existingDataFolders() {
    return ledger.dataFolders(process.platform, os.homedir(), process.env).filter(isFolder);
}

// Name caches sit in the cache folder, or in one numbered folder per viewer instance.
function readNames() {
    const found = new Map();
    for (const dir of ledger.cacheFolders(process.platform, os.homedir(), process.env).filter(isFolder)) {
        const files = [path.join(dir, NAME_CACHE)];
        for (const sub of fs.readdirSync(dir)) files.push(path.join(dir, sub, NAME_CACHE));
        for (const file of files) {
            const parsed = readCached(file, ledger.parseNameCache);
            if (parsed) for (const [user, entry] of parsed) found.set(user, entry);
        }
    }
    return found;
}

function picturesFile() {
    return path.join(app.getPath('userData'), 'pictures.json');
}

// Look up the profile pictures not checked for a week, one at a time. A failed lookup
// (offline, server trouble) is tried again on the next start.
let lookingUp = false;
const failed = new Set();

async function lookUpPictures(ids) {
    if (lookingUp) return;
    lookingUp = true;
    let found = false;
    for (const id of ids) {
        const known = pictures[id];
        if (failed.has(id) || (known && Date.now() - known.checked < PICTURE_DAYS * 86400e3)) continue;
        try {
            const response = await net.fetch(`https://world.secondlife.com/resident/${id}`);
            if (!response.ok && response.status !== 404) throw new Error(`HTTP ${response.status}`);
            const image = response.ok ? ledger.profileImageId(await response.text()) : null;
            pictures[id] = {
                url: image ? `https://picture-service.secondlife.com/${image}/256x192.jpg` : null,
                checked: Date.now(),
            };
            found = true;
        } catch {
            failed.add(id);
        }
    }
    lookingUp = false;
    if (found) {
        fs.writeFileSync(picturesFile(), JSON.stringify(pictures));
        if (window) window.webContents.send('changed');
    }
}

// An account's row, with its display name and picture when it is a known Second Life account.
function accountRow(folder, merged) {
    const row = ledger.summarize(folder, merged);
    const user = ledger.slUsername(folder);
    const known = user && names.get(user);
    if (known) {
        row.name = ledger.fullName(user, known.displayName);
        row.picture = pictures[known.id]?.url || null;
    }
    return row;
}

// Every account folder in every viewer's data folder; an account found in more than one
// viewer's folder is one avatar, with its logs merged.
function scan() {
    const logsByAccount = new Map();
    for (const dir of existingDataFolders()) {
        for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
            if (!entry.isDirectory() || !ledger.isAccountFolder(entry.name)) continue;
            if (!logsByAccount.has(entry.name)) logsByAccount.set(entry.name, []);
            logsByAccount.get(entry.name).push(path.join(dir, entry.name));
        }
    }
    accounts = new Map();
    for (const [name, folders] of logsByAccount) {
        const parsed = folders.map((f) => readCached(path.join(f, LOG_NAME), ledger.parseLog)).filter(Boolean);
        accounts.set(name, ledger.mergeLogs(parsed));
    }
    names = readNames();
    watch(logsByAccount);
    const ids = [...accounts.keys()].map((f) => names.get(ledger.slUsername(f))?.id).filter(Boolean);
    lookUpPictures(ids);
}

// Live updates: one non-recursive watcher per data folder (accounts appearing or going) and
// per account folder (its money log changing). A recursive watch would also cover the
// texture cache, which changes all the time.
const watchers = new Map(); // folder -> fs.FSWatcher
let rescanTimer = null;

function changed() {
    // Editors and the viewer can write a file in several steps: wait for them to settle.
    clearTimeout(rescanTimer);
    rescanTimer = setTimeout(() => {
        scan();
        if (window) window.webContents.send('changed');
    }, 300);
}

function watchFolder(folder, onlyLog) {
    if (watchers.has(folder)) return;
    try {
        const w = fs.watch(folder, (event, file) => {
            if (!onlyLog || !file || file === LOG_NAME) changed();
        });
        w.on('error', () => {
            w.close();
            watchers.delete(folder);
        });
        watchers.set(folder, w);
    } catch {
        // The folder went away between the scan and here; the next scan sorts it out.
    }
}

function watch(logsByAccount) {
    const wanted = new Set(existingDataFolders());
    for (const dir of wanted) watchFolder(dir, false);
    for (const folders of logsByAccount.values()) {
        for (const f of folders) {
            wanted.add(f);
            watchFolder(f, true);
        }
    }
    for (const [folder, w] of watchers) {
        if (!wanted.has(folder)) {
            w.close();
            watchers.delete(folder);
        }
    }
}

ipcMain.handle('accounts', () => [...accounts].map(([name, merged]) => accountRow(name, merged)).sort(ledger.compareRows));

ipcMain.handle('ledger', (_event, folder) => {
    const merged = accounts.get(folder);
    if (!merged) return null; // the folder was removed
    const { income, expenses } = ledger.splitLedger(merged);
    return {
        ...accountRow(folder, merged),
        income: ledger.groupByCounterparty(income),
        expenses: ledger.groupByCounterparty(expenses),
    };
});

function createWindow() {
    window = new BrowserWindow({
        width: 900,
        height: 640,
        title: 'Global Accountant',
        icon: path.join(__dirname, 'build', 'icon.png'),
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            sandbox: true,
        },
    });
    window.loadFile(path.join(__dirname, 'renderer', 'index.html'));
    window.on('closed', () => {
        window = null;
    });
}

// The viewer's Help > Global Accountant starts the app on every click: a second start brings
// the open window forward instead of opening another.
if (!app.requestSingleInstanceLock()) app.exit(0);
app.on('second-instance', () => {
    if (!window) return createWindow();
    if (window.isMinimized()) window.restore();
    window.focus();
});

app.whenReady().then(() => {
    // Only macOS needs a menu (for Quit, Copy and the like); elsewhere the window has none.
    if (process.platform !== 'darwin') Menu.setApplicationMenu(null);
    try {
        pictures = JSON.parse(fs.readFileSync(picturesFile(), 'utf8'));
    } catch {
        pictures = {}; // first start
    }
    scan();
    createWindow();
    app.on('activate', () => {
        if (!window) createWindow();
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit();
});
