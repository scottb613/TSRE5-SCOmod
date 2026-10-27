# Static Object Copy To User Guide

Applies to the v0.20 release.

## Choose the Receiving Route

1. Open the source route and choose **Route > Copy To...**.
2. Leave **Choose Working Folder (no REF entry)** clear. Choose the receiving
   route's folder, containing its `.trk` file.
3. Review the validated route and REF path, then choose **Use Destination**.

The previous route folder, working folder and selected mode are remembered
between editor sessions. Return to this screen to change the destination or
choose **Clear Destination**. Clearing disables copying but retains both folder
choices for later selection. Cancel leaves the active destination unchanged.
Revalidation runs silently before copying; only errors need attention. The
screen shows whether the folder is valid. The source and destination must be
separate, nonoverlapping route folders. The receiving route must be writable.
Keep the receiving route closed, or reload its REF before using copied objects;
another editor's stale catalog must not overwrite the updated file.

## Copy to a Working Folder for Editing

1. Choose **Route > Copy To...** and check **Choose Working Folder (no REF entry)**.
2. Choose an existing writable folder, separate from the active route, and
   choose **Use Destination**. No route descriptor is required in this mode.
3. Select a Static Object, right-click and choose **Copy To...** in its own **Copy To** section.

Copying starts directly without a progress bar; no REF name/category dialog is
shown. A new folder named after the selected model contains its model, SD and
texture resources. SHAPES/TEXTURES and any relative resource subfolders are
retained inside that object folder so files remain organized for editing.
Repeated copies get suffixes such as `_2`, preserving earlier exports. This
mode exports the selected model; source REF alternative models are not exported.
No REF file is read or written in the working folder. A completion message gives
the new folder's path. A failed transfer removes newly copied resources and
empty folders owned by this operation; existing working files remain unchanged.

## Copy a Static Object to a Route

1. Select one placed Static Object and right-click it.
2. In the context menu's separate **Copy To** section, choose **Copy To...**. This action is
   shown only when a destination is selected and a supported object is selected.
3. Review the resource list: **COPY** adds a file; **REUSE** keeps an identical
   destination file. Missing resources or same-name files with different contents
   stop the transfer before copying.
4. **Name** starts with the selected shape's filename without its path or extension;
   edit it if desired. Choose **Category** from the editable destination list.
   These become the REF description and class. All other settings
   are automatic. An existing name/category pair requires a different name.
5. Choose **Copy Object** and wait for the completion message. Reload the
   receiving route's REF to place it. Copy To shows no progress bars.

The following Name/Category rules apply only to route destinations.
Names/categories cannot contain quotes, backslashes or control characters.
Categories beginning with `#` are reserved for editor tools. A legacy 8-bit REF
cannot accept characters outside its encoding; the dialog reports this without
changing the file. Existing REF bytes are preserved; a missing REF is created
as UTF-16LE.

## Resources and Settings

Native `.s` models retain their original bytes, compression and `.sd` companion.
Referenced textures include available seasonal/night variants and both ACE/DDS
counterparts. A missing `.sd` is reported as a warning; model defaults apply.
Static glTF/GLB copies include external images and buffers while preserving
their relative folder layout. Embedded resources remain inside their model.
Unsafe paths, links, unsupported models and missing dependencies stop the copy.
Unsaved generated resources must be saved or baked first.

For route copies, matching source REF entries retain model alternatives, alignment,
selection method and random placement ranges. Different definitions for the
same model require selecting the intended catalog entry before copying.
The selected instance's static flags, animation/shadow/terrain flags, detail
level and collision settings are stored as GenX REF metadata and restored when
GenX places the new object. Other editors may ignore these additional fields.
The model and `.sd` settings remain compatible file copies. Source placement
coordinates, orientation and scale are not copied into the destination world;
this operation adds a reusable catalog entry, ready for a new placement.

## Completion and Failed Transfers

Validation, resource discovery, staging, copying and verification run without
progress dialogs. Completion and error messages report the result. You can
cancel the route resource review before choosing Copy Object; once copying
starts, it runs to completion. Large transfers use temporary disk staging.
Allow room for the staged resources plus new destination files.

The REF is saved atomically after all resource checks succeed. A failure
before that point removes newly copied files and leaves the REF
unchanged. Preexisting destination resources are never overwritten or removed.
If another process changes a newly copied file, cleanup retains and reports
it. An abrupt computer/process failure can leave unreferenced resource files;
the result is not a multi-file crash-recovery transaction. Keep other programs
from modifying the receiving route during the transfer.
