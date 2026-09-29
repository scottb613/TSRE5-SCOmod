# TSRE GenX v0.17 — SNAP Wires, Route Diagnostics and Save Reliability

TSRE GenX is a 64-bit Windows route editor for Microsoft Train Simulator and
Open Rails routes. It combines the established TSRE editor with Qt 6/CMake,
terrain and map tools, Dynamic Track, PolyVeg, Activity Builder and Consist Builder.

> **Current version: v0.17.** A full Windows release with native SNAP wires, expanded route diagnostics and save reliability improvements. v0.16 remains the immutable prior-release baseline.

## Major Updates Since v0.16

### Auto Place poles and native wire runs

- F5 separates Objects and Wires, shows the selected shape, and accepts normal
  E-selection of placed scenery as the placement source. Finite Range, spacing,
  track/road targets, offsets and repeatable pole wobble support controlled runs.
- Commit connects valid numbered SNAP attachments on compatible placed supports.
  Shape-family prefix matching supports variants; eligible spans can cross nearby
  track-node boundaries. Attachment discovery is no longer limited to 16 wires.
- Adjust sag, width and maximum span, inspect blue raw wires, then Save to bake
  and retain placements. Bake All Wires, Preview Off and selected-run Delete Wires
  provide explicit control; recorded baked pairs are not duplicated.
- The separate SCOsnapPole example ZIP supplies a ready-to-place 16-attachment
  pole, textures and object-list entry. The new SNAP Pole and Wire User Guide
  covers installation, model conventions, placement, baking and troubleshooting.

### Generated work and safer cleanup

- Raw PolyVeg uses instance ownership and a blue tint; manually placed copies of
  the same vegetation remain ordinary scenery. Save bakes active raw wires and
  owned pending PolyVeg on loaded tiles.
- Bake & Save, Discard and Cancel handle pending generated work explicitly.
  Saved world references and shared assets remain protected during cleanup,
  including after a partially successful save. Cleanup failure keeps the editor
  open for retry.
- Wire and PolyVeg cleanup reject malformed or incomplete world data before
  deleting generated assets. Discard invalidates affected selection/clipboard
  references before object deletion.
- Wire registry reads and writes share an 8,192-span limit. Rejected Commit
  preserves the accepted file and in-memory span state instead of creating a
  registry that cannot be reopened.

### Route diagnostics and track-section compatibility

- F11 loads and scans world tiles with progress, cancellation and explicit
  incomplete-coverage reporting. It identifies stacked orphan track, isolated
  pieces and TrackDB joint/connectivity defects without automatic route repair.
- Orphan length and joint-gap thresholds, Jump/Select navigation, stable compact
  rows, wrapped details, remembered No Factor dismissal and Reset Status make
  diagnostic review easier. Valid terminal nodes and zero-length dynamic-track
  subsections no longer produce the same false positives.
- Rejected world files remain visible as diagnostics while valid tiles are
  scanned. Confirmed removal of a malformed-filename file retains a recovery
  copy; ordinary tile-load failures are diagnostic-only.
- Unused low-numbered route-local track sections are accepted and preserved on
  Save. Actual occupied-ID conflicts use explicit conversion, inspection or
  cancel choices; conversion refuses incomplete world loading.

### Track, scenery and moving tables

- Place Guard accepts intentional elevated scenery with Stick to All while
  retaining terrain-snapped checks. It prevents X-flipping track already in
  TrackDB/RoadDB when enabled, independently of AutoTDB placement settings.
- Pickup properties add track alignment and quarter-turn rotation, including
  upright orientation on grades and retained relative orientation while snapping.
- Animated turntable activation supports eligible Global and route-local shapes.
  Transfer activation handles supported linear models with matching track paths.
- Track context menus are cleaner, grade editing returns focus to the viewport,
  and the save-time indicator flashes after more than an hour without a save.

### Terrain, textures and editor reliability

- An explicit Snow texture set complements the existing seasonal and Night
  choices. Shape reload refreshes resolved seasonal textures across detail levels.
- Repeated terrain patches sharing one texture destination no longer collide in
  save staging; conflicting payloads still stop publication. Unloaded-terrain
  checks protect route rendering and map-overlay cleanup.
