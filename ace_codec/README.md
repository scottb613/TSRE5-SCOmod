# ACE codec: production CPU reads

Piotr Gadecki / GokuMK's standalone `AceDocument` and `DxtCodec`, from TSRE5vc
commit `30937472cde4e99517528758f8ff913a719655f6`. Source paths and Git blob IDs
are in [UPSTREAM.json](UPSTREAM.json). Four codec files now carry a documented
decode-only compatibility adaptation; miniz remains unchanged. GenX-specific
integration lives in CMake and the probe, not a copied Texture, TexLib, AceLib
adapter, renderer or cache implementation.

The library is required by production TSRE5 for ACE reads, independently of
BUILD_TESTING. AceLib adapts document decode to the existing CPU RGB/RGBA Texture
contract. Renderer, GPU upload, cache/season/Undo and texture writers remain
unchanged. Probes and the frozen legacy reader oracle are test-only.

Dependencies: actual Qt6 Core and pinned bundled miniz for bounded inflation.
Upstream Qt-free replacement headers are excluded. No new vcpkg dependency.
Preserve Piotr's authorship and TSRE source lineage. GenX is distributed under
GNU GPL v3 or later (see `../LICENSE.md`). Upstream codec files have no individual
copyright/license banners; this import retains their comments and records
provenance/adaptations here rather than inventing banners. Miniz's separate MIT notice is
preserved in `third_party/miniz/LICENSE`, alongside its upstream version and
modification record. Include the notice in any eventual source export or binary
distribution using this codec.

The codec contains document serialization APIs, but production save dispatch
is not connected to them. The probe uses the current GenX in-memory serializer
to verify historical RGB offset recovery and authored mip output. Temporary
files are used only by the production-adapter/oracle regression fixtures.

See `../../docsInternal/12-acelib-production-loader.md` for production scope,
operator commands, regression coverage and pending manual acceptance. Stage-1
evidence remains in `../../docsInternal/11-acelib-stage1.md`.

GenX adaptation: `AceReadOptions::legacyDxt1Colors` defaults to false. The parity
probe and production adapter enable it to match GenX's bit-replicated BC1
endpoints. It is stored per parsed document and applies to CPU decode/decodeInto/thumbnails only.
Other formats and upstream default scaling remain unchanged. Encoder/serializer
source is unchanged; the compatibility flag is not an output encoding option.

`AceInputValidation.h` is GenX-owned shared preflight policy, used by route-wide
painting and TERRTEX conversion. It uses AceDocument structural parsing with
their distinct existing memory limits; the existing production decode still
follows it. The document is released before decoding. Legacy-layout heuristics
remain in regression/oracle tests only. See the production gate for acceptance.
