# TSRE GenX v0.23 Test Matrix

Opened 18 November 2026. Fresh evidence only. v0.22 remains capped.

| Check | Status |
| --- | --- |
| Version identity and Open Rails source comparison | PASS |
| CMake configure and compilation with the parity change | PASS, MSVC Release configure and clean application build |
| Rebuilt MSVC Release shape-path, glTF model, glTF graphics and DDS decoder probes | PASS, 4/4 |
| Existing MSVC Release shape-path and DDS-decoder probes | PASS, 2/2 on the 18 November binaries |
| Clean MinGW and MSVC Release verification | PASS, 39/39 per lane across the suite and separate corpus run |
| Working-pack checks | Earlier MSVC working packs matched their corresponding builds. See final verification below for the selected release executable. |
| Manual editor and Open Rails comparison | PENDING |

## Prepared automated coverage

The shape texture probe covers shared geometry with separate wagon-local
repaints, nested shape paths, explicit shared image paths, texture misses when
the shared shape directory has the only copy, LoadData Shape-derived roots,
ACE/DDS substitution and priority, rejection of PNG and drive-qualified
image names, seasonal selection, and fallback through recognized seasonal
directories. An unreadable preferred DDS path must remain the selected path.
The existing scenery texture lookup cases remain in the probe.

## Required manual checks

Use the same disposable stock in Open Rails and the Consist Editor. Compare
the main body, legacy FreightAnim and ORTS FreightAnimStatic for these cases.

- Two wagons share one shape and each has a different local repaint. Both
  previews should show the correct artwork, including after reload.
- Put a basename-only image exclusively beside a shared sibling shape.
  Both programs should leave the material's texture missing. Then give
  the shape an explicit relative image path and confirm both find it.
- Give the same material an ACE and DDS, then corrupt the DDS. Both programs
  should prefer the DDS and report a missing or failed texture when it is
  unreadable. Also check a legacy shape that references a PNG directly.
- Use a LoadData definition whose folder differs from the first directory
  in its Shape field. Put textures in the Shape-derived root and also try
  an image stored exclusively beside geometry in a deeper subfolder.
- Spot-check the same wagon in Route Editor on-track display, reload, and a
  seasonal texture set. Compare using matching Open Rails season and weather.

## Embedded glTF and GLB texture checks

The rebuilt MSVC Release shape-path, model, graphics and DDS probes passed 4/4.
The updated model probe
checks self-contained .gltf and .glb buffer views for PNG (including alpha),
JPEG and DDS, plus MIME mismatch, dual sources, the pixel budget and required
MSFT_texture_dds selection. It also checks the result for a direct image data
URI and a damaged DDS with an optional PNG source.

The local Open Rails runtime loader parsed an in-memory .gltf with
image/vnd-ms.dds and read its image buffer view byte-for-byte. This verifies
its schema and image stream support. Live rendered acceptance is pending.

With the rebuilt Consist Editor and Open Rails, open the same wagon body and
freight model using embedded PNG/JPEG/DDS. Move the model files to a clean test
folder without texture sidecars. Confirm colour, PNG alpha, correct DDS
selection, and texture refresh after reloading. Run the focused Release CTest
probes tsre_gltf_model_probe, tsre_gltf_graphics_probe and tsre_dds_decoder_probe.

## Large GLB rolling-stock preview

A reported GLB and its crew variant declared BIN chunks of approximately
278 MiB and 292 MiB. Inspection found supported 32-bit RGB(A) and DXT5
embedded DDS images, with estimated decoded images below the existing
256 MiB CPU and GPU budgets. The earlier build rejected the BIN chunk before
texture decoding.

The preview now maps GLB input read-only and validates the total length in
the GLB header. The decoded-image and external-buffer allocation budget
does not include in-container BIN data. A bounded heap fallback still
handles input when mapping is unavailable. The updated model probe puts a
DDS above 340 MiB in a synthetic GLB and checks decoding and mapping release.
It also checks that the separately loaded buffer budget still applies.

Pending operator incremental build and rebuilt model/graphics CTest.
Recheck the reported body and crew variants in the Consist Editor after
promotion. Verify both embedded textures and model geometry are visible,
and reload the model to check that the file mapping was released.

