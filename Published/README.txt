TSRE5-SCOmod Test Build v0.4

This package contains the TSRE5 executable and runtime files from the terrain/editor improvement work.

License and source:
TSRE5-SCOmod is distributed under the GNU General Public License version 3, following the original TSRE5 project by Piotr Gadecki / GokuMK and Eric's Trainsim.Com fork. See LICENSE.md.

Source code for this build is available at:
https://github.com/scottb613/TSRE5-SCOmod/tree/tsre-scomod-wip

Bundled runtime DLLs remain under their respective upstream licenses. See THIRD-PARTY-NOTICES.txt.

Highlights:
- improved track cuts and embankments
- adjusted F-tool track-width behavior for practical 1/2/3 sizing
- Shift+F terrain smoothing pass
- selected-object Ctrl+F tile conforming for selected track/road objects
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
- high-resolution branded startup splash with scaled loader/about display
- clickable Status Window control panel for common editor state toggles
- gentle delayed snapping for the Status and Navi windows
- darker Navi Window styling with readable pointer values
- improved right-side object search with ALL category defaults and Reset
- global uiScale support for larger editor fonts and proportionally wider panels
- separated sound feedback: SCOclick, SCObuzz, and SCOchirp
- Place Guard validation with automatic undo, red status-panel error flash, and click/buzz WAV feedback
- useful selected-object status readout with click-to-clear selection
- TSRE SCOmod v0.4 startup and editor titlebar branding
- F3 OSM Vector Map HTTPS/runtime fix with OpenSSL 3 DLLs
- safer F3 OSM network failure handling to avoid crash-to-desktop
- downloaded map imagery resolution set to 4096
- embedded TSRE icon in the Windows executable
- AddShortcutDesktop.cmd helper for creating a desktop shortcut

See worklist.txt for the full forum-style summary.
See fileEdit.txt for the code/project file edit ledger.

Seasonal TERRTEX note:
TERRTEX painting only writes to default TERRTEX and TERRTEX/SNOW. Mirror Season applies the same brush/autopaint trace to the paired side, but only when a matching paired source texture exists. No matching snow/default texture means no mirror paint for that stroke. With Mirror Season off, TSRE still creates safe terrain.ace placeholders on the opposite side when needed so one-season painting does not leave blank tiles.

Startup restore note:
After TSRE closes normally, it writes lastSession.json under the user's AppData TSRE folder. The startup screen's Restore Last Session button reloads the last route, main/Navi/Status window positions, and camera view. Route paint preset JSON files are also stored under the AppData TSRE folder in route-specific subfolders.

Status window note:
The Status Window now works as a compact clickable control panel for common editor states. The selected-object button reports the current broad selection type and clears the current selection when clicked.

Object search note:
The right-side object panel now uses ALL defaults for Tracks, Roads, and Other. Choosing a specific value in one of those three filters resets the others to ALL, and the search box searches only inside the active filter. The Reset button clears search text, restores the filters to ALL, and repopulates the full object list.

Sound note:
Sound feedback is intentionally separated. SCOclick.wav plays after successful guarded object placement. SCObuzz.wav plays after Place Guard rejects and undoes a placement. SCOchirp.wav plays only for deliberate user-commanded mode/status changes, such as Status Window buttons or keyboard shortcuts. Passive status refreshes stay silent to avoid duplicate sounds.

UI scaling note:
The editor now supports a global uiScale setting. The packaged setting uses uiScale = 1.15. The recommended range is 1.00 to 1.25. This scales the main editor font, menus/dropdowns, startup screen, F2/F3 style panels, object list side panel, Status Window, and Navi Window.

Place Guard note:
Place Guard validates object placement after TSRE performs its normal placement action. Rejected placements are automatically undone, the Status Window Place Guard button flashes ERROR for three seconds, and SCO_buzz.wav plays. Accepted placements play SCOclick.wav.

Current Place Guard rules:
- all guarded placements must finish on the camera tile or one immediately adjacent tile
- track-linked interactives, including signals and other track/road database items, must be started with the pointer within 3 meters of the target track or road database line
- track-linked interactives must finish within 10 meters of the sampled database elevation
- normal scenery/static objects must land on loaded terrain and within 1 meter above or below the terrain surface
- track objects and dynamic track are allowed a wider edit tolerance: 50 meters below terrain to 100 meters above terrain
- turning Place Guard off from the Status Window restores legacy placement behavior

v0.4 note:
Ctrl+F now requires one selected track or road object and conforms only that object's database vector where it crosses the current tile. The Status and Navi windows now use a gentle delayed snap against nearby window frame edges. v0.4 also includes global UI scaling and Place Guard placement validation.

F3 map note:
OSM Vector Map loading now includes the current OpenSSL 3 runtime DLLs and guards against failed HTTPS/HTTP/empty/non-XML replies. The package default mapImageResolution is 4096 for clearer per-tile map output.

Run TSRE5.exe from this folder, or run AddShortcutDesktop.cmd to create a desktop shortcut.