- Existing activity preview initializes its path and consist independently of
  debug logging. Additional guards cover preview refresh, path chains, traffic
  selection and timetable handling; unsupported actions no longer look usable.
- Consist Builder restores panel layouts without triggering an extra menu sound,
  retaining the normal launcher press cue.

### Verification

Final operator-run clean MinGW and MSVC Release builds each passed all **25
registered CTest tests** on 29 September 2026. See `TEST-MATRIX-v0.17.md` for
executable hashes, scope and remaining manual acceptance. Automated passes do not
claim complete Activity Builder, model-rendering or simulator coverage.

## Installation

1. Download **`tsre-scomod-v0.17.zip`** and its `.sha256` companion from this
   release. GitHub's automatic “Source code” archives are source only and are
   not the runnable editor package.
2. Verify the ZIP against the published SHA-256 companion.
3. Extract the **complete archive** to its permanent writable location. Do not
   run the application from inside the ZIP and do not copy only `TSRE5.exe`
   into an older release folder.
4. Run **`AddShortcutDesktop.cmd`** from the extracted folder.
5. Launch through the generated **TSRE GenX** shortcut. Move that shortcut
   wherever desired only after it has been created.
6. Keep the extracted application folder intact. The shortcut targets that
   folder, and the executable, Qt runtime, plugins, assets, and documents must
   remain together.

Back up a route before major terrain conforming, 4 m terrain work, water
processing, route-wide saves, or PolyVeg baking. Test high-risk workflows on a
copy before committing them to a production route.

### Optional SNAP-pole example

Download `SCOsnapPole-TSRE-example-v0.17.zip` and its `.sha256` companion separately.
Merge its SHAPES, TEXTURES and ADDONS folders into a backed-up route, then follow
`SNAP-POLE-WIRE-USER-GUIDE.md`. This model ZIP is not the application installer.
Blender is not required. Its runtime/simulator acceptance is recorded separately.

## Requirements and Limitations

- 64-bit Windows 10 version 1809 or later, or Windows 11.
- Existing Microsoft Train Simulator or Open Rails route content for editing.
- Qt 6.11.1 for source builds. The MinGW 13.1 build is the parity/release
  baseline; MSVC v143 is the independent verification lane.
- PolyVeg consumes route-local geodata derivatives produced by SCO LIDEX.
- Windows 7 and Qt 5/qmake builds are not supported.
- GenX remains an actively developed editor. Maintain route backups and verify
  simulator behavior after significant terrain, track, water, path, or
  vegetation changes.

- Wire discovery is intended for connected pole runs, not arbitrary branching or
  ambiguous parallel networks. The route registry is limited to 8,192 spans.
- Individual save files use atomic publication; the whole route is not one
  transaction. Keep complete backups before conversion and major edits.
- Activity/traffic extension preservation is not exhaustive; inspect simulator
  compatibility before replacing important activity files.

## Source and Documentation

The `v0.17` tag identifies the reviewed source accompanying this release.
Use `tsre-scomod-v0.17.zip` for the runnable editor; GitHub source archives do not
contain the complete Windows runtime. Verify each download against its matching
SHA-256 companion. Executable hashes in the test matrix are not ZIP checksums.

The public document set includes release notes, the test matrix, the SNAP Pole
and Wire guide, and the terrain, TERRTEX, seasonal painting, PolyVeg, Water Tools,
Dynamic Track, Grade Helper and Path Editor/Full Map guides. Its document manifest
covers the versioned copies. Verify each downloaded ZIP against its own checksum.

## Acknowledgments

Piotr Gadecki (GokuMK) created TSRE5 and its core route, terrain, texture,
activity, consist, file, and rendering systems. His later TSRE5vc work also
provided independent Qt 6 and compressed-texture references.

Eric Olesen (eric-from-trainsim) sustained the TSRE 8.006 line that directly
precedes GenX and provided its principal compatibility and source/API
reference.

Peter Gronbaek Andersen produced an independent Qt 6/CMake port whose build,
dependency, deployment, and DDS work supplied important reviewed references.

Wayne Campbell and Pete Willard are thanked for their MSTS/ORST exporter code, which was used as a reference.

## License

TSRE GenX is distributed under the GNU General Public License version 3 or
later. Third-party components remain under their respective licenses; see
`LICENSE.md` and `THIRD-PARTY-NOTICES.txt`.
