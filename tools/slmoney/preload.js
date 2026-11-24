'use strict';
// The page's only way to the data: two questions and a "something changed" signal.

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('money', {
    accounts: () => ipcRenderer.invoke('accounts'),
    ledger: (folder) => ipcRenderer.invoke('ledger', folder),
    onChanged: (callback) => ipcRenderer.on('changed', () => callback()),
});
