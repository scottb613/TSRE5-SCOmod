# TSRE GenX v0.20 — Route Distribution and Object Copy

TSRE GenX is a 64-bit Windows route editor for Microsoft Train Simulator and
Open Rails routes, with Qt 6/CMake, terrain and map tools, Dynamic Track,
PolyVeg, Activity Builder and Consist Builder.

> **Current version: v0.20.** Both operator-run clean Release builds passed all 34 registered tests on 27 October 2026. This full Windows release follows the immutable v0.19 baseline.

## Major Updates Since v0.19

### Pack for Distro

- **Route > Pack for Distro...** scans the full saved route and shows exact
  removal candidates, categories and sizes before deleting selected items.
  All three scan options default on; deletion candidates start unchecked.
- Protects placed shapes, SD files, textures, seasonal/night variants, ACE/DDS
  fallbacks, glTF resources and retained editor catalogs. Missing-content warnings
  allow unrelated cleanup; uncertain dependency data protects affected assets.
- Optional `_e.raw` and `_n.raw` buffer removal preserves elevation `_y.raw`,
  flags `_f.raw`, other RAW data and RAW backups. Separate categories identify
  unused models/textures, backups, artwork, temporary files and editing caches.
- The default-on editing-data option includes `osm_data` and saved `terrain_maps`
  tiles/overlays. **Editing maps / overlays** has its own selectable category.
  Runtime/catalog references, uncertain dependencies and RAW data remain protected.
- Optional consolidation compares complete byte-identical TERRTEX families,
  including every seasonal/format variant. Verified terrain-reference updates
  precede removal; images are not recompressed. Unsupported inputs retain assets.
- Category selection, numeric size sorting, grouped warnings and UTF-8 reports
  make the review easier. Reports retain actual partial outcomes and measured
  whole-route before/after sizes, including descriptor-size changes.
- Total cleanup progress includes revalidation, file checks, reference updates,
  deletion, measurement and loaded-terrain refresh. **Delete Selected is permanent:
  no ZIP, backup or Undo is created.** Use the intended saved production copy.

### Static Object Copy To

- **Route > Copy To...** selects a validated receiving route or an ordinary
  **Working Folder**. Both folder choices and the mode persist between sessions;
  revalidation is silent unless there is an error.
- Selected Static Objects offer **Copy To...** in their own right-click section,
  outside Edit. Route copies ask only **Name** and **Category**. Name starts with
  the selected shape filename without path/extension and remains editable.
- Copies original models, SD companions, referenced seasonal/night textures,
  ACE/DDS counterparts and external glTF/GLB images/buffers. REF alternatives,
  alignment/random placement settings, static flags, detail and collision settings
  are retained automatically for route catalog entries.
- Identical destination resources are reused; conflicting or missing resources
  stop copying. Staging and verification precede the final atomic REF update.
  Existing REF bytes/encoding are preserved; new catalogs use UTF-16LE.
- Working mode directly exports the selected model/resources into a new object
  folder for editing, with no REF form or REF update. Repeated exports receive
  numbered folders. Relative SHAPES/TEXTURES layouts remain intact.
- Copy To has no progress bars; completion/errors report the result. Failures
  roll back newly copied resources while retaining preexisting or externally
  changed files. Placement coordinates, orientation and scale are not cloned.

### Reliability and Interface

- Explicit Unicode validation rejects truncated text and isolated surrogate
  characters without rejecting valid supplementary characters. Descriptor/REF
  fields ignore lookalike text in comments or quoted descriptions.
- Bounded dependency reads and shared destination locks protect copying and
  cleanup. Report extensions are resolved before overwrite confirmation.
- Removes the redundant standalone Select Tool from right-click menus; **E**,
  control-panel selection and the main Edit command remain available.
- Both operator-run clean Release builds passed **34/34 registered tests**.
  The operator authorized the official cap after updating the desktop test copy.

## Installation

The binary asset is **`tsre-scomod-v0.20.zip`** with its `.sha256` companion.
This full release contains the reviewed, compiler-matched MinGW runtime.

1. Download the binary ZIP and matching checksum from the v0.20 release.
   GitHub's automatic source archives are not the runnable Windows package.
2. Verify the ZIP against its SHA-256 companion.
3. Extract the **complete archive** to a permanent writable location.
4. Run **`AddShortcutDesktop.cmd`** from the extracted folder.
5. Launch through the generated **TSRE GenX** shortcut. Move the shortcut only
   after creating it, and keep the extracted application folder intact.
6. Keep the executable, matched runtime libraries, plugins, assets and documents
   together. Do not mix compiler runtimes or copy only a new executable into an old pack.

Back up routes before terrain conforming, water processing, route-wide saves,
Forest replacement or PolyVeg baking. Test substantial work on a copy first.
Production DXT1 conversion deliberately retains no TERRTEX backup: use a separate
final upload copy and retain your editable route separately.

## Requirements and Limitations

- 64-bit Windows 10 version 1809 or later, or Windows 11. Windows 7 and Qt5/qmake
  builds are unsupported.
- Existing MSTS/Open Rails route content; SCO LIDEX derivatives for mapped PolyVeg.
- Source builds use Qt 6.11.1; MinGW 13.1 is the release baseline and MSVC v143
  is the independent verification lane.
- Wire discovery is bounded to 512 supports and 256 tiles per run, with an
  8,192-span registry. It cannot infer the intended path among nearby parallel lines.
- glTF preview is a bounded base-colour/rest-pose preview, not full simulator
  PBR, animation or intersecting-transparency parity.
- Pack for Distro permanently deletes selected files without backups/Undo;
  its title does not mean ZIP creation. Copy To adds catalog resources, not world
  placements. External editors do not honor the internal operation locks. Abrupt
  process failure can leave unreferenced resources or safe partial terrain updates.
- Both clean Release compiler suites passed 34/34 tests. The operator authorized
  the official cap after desktop testing. See the current test matrix for hashes and coverage.

## Source and Documentation

The immutable **v0.20** tag identifies the exact reviewed release source. `tsre-scomod-wip` remains the maintained public source branch.
`TEST-MATRIX-v0.20.md` records build hashes, tests and manual coverage. Verify the
binary ZIP with its matching SHA-256 companion before extraction.

The package includes Pack for Distro and Static Object Copy To guides, plus
terrain, TERRTEX, seasonal painting, PolyVeg, Water Tools,
Dynamic Track, Grade Helper, Path Editor/Full Map and SNAP Pole/Wire guides.
`scoWorkList.txt` contains the same grouped feature summary without release history.
The optional SCOsnapPole model example remains a separate route-content download.
## Acknowledgments

Piotr Gadecki (GokuMK) created TSRE5 and its core route, terrain, texture,
activity, consist, file and rendering systems. His later TSRE5vc work provided
independent Qt 6 and compressed-texture references.

Eric Olesen (eric-from-trainsim) sustained the TSRE 8.006 line that directly
precedes GenX and provided its principal compatibility and source/API reference.

Peter Gronbaek Andersen produced an independent Qt 6/CMake port whose build,
dependency, deployment and DDS work supplied important reviewed references.

Wayne Campbell and Pete Willard are thanked for their MSTS/ORST exporter code,
which was used as a reference.

## License

TSRE GenX is distributed under the GNU General Public License version 3 or later.
Third-party components retain their own licenses; see `LICENSE.md` and
`THIRD-PARTY-NOTICES.txt`.
