# Gate 4 Main smoke test

This is a no-save interactive test of the Qt6 Route Editor in the controlled
`TSREvcTST` runtime. Do not deliberately alter or save route content during
this pass.

Launch `Open-Route-Editor-Test.bat`, then check:

1. The saved route and camera position restore normally.
2. Keyboard movement, fast/slow movement, mouse look, and mouse-wheel zoom
   respond normally.
3. Clicking a known scenery object selects it and updates its properties.
4. Selection highlighting, the 3D pointer, compass, markers, track lines, and
   tile/world-grid overlays can be toggled without display corruption.
5. Object Selection opens; ref-file/category filters, search, and list
   selection respond.
6. Terrain, Track, Activity, Geo, and object/property panels open and close.
   Do not apply a brush, placement, database, or save operation yet.
7. Floating panels can be moved, pinned/unpinned, closed, and reopened.
8. Menus, dialogs, and native captions remain readable at the current UI
   scale and monitor arrangement.
9. Exit without saving, then inspect the newest `TSREvcTST/tsre-log-*.txt`
   for Qt connection, OpenGL, shader, fatal, or crash messages.

Passing this test establishes interactive startup and legacy-renderer parity.
Editing, serialization, recovery, terrain painting, TrackDB, Dynamic Track,
and path operations remain Gate 5 tests on disposable content.

## Result

Passed on 2026-07-30 with no showstoppers. Camera, rendering, panels, overlays,
and terrain interaction were exercised. Two follow-ups were recorded:

- Brush Settings used hard-coded 25-pixel value fields that clipped under the
  enlarged panel font. The fields now calculate a scaled, font-aware width;
  Size, Intensity, and Max Radius cap at 99. The rebuilt UI was visually
  confirmed fixed.
- Shoreline AutoPaint showed noticeable individual brush dabs. The terrain
  paint implementation is unchanged from frozen v0.8; only the required Qt6
  mouse-position accessor changed. Keep this as a provisional Gate 5 behavior
  comparison rather than changing the paint algorithm during migration.
