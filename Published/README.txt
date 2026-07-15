TSRE5-SCOmod Test Build v0.3

This package contains the TSRE5 executable and runtime files from the terrain/editor improvement work.

Highlights:
- improved track cuts and embankments
- adjusted F-tool track-width behavior for practical 1/2/3 sizing
- Shift+F terrain smoothing pass
- tile-level track, road, and water autopaint
- real shoreline/water-edge terrain painting
- route and tile terrtex paint reset tools
- F2 terrain paint presets saved per route
- 0-360 degree terrain texture rotation control
- F2 seasonal selector: Summer/Spring/Autumn/Winter/Night
- seasonal fallback refresh for terrain, objects, transfers, dynamic track, and forests
- Mirror Season option for paired default/snow TERRTEX painting
- old settings-file season/seasonalEditing controls disabled
- texture paint resolution protection for 1024 terrtex files
- forest/object stutter mitigation pass
- startup guard to prevent multiple TSRE instances
- fixed route-selection table refresh when switching MSTS roots
- Restore Last Session button for reopening the last route, camera view, and editor layout
- embedded TSRE icon in the Windows executable
- AddShortcutDesktop.cmd helper for creating a desktop shortcut

See worklist.txt for the full forum-style summary.
See fileEdit.txt for the code/project file edit ledger.

Seasonal TERRTEX note:
TERRTEX painting only writes to default TERRTEX and TERRTEX/SNOW. Mirror Season applies the same brush/autopaint trace to the paired side, but only when a matching paired source texture exists. No matching snow/default texture means no mirror paint for that stroke. With Mirror Season off, TSRE still creates safe terrain.ace placeholders on the opposite side when needed so one-season painting does not leave blank tiles.

Startup restore note:
After TSRE closes normally, it writes lastSession.json under the user's AppData TSRE folder. The startup screen's Restore Last Session button reloads the last route, main/Navi/Status window positions, and camera view. Route paint preset JSON files are also stored under the AppData TSRE folder in route-specific subfolders.

Run TSRE5.exe from this folder, or run AddShortcutDesktop.cmd to create a desktop shortcut.
