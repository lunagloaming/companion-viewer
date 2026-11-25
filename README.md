<img align="left" width="100" height="100" src="doc/companionviewer_icon.png" alt="Companion Viewer logo"/>

**Companion Viewer is a free client for 3D virtual worlds such as Second Life and various OpenSim worlds where users can create, connect and chat with others from around the world.**

It is based on [Firestorm](https://www.firestormviewer.org), so everything Firestorm does, it does too. This page lists what is different. Companion Viewer is not affiliated with or supported by the Firestorm project: please don't take Companion Viewer questions to Firestorm's support groups.

## Download

Nightly builds are on the [Releases](https://github.com/lunagloaming/companion-viewer/releases) page: `.deb`, `.rpm` and Arch packages for Linux, a disk image for macOS, and an installer for 64-bit Windows. No release builds are published so far.

Nightly builds follow Firestorm's development branch, so they also contain Firestorm's own unreleased changes. They are untested: expect bugs.

## How it differs from Firestorm

### Its own name, look and settings

- One build for Second Life and OpenSim: like Firestorm's OpenSim edition, it logs in to Second Life and to OpenSim grids, and it leaves out the licensed Havok physics library that Firestorm's Second Life edition includes.
- Its own name and artwork, and an ocean-blue theme in place of Firestorm's orange (in the default Grey theme of the Firestorm skin).
- It keeps its settings, chat logs and caches apart from Firestorm's: `~/.companionviewer` on Linux, `~/Library/Application Support/CompanionViewer` on macOS, `%APPDATA%\CompanionViewer` on Windows. A Firestorm install is never touched, and nothing is copied over from it automatically.
- Firestorm's online services are switched off: the message of the day, the Avatar Welcome Pack on first login, help and wiki links (including F1), Report Problem, and links to Firestorm's support and bug tracker. Where a grid's login page is hosted by Firestorm, a login page built into the viewer is shown instead.

### Different defaults

These are the starting settings of a new install. All of them can be changed in Preferences.

- **Moving and teleporting:** letter keys walk your avatar instead of starting a chat (press Enter to chat), and double-clicking on land teleports you there.
- **Sound:** master, interface, media and music sound on; sound effects, ambient sound and voice muted; no typing sound; voice chat off.
- **Chat and radar:** nearby chat is logged. The radar reports in nearby chat only when someone comes within chat range, with a sound.
- **Fewer pop-ups:** 73 alerts start switched off. These are hints, notices about something that has already happened, teleport and web page prompts, copy-to-clipboard notices, and confirmations for things that are easy to undo (closing IMs, quitting, deleting to the trash, hiding the interface). Alerts that guard against something hard to undo stay on. Turn any of them back on in Preferences > Notifications > Alerts.
- **No invalid-keystroke tip or beep** when a text box refuses a character, or when you arrow past the end of a list.
- **Toolbar** buttons sit on the right edge only. Help buttons are hidden.
- **Help menu:** Global Accountant, Set UI Size to Default, Set Window Size... and About, the same on the login screen and once logged in. Firestorm's other Help items are under Advanced > Help (Ctrl+Alt+D shows the Advanced menu); on the login screen they are under Debug > Help, and a grid's own help and about pages under Debug > OpenSim.
- **Login screen:** the grid list is hidden. Turn on "Allow login to other grids" in Preferences > Advanced to show it.
- **Other:** RLVa is on; the default Bento idle animation plays; search includes all maturity levels; the L$ tracker records payments even while its window is closed; caches are 20 GB.

### Extra features

- **Greeted column in the radar.** Tick the people you have greeted. The ticks last for the session, even if someone leaves and comes back, and are cleared at every login. People who come within chat range after you arrive are shown in cyan at the top of the list until they are ticked. Saying "welcome" in nearby chat ticks the people you name, even when their display names use fancy lookalike letters. The automatic ticking can be turned off in the radar's options menu. A record of each session is kept in `greetings_log.txt` in your account's folder.
- **Money log.** Every change to your L$ balance is written to `money_log.txt` in your account's folder: the time, the change, the balance after it, who it came from or went to, and what for.
- **Global Accountant** (Help menu) opens the Global Accountant app, which comes with the viewer and reads the money logs of every account you have logged in with on this computer. It lists each account's balance, largest first, with its Second Life profile picture and display name. Double-click an account to see its income and expenses, one row per avatar who paid it or was paid, with the number of payments, the total and the latest date; click a row to see each payment. It updates as the logs change, and only reads them. It keeps running after the viewer quits, and choosing Help > Global Accountant again brings its window back. On Windows it also has its own entry in the viewer's Start menu folder.
- **Dockable windows.** Windows dock to the edges of the viewer window, as panels do in code editors, so they don't cover up your view of the world or your HUD attachments: a docked window takes its space from the view instead of floating over it. Right-click a window's title bar and choose an edge, or drag the window by its title bar until the mouse pointer reaches an edge of the viewer window. Several windows can share an edge: drop one onto a docked window to put it beside that one, and drag the edge between them to share the space differently. The first edge you dock to runs the full length of the window. Drag a docked window by its title bar to free it again. Windows too big for an edge, and a few that don't suit one (such as Preferences), stay floating; Debug Settings > `FSWindowEdgeDockExcluded` lists them, and `FSWindowEdgeDockByDragging` turns off docking by dragging.

### Video memory and speed

- **Sharper textures with less video memory.** On graphics cards with OpenGL 4.3, world textures are compressed (BC7) as they load and take about a quarter of the video memory, so more of the scene stays sharp. Each texture is checked first, and any that would visibly lose quality stay uncompressed. macOS is limited to OpenGL 4.1, so this is off there.
- **Less blurring and re-sharpening.** Texture memory is counted accurately, and the texture budget comes from what is actually free on the graphics card. On Linux it also stays within the system memory that is available.
- **Several viewers at once.** Each running copy gets its own cache folder, and the copies share the texture budget and the graphics card instead of each aiming for all of it.
- **Less memory use,** especially in crowds: unused avatar textures are no longer uploaded, geometry is uploaded through a small staging buffer, and on Linux freed memory is handed back to the system at once.
- **Fewer stutters near some attachments.** Swaying flexible prims and attachments that hide and show parts of themselves (flipbook animations) no longer rebuild the whole attachment many times a minute.

### Fixes

- **Linux:** saved logins are no longer lost when the viewer starts with a different network connection up (wired or wireless) than last time.
- **Linux, release builds:** the file picker no longer stays on screen after Cancel.

## Source and licence

Release builds are derived from Firestorm release `Firestorm_Release_7.2.5.81788`; nightly builds from Firestorm's development branch. Firestorm is by The Phoenix Firestorm Project, Inc., and is itself derived from the official [Second Life](https://github.com/secondlife/viewer) client, whose code has been open source since 2007. All are available under the LGPL; see [doc/LICENSE-source.txt](doc/LICENSE-source.txt), [doc/LGPL-license.txt](doc/LGPL-license.txt) and [doc/LICENSE-logos.txt](doc/LICENSE-logos.txt). Firestorm's own README is kept as [README-Firestorm.md](README-Firestorm.md).

## Building

- [Windows](doc/building_windows.md)
- [Mac](doc/building_macos.md)
- [Linux](doc/building_linux.md)
- [Global Accountant](https://github.com/lunagloaming/companion-viewer/blob/companion-viewer-nightly/tools/slmoney/README.md), the money log app (Node.js and Electron)

## Community respect

This project is guided by the [TPV Policy](https://secondlife.com/corporate/third-party-viewers) and the [Second Life Code of Conduct](https://github.com/secondlife/viewer?tab=coc-ov-file).