## Transform keyboard confirmation

Pending operator rebuild and manual verification. In Route Editor, open
Transform for a selected object. Enter a nonzero Translate value and press
Enter while the field has focus. Repeat with a Rotate value and numeric
keypad Enter. Verify the popup closes, the action applies once, and one Undo
restores the original transform. Verify mouse OK and Cancel continue to work.

## Remove DYNATRAX

Pending operator incremental MSVC build and manual acceptance on disposable
route content. Open Hacks with a fixed or native Dynamic Track selection.
Verify a clean route reports no DynaTrax and that selecting No preserves all
files. On a converted fixture confirm the route-wide totals, then select Yes.
Check native track geometry, preserved UiDs and TrackDB section references,
unchanged database and shared-texture hashes, and removal of generated shapes
and descriptors. Reopen in TSRE and Open Rails. Check that missing local
sections, mismatched geometry, binary DynaTrax placements, and surviving
metadata references stop the operation before writing.

Existing MSVC Release route-section, route-save and world scan probes passed
3/3. The regenerated route-save probe must cover the new removal, rollback and
changed-input cases.

## Track-path bounds and unsaved databases

Pending incremental MSVC build and manual checks. On a route copy, verify a
valid 12-section path loads and malformed, oversized or truncated TrackPath
and Global SectionIdx records stop loading with an error. The parser probe
exercises the production reader and checks that a failed read leaves its
output record untouched.

Open a clean route and close it to confirm there is no spurious save prompt.
Repeat after Remove Vector or Remove Branch without any world or terrain
edit. Verify the TrackDB or RoadDB entry in the close prompt, and confirm
Undo to the saved state removes that entry. Check that a failed save and
disabled database writing leave database changes pending, and a successful
key-file save clears them. Include a route-local track-section change.

## Operator verification

Run AAA_Compile.ps1 -Compiler MSVC -Clean to configure, compile and promote the
matched v0.23 working runtime. Run Release CTest in the rebuilt tree with regex
^tsre_shape_texture_path_probe$. The MinGW parity lane and side-by-side live
checks are pending. Test results from the earlier executable are baseline only.

## Route editor vegetation instancing

Existing MSVC Release forest and glTF graphics probes passed 7/7 on the
previous build. Static checks compared the old and instanced forest vertex
positions and UVs for 1,000 sizes and offsets. The source checks also verified
the vegetation inputs in both shader variants.

After incremental configuration and compilation, repeat the focused CTest
run. Open a view with a dense forest region, two consecutive raw plants of
one single-part species, a multi-part species, and baked vegetation. Compare
with the same view in a second run using TSRE_DISABLE_VEGETATION_INSTANCING=1.
Verify forest positions, winding, track and road clearance, selection color,
selection boxes, shadows, resized regions, seasonal textures, and the
forest and PolyVeg visibility toggles. Verify raw tint, saved raw plants after
reload, isolated plants, and baked vegetation. Check overlapping foliage
while moving the camera. Compare steady-state frame time and inspect the
vegetation draw calls with warmed textures. Runtime verification is pending.

MSVC Release configured and compiled successfully, and the promoted
executable matched the build SHA-256. The focused Release CTest run passed
7/7. The compiled editor reached the viewport with instancing enabled and
disabled in both the forest and the staged raw PolyVeg views. The in-editor
profiler subsequently recorded Forest and raw PolyVeg instanced draws in
their respective enabled captures and zero instanced draws with the switch
disabled. The raw stress view reduced draw calls and improved the upper
tail of paint times, while its median paint time increased. Forest build and
shape loading were still in progress during the Forest comparison, so it
does not establish a steady-state frame-time gain.

A repeat MSVC Release check on the same default-terrain opening view
showed similar frame times in both modes and zero eligible instanced draws.
The existing vegetation, graphics, shape texture path, and file-reading
probes passed 9/9. The detailed visual checks above remain pending.

## Route editor frame and loading profiler

