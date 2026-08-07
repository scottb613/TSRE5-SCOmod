# Qt5-to-Qt6 regression matrix

The frozen v0.8 installation is the accepted behavior reference. Qt6 results
combine the recorded automated evidence, focused manual checks, extended TST
use, and the operator's final v0.9 acceptance on 2026-07-31. Rows without a
separate v0.8 rerun are marked `Reference`; they are not outstanding tests.

| Area | Test | Qt5 baseline | Qt6 result | Evidence/notes |
|---|---|---:|---:|---|
| Startup | Route Editor starts and restores session | Reference | Pass | Controlled Qt6 package opened the reference route with the expected GenX scene and panels, 2026-07-30 |
| Startup | Consist Editor starts | Reference | Pass | Controlled Qt6 package launched and initial visual inspection found no issue, 2026-07-30 |
| Startup | Shape Viewer starts | Reference | Pass | Controlled Qt6 package launched; route objects and other rolling stock rendered correctly, 2026-07-30 |
| Route | Load reference route without errors | Reference | Pass | Route Loader and Route Editor opened the reference route; no issue reported during initial inspection |
| Route | Save, close, reload | Reference | Pass | Full 2,180-file manifest: 2,175 unchanged, four timestamp-only key files, and 12 last-decimal TDB float normalizations. Reload passed; archived v0.8 atomic history shows the identical inherited 12-token drift |
| Recovery | Interrupted route save recovers safely | Reference | Pass | Closed by operator acceptance after extensive v0.9 route-save and recovery testing, 2026-07-31 |
| Terrain | Height brush and conform tools | Reference | Pass | Closed by operator acceptance after extended v0.9 terrain-editor use, 2026-07-31 |
| Terrain | Main/Snow painting and mirror validation | Reference | Pass | Basic shoreline texture painting operated. Visible brush-dab spacing is inherited behavior and was accepted for v0.9 without changing the paint algorithm |
| Terrain | 1024x1024 TERRTEX remains full resolution | Reference | Pass | Closed by operator acceptance after extended v0.9 terrain and texture testing, 2026-07-31 |
| Textures | ACE display and refresh | Reference | Pass | Closed by operator acceptance after extended Route Editor, Shape Viewer, and Consist Editor use, 2026-07-31 |
| Textures | DDS DXT1/DXT3/DXT5 display | Reference | Pass | Native Qt6 decoder probe passed synthetic DXT1/DXT3/DXT5 color, transparency, alpha, truncation, and uncompressed 24/32-bit checks. Operator TST inspection confirmed DDS rolling-stock textures work correctly in Consist Editor, 2026-07-31 |
| Textures | Custom SCO_RS3_1510 Shape Viewer case | Reference | Pass | The blank model was caused by the Qt6 DDS loading gap. Its valid DXT5 skin renders through the restored DDS decoder; closed with the operator's DDS acceptance, 2026-07-31 |
| Track | Static placement and TrackDB rebuild | Reference | Pass | Closed by operator acceptance after extensive v0.9 TrackDB and editing tests, 2026-07-31 |
| Track | Classic Dynamic Track | Reference | Pass | Closed by operator acceptance after extensive v0.9 track testing, 2026-07-31 |
| Track | NextGen Auto-Flex and reverse-direction train test | Reference | Pass | Closed by operator acceptance after extensive v0.9 Auto-Flex and reverse-direction train testing, 2026-07-31 |
| Track | Grade lock, helper, and symbols | Reference | Pass | Closed by operator acceptance after extended v0.9 track-tool use, 2026-07-31 |
| Paths | New/Edit/Clone PAT workflow | Reference | Pass | Closed by operator acceptance and the focused Activity Path Editor evidence, 2026-07-31 |
| Paths | Reverse, wait, passing, and advanced shunting points | Reference | Pass | Closed by operator acceptance and the focused Activity Path Editor evidence, 2026-07-31 |
| Objects | Move existing scenery object, save, close, reload | Reference | Pass | One ordinary scenery object was deliberately moved on the backed-up SCO_LHR test route; its new position persisted after a satisfactory save/reload test, 2026-07-30 |
| Objects | Place ordinary scenery object, undo, save, close, reload | Reference | Pass | No world file changed and the temporary object remained absent after reload. Only seven bounded TDB float normalizations remained, matching inherited no-edit behavior, 2026-07-30 |
| Objects | Retain placed ordinary scenery object through save/reload | Reference | Pass | Exactly one world file gained one complete 240-byte UTF-16 Static record for tree1.s, UID 48; the object remained visible after reload, 2026-07-30 |
| Objects | Delete retained ordinary scenery object through save/reload | Reference | Pass | The same world file lost exactly the 240-byte UID 48 record; UID 47 remained and no unrelated world file changed, 2026-07-30 |
| Objects | Search and Place Guard | Reference | Pass | Search/Reset behaved correctly. A signal attempted away from TrackDB was rejected and automatically undone; reload retained no Signal, TrItemId, signal-unit data, or new UID, 2026-07-30 |
| Maps | OSM and configured imagery providers | Reference | Pass | Closed by operator acceptance after extended v0.9 map-provider use, 2026-07-31 |
| Settings | JSON save, backup, reset, damaged-file recovery | Reference | Pass | Closed by operator acceptance after extended v0.9 settings and session-restoration testing, 2026-07-31 |
| UI | Scaling, panels, pinning, and restored positions | Reference | Pass | Main smoke test passed with no showstoppers; application-wide captions verified. Terrain Brush Settings fields received a font-aware width, Size/Intensity/Max Radius cap at 99, and clipping was visually confirmed fixed. Combo-box popups were verified anchored after native-caption handling stopped forcing premature QWidget native handles |
| UI | Route Editor helpers close with the editor | Reference | Pass | Complete helper ownership and shutdown behavior accepted by the operator in the approved production build, 2026-07-31 |
| UI | Load with no route selected plays rejection sound only | Reference | Pass | Rejected Load sound behavior accepted by the operator in the approved production build, 2026-07-31 |
| UI | Water Helper Above bed arrows use 0.25 m steps | Reference | Pass | The 0.25-meter step behavior was accepted by the operator in the approved production build, 2026-07-31 |
| Test infrastructure | Route regression evidence harness | N/A | Pass | Automated baseline/compare workflow inventories the full route, preserves mutable route data, analyzes terrain samples and binary changes, checks Undo/save logs, and emits machine-readable CSV/JSON evidence. A CTest fixture verifies exact no-change and one-sample terrain-change detection |
| Path Editor | Create, save, reload, and extend a path with reverse and wait/advanced controls | Reference | Pass | Automated comparison found only `GENX09_TEST.pat` changed; all other 2,180 files, including TrackDB/RoadDB and both item tables, remained exact. The path grew from 11 to 17 nodes and 9 to 15 PDPs, retained one reverse, and serialized five wait/advanced controls. Switch accept/reject and point-add sounds passed |
| Camera | Terrain Lock blocks direct vertical descent through terrain | Reference | Pass | CameraFree vertical descent now invokes the existing terrain clamp. LOCK stopped at the established two-meter clearance; FREE continued to permit underground movement, 2026-07-30 |
| Packaging | Clean Windows 10 1809+ launch | Reference | Pass | Supported-Windows production package accepted by the operator after extensive TST use, 2026-07-31 |
| Packaging | Clean Windows 11 launch | Reference | Pass | Supported-Windows production package accepted by the operator after extensive TST use, 2026-07-31 |
