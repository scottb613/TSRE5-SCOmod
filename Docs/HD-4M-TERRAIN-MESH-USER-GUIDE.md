# HD 4 m Terrain Mesh User Guide

This guide walks through a complete route conversion to the experimental 4 m
terrain mesh, from SCO LIDEX preparation through editing in TSRE GenX and the
first validation run in Open Rails.

> **Open Rails compatibility — read before converting**
>
> As of August 28, 2026, **Open Rails UNSTABLE is the only Open Rails channel
> that supports the experimental 4 m terrain mesh.** Stable and Testing do not
> support it. Keep a separate working Open Rails installation for ordinary
> routes and use an Unstable installation only for the 4 m route. The Open
> Rails project warns that Unstable builds may contain serious defects.

## What 4 m Terrain Changes

An MSTS/Open Rails terrain tile still covers approximately 2048 m by 2048 m.
The difference is the number and spacing of the height samples inside it:

| Terrain type | Height grid | Post spacing | Samples per 16x16 texture patch |
| --- | ---: | ---: | ---: |
| Standard terrain | 256x256 | 8 m | 16x16 |
| HD test terrain | 512x512 | 4 m | 32x32 |

The denser grid gives terrain tools more height posts to work with. It can
improve cuttings, embankments, drainage, shorelines, and other close terrain
work, but it does not turn coarse elevation source data into true 4 m survey
data. Source quality still determines how much real detail is available.

The texture-patch layout remains 16x16 patches per terrain tile. A 4096x4096
map image can improve the editing reference, but **HD Map Tiles and HD Mesh
Tiles are independent options** in LIDEX.

## Non-Negotiable Safety Rules

- Back up the **entire route folder** before conversion. A copied route folder
  on another drive is the safest starting point.
- Never mix 256/8 m and 512/4 m terrain tiles in one route. LIDEX checks the
  whole route and can rebuild all mismatched terrain tiles during Run.
- Close TSRE GenX, Open Rails, and any other program using the route before
  LIDEX writes it.
- Leave **Clean Tile Wipe** off for a normal conversion. It intentionally
  rebuilds a terrain descriptor from a clean template and can remove texture,
  water, overlay, and other tile edits.
- Do not open or save the converted route in an editor that does not understand
  512-sample terrain.
- Treat 4 m support as experimental. Test a copy before making the converted
  route your working master.

## What You Need

- A complete, working backup of the route.
- SCO LIDEX v1.401 or later.
- TSRE GenX v0.15 or later for editing native 4 m terrain.
- A separate current Open Rails **Unstable** installation for driving the
  route.
- Enough free disk space for the backup, converted terrain, source downloads,
  and optional 4096 map tiles.
- A working internet connection while LIDEX obtains elevation and map data.

## Recommended End-to-End Plan

Use this order for an existing route:

1. Prove the original route still works.
2. Back up the entire route folder.
3. Prepare a separate Open Rails Unstable installation.
4. Convert all existing route terrain to 4 m in one LIDEX operation.
5. Resolve any LIDEX failures before opening the route in an editor.
6. Open, inspect, make a small test edit, save, and reopen in TSRE GenX.
7. Drive a short test in Open Rails Unstable with logging enabled.
8. Only then begin normal 4 m route work.

## Part 1 — Establish a Safe Baseline

Before changing anything:

1. Open the unconverted route in its currently supported editor.
2. Visit several representative areas:
   - a tile boundary;
   - a cutting or embankment;
   - a water crossing;
   - a textured area;
   - an area with map overlays, if used.
3. Save only if the route is already known to be healthy.
4. Run a short Open Rails test and preserve the log if there are existing
   warnings.
5. Close the editor and simulator.
6. Copy the **entire route folder**, including `Tiles`, `World`, `TERRTEX`,
   databases, activities, paths, and route-local support folders, to a backup
   location.
7. Give the backup an unmistakable name such as
   `RouteName_before_4m_conversion`.

Do not rely on a copy of only the `Tiles` folder. Terrain, world placement,
textures, water, track databases, and route settings must remain a matched set
for a dependable rollback.

