# SL Money

A small Electron app that shows every viewer account's L$ balance, read from the
money logs Companion Viewer writes (`<user data folder>/<account>/money_log.txt`,
see `indra/newview/fsmoneylog.cpp`). It only reads; it never writes to the
viewer's folders. It is a separate program, packaged with the viewer: Help > Global
Accountant starts it.

- **Avatars tab:** one row per account folder, largest balance first. A folder
  without a money log shows `L$-` and sorts below `L$0`. Second Life accounts
  show their display name and profile picture.
- **An avatar's tab** (double-click or Enter on a row): income and expenses, one
  row per sending or receiving avatar with the count, total and latest date.
  Click a row (or Enter) to list its transactions, newest first. Times are local.
- **Live:** the window follows changes to the logs and new or removed account folders.
- **From the viewer:** Help > Global Accountant starts it (or brings the open window
  forward).

## Where it looks

| | user data (accounts, logs) | cache (name caches) |
|---|---|---|
| Linux | `~/.companionviewer`, `~/.firestorm`, `~/.firestorm_x64` | `<user data>/cache` |
| macOS | `~/Library/Application Support/CompanionViewer` (and `Firestorm`, `Firestorm_x64`) | `~/Library/Caches/<same>` |
| Windows | `%APPDATA%\CompanionViewer` (and `Firestorm`, `Firestorm_x64`) | `%LOCALAPPDATA%\<same>` |

An account found in more than one of these is one avatar, its logs merged.
Account folders are the ones named like `first_last` or `first_last.grid`.

## Profile pictures

The viewer's name cache (`avatar_name_cache.xml`) gives a Second Life account's
avatar key and display name. The picture comes from Second Life's public resident
page (`world.secondlife.com/resident/<key>`, checked at most once a week, kept in
the app's `pictures.json`) and `picture-service.secondlife.com`. The service
stretches the square pictures to 256 × 192, so they are shown squeezed back to
square. Other grids' accounts get a letter instead.

## Building

Needs Node.js with npm. `npm ci` once, then:

```
npm test              unit tests (src/ledger.test.js)
npm start             run from the source
./install-linux       build and install for this user: ~/Applications/slmoney and a menu entry
npm run dist:mac      dist/SL Money-<version>-universal.dmg (on a Mac; ad-hoc signed)
npm run dist:win      dist/SL Money Setup <version>.exe (on Windows, or Linux with Wine)
```

### Packaged with the viewer

`npm run bundle:linux`, `bundle:mac` or `bundle:win` builds just the program into
`dist/` (`linux-unpacked`, `mac-universal/SL Money.app`, `win-unpacked`). When that is
there, the viewer's packaging (`fs_bundle_slmoney` in `indra/newview/fs_viewer_manifest.py`)
copies it into the viewer: `slmoney/` beside the viewer on Linux and Windows,
`Contents/Resources/SL Money.app` on macOS. Global Accountant looks there first, then for
a separate install. Without it the viewer builds as usual and Global Accountant says SL
Money is missing.

The macOS app isn't signed with an Apple Developer ID: the first time, open it from
**System Settings → Privacy & Security → Open Anyway**. The Windows installer isn't
code-signed either; SmartScreen asks for **More info → Run anyway**. It installs for
the current user with Start menu and desktop shortcuts.

`build/icon.png` is the Companion Viewer icon with a banknote; `build/make-icon.py`
remakes it from `doc/companionviewer_icon.png`.
