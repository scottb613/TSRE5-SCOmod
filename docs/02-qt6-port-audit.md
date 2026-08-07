# Qt6 port audit

Run:

```powershell
.\scripts\Audit-Qt6.ps1
```

The report is written to `reports/qt6-audit.txt`.

## Imported v0.8 result

Initial audit run: 2026-07-30

- 10 `QRegExp`/`QRegExpValidator` references
- 5 `QDesktopWidget`/`QApplication::desktop()` references, including comments
- 36 `QTextStream::setCodec()` references
- No legacy event-position patterns detected by the current audit
- No `QGLWidget`/`QGLFormat`/`QGLContext` references detected
- No removed-container or Qt5 compatibility-module references detected

The encoding conversions are the highest-risk mechanical group because they
write MSTS/Open Rails UTF-16 files. They must be handled as a dedicated change
with serialization comparisons, not mixed into the general compile cleanup.

## Step 1 result

Completed: 2026-07-30

- Replaced all `QRegExp` and `QRegExpValidator` use with
  `QRegularExpression` and `QRegularExpressionValidator`.
- Preserved anchored matching, case-insensitive backup cleanup, numeric-value
  detection, route-key sanitizing, and new-route input validation.
- Replaced the remaining `QDesktopWidget` dependency with `QScreen` and
  `QGuiApplication::primaryScreen()`.
- Removed one obsolete commented desktop-size block.
- Corrected `WindowManager.cpp` so it includes its own header and no longer
  defines an undeclared constructor.
- Re-ran the audit: QRegExp and QDesktopWidget groups both report `(none)`.
- Focused Qt6 compiles passed for `Eng.cpp`, `Game.cpp`, `LoadWindow.cpp`,
  `PropertiesSignal.cpp`, `SettingsDialog.cpp`, `WindowManager.cpp`, and
  `main.cpp`.

No UTF-16 stream conversion was included in this step.

## Step 2 result

Completed: 2026-07-30

- Replaced all 36 `QTextStream::setCodec()` calls with explicit
  `QStringConverter::Utf16` or `QStringConverter::Utf8` encodings.
- Replaced all removed `QDataStream::unsetDevice()` calls with
  `setDevice(nullptr)`.
- Added a CTest-compatible text-stream probe which can be compiled against
  both Qt5 and Qt6 for migration comparison.
- Verified the old Qt5 and new Qt6 paths produce byte-identical 122-byte
  UTF-16 output, including the little-endian BOM and non-ASCII text.
- Recorded matching SHA-256:
  `C4EE395A2404829448DD9CC008A2CDD17F525445D20530C94273C263216B9623`.
- Replaced the removed `QStringRef` and `QString::midRef()` APIs found by the
  focused build with behavior-equivalent `QString::mid()` calls.
- Qualified the server's `QTextStream` line-ending manipulator as `Qt::endl`.
- Corrected two adjacent pre-existing Activity horn-pattern type/assignment
  defects exposed by the Qt6 compiler.
- All 17 affected application translation units compile under Qt6, Qt
  AUTOMOC passes, and the post-port encoding probe remains byte-identical.

No Qt5 dependency was added to v0.9. The Qt5 compile was a one-time reference
measurement only; the application and permanent build target remain Qt6-only.

## Step 3 result

Completed: 2026-07-30

- Used Eric's dedicated Qt6 migration commit `7d84e54` as the primary
  mechanical checklist, with Peter and Goku used as independent references.
- Adapted only narrow API-level changes to GenX. No reference file or source
  tree was copied over a GenX implementation.
- Converted the remaining mouse and wheel event handling to
  `position()`/`angleDelta()`.
- Converted the remaining XML string-view literals, font metrics, layout
  margins, line-edit writes, shape-character conversion, and tree-item color
  APIs found by the complete compiler pass and reference review.
- Marked the impossible `AceLib` copy operation deleted. This agrees with
  Eric's explicit deletion and Goku's removal; GenX has no copy call sites.
- Added the Peter-confirmed Windows `winmm` link required by the existing GenX
  `PlaySoundW` implementation.
- Completed the first full Qt6 compile and link with zero error lines.
- Produced `TSRE5.exe` with SHA-256
  `AB8A67DCE28B893A25358E4F7EACD16C9C988DB3FF522CC18064B89702ABFDB3`.
