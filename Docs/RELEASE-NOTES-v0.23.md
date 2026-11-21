# TSRE GenX v0.23 - Route Loading and Editor Improvements

TSRE GenX is a 64-bit Windows route editor for Microsoft Train Simulator and
Open Rails routes, with Qt 6/CMake, terrain and map tools, Dynamic Track,
PolyVeg, Activity Builder and Consist Builder.

> **Version v0.23.** Both supported Windows Release builds passed verification.

## Major Updates Since v0.22

### Route loading and rendering

- **Work across CPU cores.** Background workers read and decode painted ACE and
  DDS terrain textures, prepare terrain meshes, and read, decompress and parse
  uncached legacy scenery shapes. OpenGL buffer and texture uploads stay on the
  render thread, with per-frame limits during normal movement.
- **Opening a route and jumping to a new area.** A progress bar covers the
  initial loading while ordinary 3D redraws are paused. Terrain textures for the
  opening area are prepared around the camera so turning immediately has
  coverage. Terrain farther away streams in as you move.
- **Painted terrain streaming.** Shared default ground textures can display
  immediately. Painted tiles use existing mipmaps or generated previews, then
  upgrade to full resolution near the camera for painting. Eligible opaque
  DXT1 ACE and DDS terrain textures upload compressed on supported hardware,
  with CPU decoding available when needed.
- **Instanced vegetation and scenery.** Repeated trees in Forest and eligible
  PolyVeg regions, and compatible opaque scenery such as repeated buildings,
  share draw calls. Picking and shadows keep their individual rendering paths.
- **Track overlays and diagnostics.** TrackDB, RoadDB and Tsection overlays
  reuse geometry across tile boundaries, and grade symbols reuse cached data.
  Optional profiling identifies time spent on loading, uploads and overlays,
  and ranks scenery draws by shape and material.

### Rolling stock and Consist Editor

- Legacy wagon bodies, freight animations and LoadData containers use the
  corresponding Open Rails texture lookup rules in previews and on the route.
  ACE and DDS follow seasonal lookup order with DDS preferred within each
  directory. Shared shapes can keep separate wagon repaints.
- Self-contained glTF and GLB previews can display embedded PNG, JPEG and
  supported DDS textures, including PNG transparency. Large GLB files are
  memory-mapped for preview while decoded textures retain their memory limits.

### Route editing and maintenance

- **Panel alignment.** In F12, choose Left or Right to group Tools and
  Properties on the same side with a divider. Center keeps the viewport
  between the panels and is the default. The editor saves the selection.
- **Transform.** Press Enter or keypad Enter after filling in Translate or
  Rotate. The transform remains undoable.
- **Remove DYNATRAX.** Scans the route from a selected track piece and asks for
  confirmation before replacing detected placements with native Dynamic Track.
  The conversion removes obsolete generated shapes and descriptors while
  protecting assets still in use. Placements that cannot be reconstructed
  safely block conversion.
- **Process All TERRTEX.** Cancel lets the active texture finish. Resuming
  checks completed output and skips verified textures, including textures
  that have already been compressed.
- **Path and database safety.** Track-path input is checked before indexing
  track records. Route saves and unsaved-change prompts detect TrackDB and
  RoadDB changes even when no world object was edited.

### Builds and testing

- The routine compile builds the application and its dependencies. Use
  -WithProbes to explicitly compile the diagnostic probes. The release
  verification builds and tests the full registered suite in MinGW and MSVC.

- Clean MinGW and MSVC builds verified all 39 registered tests per lane,
  including 68 exact matches in the optional ACE corpus on a separate run.

## Installation

The binary asset is **`tsre-scomod-v0.23.zip`** with its `.sha256` companion.
The binary package uses the MSVC v143 build and its matching runtime.

1. Download the binary ZIP and matching checksum from the v0.23 release.
   GitHub's automatic source archives are not the runnable Windows package.
2. Verify the ZIP against its SHA-256 companion.
3. Extract the **complete archive** to a permanent writable location.
4. Run **`AddShortcutDesktop.cmd`** from the extracted folder.
5. Launch through the generated **TSRE GenX** shortcut. Move the shortcut only
   after creating it, and keep the extracted application folder intact.
6. If the Microsoft Visual C++ runtime is unavailable, run the included
   `vc_redist.x64.exe` installer before starting the editor.
7. Keep the executable, matched runtime libraries, plugins, assets and documents
   together. Do not mix compiler runtimes or copy only a new executable into an old pack.

Back up routes before terrain conforming, water processing, route-wide saves,
Forest replacement, DYNATRAX conversion or PolyVeg baking. Test substantial work on a copy first.
Production DXT1 conversion deliberately retains no TERRTEX backup: use a separate
final upload copy and retain your editable route separately.

## Requirements and Limitations

- 64-bit Windows 10 version 1809 or later, or Windows 11. Windows 7 and Qt5/qmake
  builds are unsupported.
- Existing MSTS/Open Rails route content; SCO LIDEX derivatives for mapped PolyVeg.
- Source builds use Qt 6.11.1 with MinGW 13.1 or MSVC v143.
- Wire discovery is bounded to 512 supports and 256 tiles per run, with an
  8,192-span registry. It cannot infer the intended path among nearby parallel lines.
- glTF preview is a bounded base-colour/rest-pose preview, not full simulator
  PBR, animation or intersecting-transparency parity.
- Pack for Distro permanently deletes selected files without backups/Undo;
  its title does not mean ZIP creation. Copy To adds catalog resources, not world
  placements. External editors do not honor the internal operation locks. Abrupt
  process failure can leave unreferenced resources or safe partial terrain updates.

## Source and Documentation

The **v0.23** tag identifies the source for the matching release package. `tsre-scomod-wip` remains the maintained public source branch.
`TEST-MATRIX-v0.23.md` records build hashes, tests and manual coverage. Verify the
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