## Part 2 — Install or Switch Open Rails to UNSTABLE

The safest arrangement is to preserve your normal Stable or Testing folder and
put Unstable in a different folder. Open Rails states that separate versions do
not interfere when kept in separate folders.

### Using the Open Rails updater

1. Start the Open Rails installation you intend to dedicate to 4 m testing.
2. Open **Menu > Options > System**.
3. Set **Update mode** to **Unstable**.
4. Select **OK** and allow Open Rails to check for an update.
5. Open the red **Notifications** flag when an update is offered.
6. Review the notification and select **Install**.
7. Restart Open Rails after the update completes.
8. Confirm that the title/version information identifies the running build as
   Unstable before loading the 4 m route.

If the updater does not offer the required build, use the official
[Open Rails Versions page](https://www.openrails.org/download/versions/) and
its Unstable archive. Extract that version into its own folder rather than
overwriting a known-good Stable or Testing installation.

After starting Unstable:

1. Open **Options > Content** and add or verify the normal MSTS/Open Rails
   content path that contains the route.
2. Enable logging for the first route tests.
3. Note the exact Unstable build number. Include it with any problem report.

Open Rails normally writes `OpenRailsLog.txt` to the desktop. Preserve that
file after a failed 4 m test before launching another session.

## Part 3 — Configure SCO LIDEX for a Complete Route Pass

This procedure converts the full existing terrain coverage in one operation.

1. Close TSRE GenX and Open Rails.
2. Start `SCOLIDEX.exe`.
3. At **Route Path**, select **Browse** and choose the route folder containing
   the route `.trk` file.
4. Select **Overwrite** mode.
5. Select **Use Route Tiles**.
6. Select **Create Route Tiles**.
7. Select **Enable HD Mesh Tiles**.
8. Select **HD Test - 4m Tiles**.
9. Leave **Clean Tile Wipe** clear.
10. Leave the north/south and east/west **Geo Bias** values at `0` unless the
    route has already been measured and calibrated for a specific correction.
11. Choose the optional products you want:
    - **Create DM Tiles** builds distant-mountain terrain. It is independent of
      the normal 4 m terrain grid.
    - **Create OSM/Map Tiles** creates route-local map and geodata derivatives.
    - **Enable HD Map Tiles** creates 4096x4096 map PNGs instead of the normal
      2048x2048 images. It does not change terrain height resolution.

`Use Route Tiles` is the recommended selection for a complete conversion
because it starts with the route's existing terrain coverage. The other
selection methods are useful when creating or extending coverage:

| Selection | Source | Tile Radius applies? |
| --- | --- | --- |
| Use Route Tiles | Existing route terrain tiles | No |
| Use Text File | Exact tile names in `<Route>\SCOLIDEXTiles.txt` | No |
| Use Marker File | `<Route>\<RouteName>.mkr` | Yes |
| Use KML File | `<Route>\<RouteName>.kml` | Yes |
| Use Track Database | `<Route>\<RouteName>.tdb` | Yes |

For radius-based selections, radius `0` selects only the source tile, `1`
selects a 3x3 area, and `2` selects a 5x5 area around it. These methods do not
override the no-mixed-grid rule.

### Why Overwrite is recommended

Append normally fills tiles that lack usable elevation. Overwrite expresses
the intended conversion clearly: rebuild the selected terrain from the current
elevation sources while retaining existing texture, water, and overlay data
where possible. The route backup remains essential.

When LIDEX detects tiles at the wrong grid resolution, approving the conversion
causes Run to rebuild **every mismatched route terrain tile** to the selected
resolution, including mismatched tiles outside a narrower original selection.
This is deliberate protection against an invalid mixed 8 m/4 m route.

## Part 4 — Scan Before Writing

1. Recheck that the route path points to the working copy, not the backup.
2. Recheck these critical settings:
   - **Overwrite**;
   - **Use Route Tiles**;
   - **Create Route Tiles** selected;
   - **Enable HD Mesh Tiles** selected;
   - **HD Test - 4m Tiles** selected;
   - **Clean Tile Wipe** not selected.
3. Select **Scan**.
4. Read the full result before approving anything.

Scan is read-only. With **Create Route Tiles** selected, it inspects the
resolution of every terrain tile already in the route—not only the selected
coverage. An existing 8 m route should produce a resolution-mismatch warning.

Approve the warning only if:

- the entire route has been backed up;
- 4 m is the selected target;
- you intend a route-wide conversion;
- no editor or simulator has the route open.

If the mismatch count or route location is unexpected, cancel and correct the
selection. Do not use Clean Tile Wipe as a shortcut around a scan warning.

## Part 5 — Run the Full 4 m Conversion

1. Select **Run**.
2. Read the final confirmation and verify the route and 4 m target again.
3. Confirm the operation.
4. Leave LIDEX running until it reports completion. Do not open or edit the
   route during the run.
5. Review the on-screen result and `SCOLIDEX.log`.

LIDEX processes terrain with rolling seam windows. Completed rows are
edge/corner merged, written, and released instead of retaining the whole route
mesh in memory. The conversion may still take substantial time on a large
route, especially with optional maps and distant mountains enabled.

For normal terrain, LIDEX prefers the best available source in this order:
USGS approximately 1 m, 5 m, then 10 m products, followed by Copernicus GLO-30
for unresolved posts. Distant mountains use Copernicus. A temporary source
failure is reported rather than silently treated as lower-quality data.

### If any terrain tile fails

Do not move on to GenX or Open Rails with an incomplete terrain pass.

1. Preserve `SCOLIDEX.log`.
2. Read the failed-tile list. LIDEX prints a list suitable for pasting into
   `SCOLIDEXTiles.txt`.
3. Wait for the elevation service to recover if the error is network-related.
4. Put the failed tile names in `<Route>\SCOLIDEXTiles.txt` if a focused retry
   is appropriate.
5. Select **Append**.
6. Keep **Create Route Tiles**, **Enable HD Mesh Tiles**, and
   **HD Test - 4m Tiles** selected.
7. Select **Use Text File** for the exact failed list, or repeat the original
   route selection.
8. Run the retry.
9. Repeat until no required terrain tile is reported failed.

LIDEX stops dependent later stages when normal terrain fails. Finish the
terrain retry before relying on DM or map output.

## Part 6 — Validate the LIDEX Result

Before editing:

1. Run **Scan** again with **Create Route Tiles** and **HD Test - 4m Tiles**
   selected.
2. Confirm that LIDEX reports no 8 m/4 m mismatch in the route.
3. Review `SCOLIDEX.log` for failed or skipped terrain tiles.
4. Confirm that route terrain files have current timestamps across the expected
   coverage.
5. If map tiles were requested, check the route's `terrain_maps` folder. A
   4096 image confirms HD map output, not 4 m mesh output.

As a secondary diagnostic, an uncompressed 512x512 16-bit terrain height file
contains 524,288 bytes of height samples, while a 256x256 file contains 131,072
bytes. Use the LIDEX resolution scan as the authoritative route-wide check;
file size alone does not prove that the complete route is consistent.

## Part 7 — First Open in TSRE GenX

TSRE GenX v0.15 edits both native 256/8 m and experimental 512/4 m terrain.
The first session should be an inspection and controlled save test.

1. Start TSRE GenX v0.15 or later.
2. Open the converted route copy.
3. Allow all terrain and texture loading to finish.
4. Visit the same representative locations used for the baseline check.
5. Look for:
   - missing or flat tiles;
   - visible height seams at tile boundaries;
   - shifted terrain textures;
   - missing water or overlays;
   - track or road unexpectedly buried or floating.
6. Press **F2** to open **Terrain Mesh**.
7. In an expendable test area, use a small, low-intensity height brush once.
8. Press **Ctrl+Z** and confirm that the 4 m edit is undone.
9. If track conforming is part of the workflow, select a short track section
   and use the selected-object conform command on that small section first.
10. If the route has water, open **F7 Water Tools** and inspect a short,
    uncomplicated section before attempting a full river operation.
11. Save with **Shift+Ctrl+S**.
12. Close GenX, reopen the route, and recheck the edited tile and its neighbors.

Useful terrain keys and panels:

| Key | Function |
| --- | --- |
| F2 | Terrain Mesh tools |
| F3 | Terrain Texture tools |
| F7 | Water Tools |
| Ctrl+Z | Undo the last supported edit |
| Shift+Ctrl+S | Save the route |
| Z | With a terrain brush active, toggle its direction between `+` and `-` |
| F | Set/conform terrain to the selected track, object, or ruler |
| Ctrl+F | Conform along the selected track/road database vector in the current tile |
| Shift+F | Smooth terrain around the selected track, object, or ruler |
| M | Toggle the map overlay |

The F2 **Size** control represents a smaller real-world embankment half-width
on 4 m terrain than on standard 8 m terrain:

| F2 Size | 4 m terrain half-width | 8 m terrain half-width |
| ---: | ---: | ---: |
| 1 | 4 m | 8 m |
| 2 | 6 m | 12 m |
| 3 | 8 m | 16 m |
| 4 | 10 m | 20 m |
| 5 | 12 m | 24 m |
| 6 | 14 m | 28 m |
| 7 | 16 m | 32 m |

Height-brush coverage remains based on the legacy world-radius behavior, but a
4 m tile contains more affected height posts. Begin with conservative Size and
Intensity settings.

For detailed brush, conform, waterbed, Auto Paint, Undo, cleanup, and save
instructions, see the
[Terrain Improvements User Guide](TERRAIN-IMPROVEMENTS-USER-GUIDE.md).

## Part 8 — First Run in Open Rails UNSTABLE

Do this only after the route saves and reopens cleanly in GenX.

1. Start the separate Open Rails **Unstable** installation.
2. Confirm the displayed build/channel before selecting the route.
3. Confirm logging is enabled.
4. Choose the converted route and a short, simple Explore Route or activity
   run.
5. Begin in an area with known good baseline behavior.
6. Drive across at least one terrain-tile boundary.
7. Inspect:
   - near and distant terrain loading;
   - tile seams and sudden elevation steps;
   - track clearances in cuts and on fills;
   - bridge approaches and water crossings;
   - shoreline and waterbed behavior;
   - frame rate and memory behavior in dense terrain.
8. End the session normally.
9. Preserve `OpenRailsLog.txt` and record the exact Unstable build number.
10. Repeat with a representative activity only after the short test succeeds.

Do not treat success in Stable or Testing as a valid 4 m test. At the date of
this guide, those channels do not support this mesh format.

## Routine 4 m Editing Workflow

After conversion and validation:

1. Back up before each major terrain operation.
2. Open only in a 512-aware editor such as TSRE GenX v0.15 or later.
3. Make and inspect terrain changes in manageable sections.
4. Use **Ctrl+Z** immediately when a brush or conform pass is wrong.
5. Use F7 Water Tools only after confirming the route remains entirely 4 m.
6. Save, close, and reopen after major terrain, water, or texture work.
7. Test frequently in the current Open Rails Unstable build.
8. Keep the last known-good Unstable package and route backup. A newer
   Unstable build can introduce unrelated regressions.

## Geo Bias Guidance

Leave both LIDEX Geo Bias sliders at `0` unless there is a measured reason to
change them.

- Standard route projection already includes LIDEX's normal projection
  baseline; slider zero means no additional correction.
- Routes using `TsreGeoProjection` use their route-centered projection instead.
- The best final correction is a new Run from the elevation source at the
  chosen bias.
- **Commit/Post Processing** resamples the existing height grid at its native
  4 m or 8 m spacing. It is useful for testing, but it can soften or distort
  terrain, and repeated shifts compound the damage.

Test a bias on a route copy, measure the result, then rerun from source once at
the chosen values.

## Converting Back to 8 m

Restoring the complete pre-conversion route backup is the safest and most
faithful rollback.

If a deliberate route-wide rebuild to 8 m is required instead:

1. Back up the current 4 m route.
2. In LIDEX, select the route and **Use Route Tiles**.
3. Select **Overwrite** and **Create Route Tiles**.
4. Clear **Enable HD Mesh Tiles** and select **Normal - 8m Tiles**.
5. Leave **Clean Tile Wipe** clear.
6. Scan and approve conversion of every mismatched 4 m tile.
7. Run the complete rebuild and verify that no mixed resolution remains.

This is a resample/rebuild, not a lossless undo. Restore the original backup
when preservation of the old terrain is important.

## Troubleshooting

| Symptom | Likely cause | Action |
| --- | --- | --- |
| HD Test - 4m Tiles is unavailable | HD mesh controls are locked | Select **Enable HD Mesh Tiles** first. |
| Scan reports many mismatched tiles | Expected 8 m route-wide conversion, or an unintended route | Verify the route path and backup; approve only when all route terrain should become 4 m. |
| LIDEX reports failed terrain tiles | Elevation service, network, or source failure | Preserve the log and retry the reported tiles with **Append** after the service recovers. |
| Route contains both 256 and 512 grids | Conversion/retry did not complete | Do not edit or run it; use LIDEX Scan and rebuild every mismatch to one resolution. |
| Terrain textures, water, or overlays disappeared | Clean Tile Wipe may have been used, or conversion failed | Stop editing and restore the full backup; repeat with Clean Tile Wipe off. |
| Map overlay is missing after generation | Editor cached the previous map state | Close and reopen the route; verify `terrain_maps` output. |
| 4096 maps exist but terrain is still 8 m | HD Map and HD Mesh are separate settings | Enable HD Mesh Tiles, select HD Test - 4m Tiles, Scan, and Run. |
| Terrain is detailed but elevation looks blocky | Source DEM is coarser than the output grid | This is expected; 4 m sampling cannot create source detail that was not present. |
| GenX shows a seam or flat tile | Failed source tile, inconsistent resolution, or damaged terrain | Stop, inspect the LIDEX log and route-wide scan, and restore/rebuild before further saves. |
| F7 rejects a river corridor | Mixed terrain grids are present | Complete a route-wide conversion to one grid size before processing water. |
| Open Rails crashes or rejects the route | Wrong ORTS channel/build, mixed terrain, or an Unstable regression | Confirm Unstable, preserve `OpenRailsLog.txt`, scan for mixed grids, then test the last known-good Unstable build. |
| Stable or Testing will not run the route | Those channels do not currently support 4 m terrain | Use a separate current Open Rails Unstable installation. |
| Conversion result is unacceptable | Source, bias, or terrain preservation did not meet expectations | Restore the complete pre-conversion route backup and reassess before retrying. |

## Final Readiness Checklist

- [ ] The original route was tested before conversion.
- [ ] A complete route-folder backup exists and is not being edited.
- [ ] Open Rails Unstable is installed separately and its build number is known.
- [ ] LIDEX is pointed at the intended working route.
- [ ] Overwrite, Use Route Tiles, and Create Route Tiles are selected.
- [ ] Enable HD Mesh Tiles and HD Test - 4m Tiles are selected.
- [ ] Clean Tile Wipe is off.
- [ ] The Scan warning was reviewed as a route-wide conversion.
- [ ] Run completed without unresolved normal-terrain failures.
- [ ] A second Scan reports no mixed 8 m/4 m terrain.
- [ ] The route opens, saves, closes, and reopens in TSRE GenX.
- [ ] Terrain seams, texture placement, water, and overlays were inspected.
- [ ] A short route test succeeds in Open Rails Unstable.
- [ ] `SCOLIDEX.log`, `OpenRailsLog.txt`, and the exact Unstable build number are
  available if a problem must be reported.

Once every item is complete, the route is ready for normal experimental 4 m
terrain editing and Open Rails Unstable testing.