After the incremental build, launch the Route Editor with
TSRE_PROFILE_EDITOR=1. Check the log for the OpenGL and vegetation
configuration, an opening-frame sample, and rolling summaries. Open a dense
view and move into uncached world tiles. Check shape reads, texture lookups
and uploads, and load-budget deferrals when applicable. Check that these
settle at a stationary view. Timings are inclusive and can overlap. Paint
time is CPU wall time. Swap intervals measure Qt widget presentation
cadence.

Repeat on the same camera with TSRE_DISABLE_VEGETATION_INSTANCING=1 and compare
warm paint and swap distributions, vegetation draw counts, and the time for
deferred loads to clear. Confirm the profiler is silent without its flag.
MSVC Release configured, compiled, and promoted with matching hashes.
Focused texture, forest, and graphics CTest probes passed 9/9. Live trials
captured first paint, rolling swap intervals, and object load deferrals.
Same-view forest and isolated raw vegetation trials confirmed that the
instanced draws appear when enabled. The route-specific timing logs are
local. Visual parity and the profile-disabled smoke check remain pending.

## Opening view loading

Verify that a local route opens with a responsive progress bar, a blank 3D
view during asset work, and the saved or route-default camera on completion.
Exercise routes with distant terrain, hidden or cut-out patches, detail
textures, forests, transfer shapes, and glTF scenery. Rotate 360 degrees
in place immediately after the progress bar closes and check that terrain
textures are already present in every direction. Then cross into new tiles
and check that their terrain textures stream in and match the configured
season.
Verify picking, offscreen brush spill, terrain painting and save/reload on
a route copy. Verify returning to Main Load and then opening another route,
and check that the server route still opens through its usual streaming path.

With TSRE_PROFILE_ROUTE_LOAD and TSRE_PROFILE_EDITOR set, confirm that
the first presented frame is the scene, following discovery and asset
preparation. Compare startup texture loads, first real presentation,
and post-opening shape deferrals at a fixed camera.

MSVC Release configured and compiled, and the promoted executable matched
the build hash. Six focused terrain, texture, glTF and forest probes passed.
The live startup trace logged discovery and texture preparation before the
first 3D presentation. No shape-budget deferrals occurred in the subsequent
capture. Route-specific timing evidence remains local. The other visual,
editing, and server checks above remain pending.

## Resident terrain textures during streaming

Cross into an uncached tile that uses the same default base and detail
textures as the current tile. With other terrain uploads using up the
frame budget, the default patches should still render both resident
textures. Check that uncached painted patches appear as their loads finish
and that seasonal and ACE-to-DDS aliases still resolve.

MSVC Release compiled and promoted with matching hashes. All three focused
terrain and texture probes passed. The live tile crossing showed textured
default ground in the first captured view, while painted patches filled in
over later frames. The trace recorded painted terrain decodes after crossing.
Seasonal alias parity remains a separate manual check.

## Background terrain DDS decoding

On a route with painted DDS terrain, move past the opening preload area.
Verify that resident default terrain stays textured, completed painted DDS
textures appear, and the editor profile records background submissions and
completions. Compare paint and swap timing against the synchronous route
trace. Check that OpenGL uploads still run under the normal frame budget.

Move away from pending patches, revisit them, and reopen the route to check
that the bounded stream recovers. On a route copy, paint a patch whose DDS
is still pending, reload a converted texture, and save and reload the route.
Verify that edited or replaced pixels win and that RGB/RGBA and seasonal
variants remain correct. Compile and live checks are pending.

## Aliased materials and background shape reads

Profile a warm opening view with an ACE material resolved to a DDS. Confirm
subsequent draws reuse the recorded alias and check the texture lookup count
and frame times. Check changing the material, texture reload, and an ACE
material with no DDS alias.

Cross from the opening view into uncached shapes with TSRE_PROFILE_EDITOR=1.
Compare the same crossing with TSRE_DISABLE_BACKGROUND_SHAPE_READS=1. Check
worker submissions and completions, synchronous read timings, and swap
intervals. Confirm newly loaded scenery is textured, selectable, and casts
shadows. Verify missing descriptors, compressed and uncompressed shapes,
shape reload while a read is pending, and rapidly crossing away and back.
Confirm the opening load, Shape Viewer, and Consist Editor still work.