- Deployed the exact Debug executable, GenX runtime assets, Qt6 plugins,
  MinGW runtimes, and vcpkg DLLs to `TSREvcTST`.
- Recursively inspected 33 deployed PE binaries and 243 import edges. No
  deployable DLL is unresolved; three `api-ms-win-*` names are normal Windows
  virtual API-set contracts.

## Initial runtime stabilization result

Completed: 2026-07-30

- Launched the controlled Qt6 package and confirmed the Route Loader and Route
  Editor open the reference route with the GenX panels and scene intact.
- Launched Consist Editor and confirmed its initial presentation remains
  consistent with the v0.8 baseline.
- Replaced Qt5 string-based signal names removed or renamed by Qt6:
  `QComboBox::activated(QString)` is now `textActivated(QString)`, and
  `QSignalMapper::mapped(int)` is now `mappedInt(int)`.
- Disconnected one inaccessible experimental TrackDB rebuild action whose
  receiver slot no longer exists and whose action is not exposed in the menu.
- Re-scanned the source and found no remaining instances of those dead
  connections.
- Added one application-wide Windows 10/11 native-caption treatment. Every
  top-level window shown by Qt now receives the GenX charcoal caption, warm
  title text, restrained border, and native dark caption buttons. Standard
  Windows window behavior remains intact.
- Corrected the caption treatment after runtime testing exposed detached
  combo-box popups. The DWM path now operates only on an existing top-level
  `QWindow`; it never calls `QWidget::winId()` to manufacture native handles
  while embedded panels are still being assembled. Transient popups,
  tooltips, splash surfaces, frameless widgets, and untitled embedded panels
  are excluded. Object Selection dropdown placement was visually verified.
- Rebuilt and deployed the tested executable. The build and `TSREvcTST`
  copies have matching SHA-256:
  `C9C87A59F7BADA1E0F931A12E385BC87C64B7306E5F84BF262C884196EBBAF0B`.
- Visual testing reported no issues in the Load, Route Editor, and
  Consist Editor windows.

Shape Viewer was subsequently launched from the controlled runtime. Route
objects and other rolling stock displayed correctly, so its startup and basic
rendering test passes. One custom RS3 skin remained blank; its compressed shape
expanded to the declared byte count, its referenced number texture is a valid
512x512 DXT5 DDS, and the shape-loading source is unchanged from frozen v0.8.
That model-specific blank-texture case was subsequently identified as the Qt6
DDS loading gap and resolved by the restored DDS decoder. Gate 4 interaction
testing was completed and accepted by the operator for v0.9 on 2026-07-31.

Camera Terrain Lock runtime testing exposed an inherited movement-path gap:
forward, backward, and lateral movement invoked the terrain clamp, but direct
vertical descent did not. `CameraFree::moveDown()` now invokes the existing
coordinate and terrain check. LOCK mode stops at the established two-meter
terrain clearance, while FREE mode continues to permit underground movement.
Both modes were visually verified.

## Expected migration areas

| Legacy API or behavior | Qt6 direction | Risk |
|---|---|---|
| `QRegExp` | `QRegularExpression` | Pattern semantics and exact matching |
| `QRegExpValidator` | `QRegularExpressionValidator` | User-input acceptance |
| `QDesktopWidget` | `QScreen` / `QGuiApplication::screens()` | Saved window placement |
| `QTextStream::setCodec()` | `QStringConverter` encoding | MSTS UTF-16 file compatibility |
| Old mouse/wheel positions | `position()` / `globalPosition()` | Editor picking and camera input |
| Removed containers/APIs | Qt6-supported equivalents | Ordering and iterator behavior |
| Qt5 OpenGL widget assumptions | Qt6 OpenGL/OpenGLWidgets | Context lifetime and rendering |

## High-risk GenX-specific areas

- Terrain paint previews, writes, and generated textures
- Main/Snow seasonal TERRTEX mirroring
- ACE/DDS fallback and cache invalidation
- Dynamic Track mesh versus TrackDB geometry
- Atomic route/database saves and recovery
- F4 PAT output and extended shunting instructions
- Per-user settings JSON encoding and recovery
