# TSRE GenX v0.15 Release Notes

v0.15 is the corrective full release replacing the withdrawn v0.14 code. Its
existing annotated `v0.15` tag stores the exact reviewed source used for the
binary package; the tag remains immutable and was not moved or replaced while
the full distribution was prepared.

## Critical PolyVeg correction

- Corrected PolyVeg cleanup for valid compressed-binary MSTS world files.
  Binary `JINX0w0b` token streams are validated and searched for exact generated
  shape names in their supported encodings instead of being decoded as UTF-8.
- Made uncertain, malformed, and unsupported world input stop cleanup before
  any generated asset is staged or removed.
- Committed generated-file ownership when a route Save attempt begins. Once a
  world file may have been written, a later save failure can leave a recoverable
  orphan but cannot make Discard delete a shape that the saved world references.
- Expanded the registered manifest probe with representative compressed-binary
  retention and malformed-root fail-closed cases.

## PolyVeg water clearance and baking

- Added required schema-controlled water clearance beside TrackDB and RoadDB
  clearances. New schemas default to an 8 m setback.
- Excluded planting from water-enabled terrain whose current geometry is below
  the bilinear water surface, including setback expansion across tile borders.
- Shared the water exclusion between OSM/flood and ruler/area planting and
  reported water rejection counts separately.
- Kept the exclusion operation-local: it does not alter existing raw or baked
  vegetation and writes no new terrain or world metadata.
- Blank/froze the viewport during single-tile and loaded-LOD bakes while keeping
  progress dialogs responsive, and deferred patch conversion until after the
  destructive confirmation is accepted.

## Verification and disposition

- Version identities are aligned at v0.15/0.15.0.
- The operator confirmed the corrected editor, water setback, bake viewport,
  save, and reload behavior in the working pack.
- Final clean MinGW and MSVC Release builds passed all 15 registered tests in
  both lanes; executable hashes are recorded in `TEST-MATRIX-v0.15.md`.
- The flawed remote v0.14 tag was withdrawn. Its exact annotated tag and frozen
  evidence remain local for forensic comparison; public branch history was not
  rewritten.

## Distribution status

v0.15 is distributed as a complete 64-bit Windows binary release against the
existing immutable `v0.15` source tag. The release contains the reviewed MinGW
runtime, frozen public documents, user procedures, manifests, and a published
SHA-256 companion. The source tag was not moved or replaced, and no GitHub
Actions were used.