The MSVC Release build and promoted executable matched. Five focused
shape, texture, and graphics probes passed. The warm route trace confirmed
that recurring lookup scans disappeared and median paint time decreased.
In the same tile crossing with workers enabled and disabled, the background
run recorded completed reads and no synchronous shape reads during the
load burst. Its worst presentation gap was shorter. Terrain preparation and
shape parsing still produced editor-thread spikes. Detailed editing and
viewer checks above remain pending. Route-specific traces are local.

## Hidden loading on route jumps

Jump to an unprepared destination from the Control Panel. Check that a
blank frame appears before destination terrain work starts, progress is
visible during discovery and preparation, and the first scene has textured
terrain and nearby scenery. Turn in place after landing. With route-load
profiling enabled, check that the jump blank frame precedes destination
discovery, preparation, and presentation. Confirm the arrival chirp follows
the first displayed destination frame.

Jump away and return to the same prepared location. Check that the return
and a same-tile jump are immediate. Repeat with activity and error-message
jumps, a missing terrain tile, and after switching routes. Check that ordinary
movement still streams scenery, the startup loader still reports progress,
and server-route jumps still follow their existing path. Operator build and
live verification are pending.

## Routine compile target and CTest registration

Run the incremental MSVC Release compile without switches for additional
targets. Verify the build log selects TSRE5, builds its dependencies, and
promotes the executable with a matching hash. Repeat with -WithProbes and
compare the registered CTest names and fixture paths with the prior tree.
Run the configured release gate in both compiler lanes when preparing a
release. Verify an exported source tree finds the test module and builds the
forest-patch helper. Configure and build verification are pending.

## Painted terrain previews and background terrain mesh preparation

Build the DDS decoder and terrain grid probes. Verify the decoder samples a
stored mip, rejects a truncated mip, and detects a DDS with no stored mip.
Verify terrain triangle packing, winding, normals, UVs, bounds, and gap
padding. Compare 256- and 512-sample terrain and a distant mountain tile
in the editor, including the map overlay and terrain picking.

Open a painted route with TSRE_PROFILE_EDITOR and TSRE_PROFILE_ROUTE_LOAD.
Check that the first scene and a 360-degree turn show the available painted
previews. Verify one DDS with no mips produces an in-memory preview without
changing the file. Verify nearby textures sharpen after first presentation.
Cross into a new tile and check that detail is released outside the camera
tile and its eight neighbors. Record video memory and upload stalls. Repeat
the crossing with TSRE_DISABLE_BACKGROUND_TERRAIN_MESH=1 and compare the
terrain snapshot, mesh upload, and presentation timing. Shape parse timings
include the separately reported shape graphics timings.

On a route copy, paint manually and along TrackDB and RoadDB vectors, including
a vector crossing a tile boundary. Check full-resolution edits, texture
reload, save and reopen, undo, seasonal mirrors, terrain gaps, and altitude
edits during a pending mesh. Operator build and live checks are pending.

## Native opaque DXT1 for painted TERRTEX

Run the DDS decoder probe with top-level and preview BC1 payload checks,
truncated mip rejection, and transparent-block fallback. In the editor,
compare a painted route with opaque DXT1 ACE and DDS materials with
TSRE_DISABLE_NATIVE_TERRAIN_DXT1=1 and without the override. Check first
scene previews, full detail, VRAM, compressed upload counts and bytes,
and peak frame gaps. Exercise a zlib-wrapped ACE, a transparent DXT1 DDS,
and a painted ACE or DDS without authored mips. Verify individual and
TrackDB/RoadDB vector painting, undo, save/reopen, and seasonal textures
still use the original data. The ordinary shared texture path is unchanged.

Repeat the same tile crossing at the same camera position. Compare
terrain_border_read with shape_parse_inclusive minus its nested
shape_graphics total, and use frame gaps to decide which remaining task
to move off the editor thread first. Build and live verification are pending.

## Cancel and resume Process All TERRTEX

On a disposable route copy with at least two uncompressed textures and an
existing DXT1 texture, cancel during compression. Verify the active texture
is committed, the following texture is unchanged, and the editor reports
cancellation. Rerun with the checkpoint's settings. Confirm completed
outputs are reused and the existing DXT1 file is unchanged. Repeat with
format conversion and check that a cancellation before descriptor updates
preserves originals and that resume completes references and cleanup.
Also cancel during scanning and during the descriptor pass. Build and
live editor verification are pending.

## Route Editor panel alignment

Open F12 → UI and check that Panel Alignment defaults to Center on settings
without the new key and with an invalid value. Save Left, Center, and Right
while a route is open. Verify the layout updates immediately, preserves the
Tools and Properties widths and visibility, and keeps the selected object
and viewport active. Check that the separator appears between grouped
panels and is hidden when either is closed.

Restart TSRE and verify that the selection persists. Test with narrow and
maximized windows at UI scales 0.75, 1.0, and 1.25, including opening and
closing Tools, Properties, and auxiliary docks. Operator build and live UI
checks are pending.

## Full-resolution painted terrain around the camera

On a route with heavily painted ACE and DDS terrain, hold the camera still
for several seconds, then turn through 360 degrees. Compare preview
visibility and frame gaps against the previous build. Repeat after crossing
a tile boundary and after a jump. With Tile LOD at 2, verify that painted
patches in the second ring can reach full resolution as the cache allows.
Repeat at Tile LOD 1 to check prefetch in the normal 3-by-3 view. Check
native DXT1 and CPU uploads, nearby terrain painting, and texture refresh
after a reload. Three focused probes passed on the pre-change MSVC Release
build. The subsequent operator build matched the promoted executable hash.
A live marker jump confirmed destination loading and full-resolution native
DDS uploads through the cache limit. The stationary view settled after
streaming. The controlled turn, continuous crossing, CPU upload, and
editing checks are pending.

## Terrain cache admission and scenery draw profiling

Build TSRE5 and the DDS decoder probe. The probe checks that upload
estimates come from the top mip, use the active upload mode, and ignore
a truncated header. The operator's MSVC Release build passed and the
promoted executable matched its hash. The rebuilt DDS decoder probe passed
through CTest.

With TSRE_PROFILE_EDITOR enabled, repeat a painted route's startup,
uncached marker jump, and continuous tile crossing. Compare first paint,
total time to presentation, and scenery uploads during hidden loading and
the first real frame. Saturate the full-resolution cache and compare
terrain_full_preflight_rejected, terrain_full_rejected_after_read,
terrain_dds_completed, and terrain_full_uploaded. Repeat with native
DXT1 disabled and with missing or slow scenery textures. Check that the
closest painted patches still promote as the camera moves.

On a stationary dense view, repeat with TSRE_PROFILE_SHAPE_DRAWS enabled
as well. Verify that the ranked shape and material counts cover ordinary
S-file draws in the world pass, excluding instanced draws. Use separate
runs with the draw breakdown off for frame-time comparisons.

A live before-and-after startup and marker jump showed fewer first-frame
texture uploads and fewer completed terrain reads at the same full cache
occupancy. The separate draw-count run emitted the ranked ordinary S-file
shape and material counts in the destination view. Timing logs remain local.
The continuous crossing, CPU fallback, and slow-decode cases are pending.

## Repeated opaque scenery instancing

Use the same dense view on two fresh sessions with TSRE_PROFILE_EDITOR
enabled. In the second session set TSRE_DISABLE_STATIC_SHAPE_INSTANCING=1.
Keep vegetation instancing on in both. Compare world paint time, swap
intervals, shape_draws, static_shape_instanced_draws,
static_shape_primitive_instances, and static_shape_placements. The
difference between primitive instances and instanced draws is the number
of shape draw calls saved. Check the loading and stationary cases.

Visually compare repeated multi-part buildings with different rotations or
scales, blended scenery, glTF, forests, and raw PolyVeg. Pick, translate,
rotate, copy, and delete individual copies on a disposable route. Check
selection boxes, pivot and snap markers, and the shadow pass.
The existing glTF graphics, shape texture lookup, forest definition, and
ACE probes passed before rebuilding. The MSVC Release build matched the
promoted pack. A live same-path startup and camera crossing confirmed the
toggle and draw savings. The crossing hitched with both settings and the
stationary destination showed no frame-time gain in that pair. The local
logs retain the measurements. Editing and visual parity checks are pending.

## Background legacy shape parsing

Repeat the same camera crossing into uncached tiles with the existing
TSRE_PROFILE_EDITOR capture. Compare frame-gap percentiles and worst gaps
against the recorded build with background reads only. Check that
shape_worker_parse records completed worker timings and shape_graphics
records only render-thread buffer work. Repeat with
TSRE_DISABLE_BACKGROUND_SHAPE_READS=1 to check the synchronous fallback.

Check binary and text MSTS shapes, animations, picking, subobject toggles,
seasonal texture reload, repeated scenery, a missing shape, and switching
routes while workers are busy. Existing read-file, Unicode parser, shape
texture, and graphics probes passed. The operator's MSVC Release build
matched the promoted executable. The matched live tile crossing showed
lower paint and presentation delays with background parsing, and the
same-build synchronous control moved parsing back to the editor thread.
Route-specific traces remain local. The remaining asset and editing parity
checks above are pending.

## Grade symbols during tile streaming

A matched crossing on the previous build showed lower worst-frame delay
with Grade Symbols hidden. The separate TrackDB and Tsection visibility
trial also reduced the worst gap. Timing evidence stays local.

After building, restore the overlays and repeat the same crossing with
TSRE_PROFILE_EDITOR enabled. Check that the world tile loads finish
before grade-cache rebuilding and that each database used by the view
rebuilds at most once in that frame. Compare the worst gap and the
same-frame world, grade-cache, track overlay, and terrain upload timings.
Check grade symbol transitions across tile borders, grade edits and undo,
overlay toggles, picking, and startup and jump views. The four focused
TrackDB and terrain probes passed on the previous build. The operator's
MSVC Release build matched the promoted executable. The matched live
crossing with all three track overlays enabled reduced the worst gap.
The slowest frame recorded one grade-cache rebuild, and the track overlay
was its largest measured substage. Editing and jump parity checks are
pending. The exact trace remains local.

## Track overlay geometry reuse

Repeat the matched crossing with Grade Symbols, TrackDB Lines and Tsection
Lines enabled. Compare worst-frame gap and world_track_overlay time with
the prior matched profile. Check that trackdb_overlay_build is absent on a
normal tile crossing and track_section_overlay_build only covers newly
visible tiles or dynamic owners that became available. Check
track_overlay_labels for first-time label work.

Check both databases, a return crossing, a long jump, grade and line
alignment at tile seams, and labels near the camera. Edit and undo a track
or road segment on a test copy and verify the overlay geometry updates.
Check remote database updates and dynamic track whose owning world tile
loads late. Four focused probes passed on the previous build. The
operator's MSVC Release build matched the promoted pack. The matched
crossing kept all three overlays enabled and reduced their cost and the
frame gap at the boundary. The route-wide buffers were reused. A later
frame had a larger presentation gap, mostly outside the timed paint.
The remaining visual and editing checks are pending. The trace remains
local.

## Performance release readiness

Ten focused MSVC Release CTest probes passed on the promoted build,
covering texture, terrain, track, parser, and graphics behavior. The
warning cleanup in the touched overlay and profiler code is included in the final release builds. Before release, verify both supported Release lanes with the full
registered suites. CPU TERRTEX fallback and the live editing parity
checks above remain open.


## Final Release verification

Both supported Windows Release environments configured and built cleanly on
21 November 2026. In each lane, the full registered CTest run passed 38 tests
and skipped the optional ACE corpus because its input was unset. A separate
run passed in each lane with 68 exact matches out of 68 reference textures.
Both lanes therefore verified all 39 registered tests.

| Compiler | Verified executable SHA-256 |
| --- | --- |
| MinGW | 5BE6B6FB23DE7BB772D32967D9E82F6AD6BD6EA6246CD7AB8DBA794D758718E6 |
| MSVC v143 | 40EA7081FD5DE573346B12B298A904BB3C3FB4B672616197005C35F9C8B6D291 |

The MSVC v143 build is the selected binary. Manual checks and their status
are listed above.
