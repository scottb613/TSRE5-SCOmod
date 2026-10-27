# Pack for Distro User Guide

Applies to the v0.20 release.

## Review a Production Route

1. Open the intended production route copy and save all changes.
2. Choose **Route > Pack for Distro...**, below **Create Route Report**.
3. Leave **Preserve assets referenced by editor catalogs and PolyVeg schemas**
   checked if the recipient should retain catalog-only models for later editing.
   Clearing it allows otherwise unreferenced catalog assets to become candidates;
   the catalog files themselves remain in the route.
4. Choose **Scan Full Route**. The scan reads all route files and follows
   dependencies into inspected shared installation files. It does not delete.
5. Review candidate paths, categories, reasons and sizes. Inspect dependency
   problems, shared prerequisites and protected TERRTEX duplicates below the list.
   Warnings do not prevent unrelated cleanup. Blocking safety problems disable deletion.

Placed shapes, their descriptors, referenced textures, matching seasonal files,
ACE/DDS fallbacks and glTF resources are protected. Conservative matching may
retain extra assets. Unknown files outside the recognized cleanup categories
are retained. The scan checks file inclusion and dependency readability, rather
than certifying shape geometry or texture pixels.

## Delete Selected Files

Check only the files to remove, or use **Select All Candidates** after reviewing
all rows. **Delete Selected** shows the actual checked-file count, total MiB and route folder in a permanent-deletion confirmation. It creates
no backup, and editor Undo cannot restore removed files.

The full route is revalidated before deletion. Changed files require a fresh scan. Unreadable dependency formats protect potentially affected assets while unrelated candidates remain available. Keep other programs
from modifying the route during this operation.

The progress bar covers the complete current operation, including validation,
verification and removal. Cancel stops remaining work. If deletion has already
started, the results list shows exactly which files were removed; those files
remain deleted. Scan again before another cleanup pass.

## TERRTEX and Shared Dependencies

Byte-identical TERRTEX groups are identified. Unreferenced copies can be removed. Referenced families can be consolidated only with the explicit option described below.

This command does not compress textures. The separate production DXT1 hack is
lossy and should only be run once. Shared GLOBAL, SOUND and rolling-stock files
are inspected when referenced and reported as prerequisites; cleanup never
removes them or copies them into the route.

Before distribution, review the route report and shared prerequisites, confirm
redistribution rights, and test the production copy in Open Rails. Dependency
scanning does not establish asset licensing or that an external prerequisite
will exist on the recipient's installation.

## Optional Terrain Buffers and Protected Data

Direct `<tile>_e.raw` and `<tile>_n.raw` companions in TILES and LO_TILES are
optional, regenerable buffers. Existing files appear as unchecked **Optional
terrain buffer** candidates, even when named in a terrain descriptor. Their
absence does not create a missing-dependency warning.

`<tile>_y.raw` holds terrain elevations. `<tile>_f.raw` holds terrain flags.
These and all other RAW files are protected, including copies in backup folders.
Cleanup never applies a general `*.raw` removal rule. Select buffer files
individually or leave their checkboxes clear to retain them.

Missing rolling stock, absent scenery and retained opaque geodata are warnings.
They remain visible for distribution review but do not block unrelated cleanup.
Malformed required world/model/terrain data protects the possible target assets;
that protection may conservatively cover whole asset folders. Unsafe route
paths, unreadable route inventory and changes since review still stop cleanup.
## Size Review and Optional Packing Choices

The scan summary lists removable file counts and MiB by category. The selected
file total includes every member of checked consolidation families. These are
file bytes removed; terrain-reference lengths and archive compression can make
final folder/ZIP savings differ slightly. Nothing starts checked for removal.

**Offer consolidation of identical TERRTEX families** is on by default. When
checked before scanning, the packer compares each complete filename family:
base textures and all matching seasonal, night and format variants must have
identical bytes and the same variant set. A matching summer texture alone is
insufficient. Textures also used by shapes or other non-terrain consumers stay
protected. The row's reason/tooltip lists all removed files and their keepers.

Check a **Consolidate TERRTEX** row to update supported terrain texture slots to
its canonical filename, then remove its redundant family. The packer supports
bounded legacy binary, compressed binary and supported text/UTF-16 descriptors;
unsupported cases remain protected. Elevations, flags, patch transforms and
unknown binary fields are preserved. Image bytes are not recompressed.

The operation stages descriptor updates, rechecks source/texture hashes, commits
individual descriptors atomically and rescans before deleting redundant files.
Cancel during reference updates leaves both texture families available. Already
committed descriptors remain updated and are listed in the result. Loaded
terrain refreshes and editor Undo history clears after any committed update.

**Include unreferenced maps, overlays and editing geodata** is on by default.
It offers saved editor map tiles/reference overlays under `terrain_maps` in the
**Editing maps / overlays** category, and files under `osm_data` in the editing
geodata/cache category. These files are editing aids and normally unnecessary
for a playable route. Use **Select Category** to select a group, review its rows,
then **Delete Selected** to remove them from the production route copy.

Nothing is selected automatically. Runtime/catalog references still protect
files, including a map image used by a shape. Malformed retained dependencies
conservatively protect map/cache files whose use cannot be ruled out. RAW data
remains protected even within these folders. Unclassified map-like filenames
outside the known editing folders are retained. Keep the option off when the
recipient needs saved map overlays or geodata for further editing.

Pack for Distro changes the active production folder in place. It creates no
ZIP; archive the resulting folder separately after checking it in Open Rails.
The deletion button says **Delete Selected**. The confirmation states that files
are permanently deleted in place, with no ZIP, backup or Undo. A TERRTEX
reference-update explanation appears only when duplicate families are selected.

All three scan-option checkboxes default on. Every deletion row still starts
unchecked, and deleting files still requires explicit selection and confirmation.

## Category Selection and Size Sorting

Choose a category above the file list, then use **Select Category** or **Clear
Category**. These affect only that category; checked files in other categories
remain unchanged. **Select All Candidates** and **Select None** still apply to
the whole list. Selected totals show file counts, MiB and category subtotals.

The list initially shows the largest removable entries first. Click **MiB** to
switch ascending/descending numeric size order, or click another column to sort
by that column. Sorting uses exact byte counts, including whole-family totals,
so rounded display values do not scramble the order. Sorting does not change
which files are checked.

## Grouped Warnings

The **Warnings** tab shows collapsible groups with counts for rolling stock and
activities, scenery/shapes, sound, environment/weather, textures, terrain,
editing geodata/caches and dependency inspection. Expand a group to inspect the
original full warnings. **Details / Results** shows blocking problems, retained
TERRTEX groups and the actual operation result. Warning groups do not select or
delete files.

## Export the Review and Results

**Export Report...** saves a UTF-8 text report containing scan options/time,
category totals, grouped warnings, candidate paths, checked selections and
retained replacements. After a deletion attempt it also includes actual deleted
files, committed terrain descriptors, errors/cancellation and measured route
sizes. Operation results remain available for export after rescanning while the
dialog remains open. Export before closing if you need the audit record.

The suggested report location is outside the route folder. Existing route files
cannot be overwritten by report export. Saving a new report inside the route
changes its contents and requires another scan before deletion.

## Measured Net Savings

Results distinguish **deleted file bytes** from the whole route's **net
reduction**. The route is measured before and after the operation, including any
growth or shrinkage of rewritten terrain descriptors. If references grow more
than the bytes deleted, the result shows net growth. A cancelled or incomplete
measurement says savings are unavailable rather than reporting a made-up zero.

Totals are logical file sizes at measurement time, before exporting a report.
Filesystem allocation, ZIP compression and unrelated external changes can give
different totals. Keep the route folder unchanged in other programs during the
operation. Total progress includes the final size measurement and terrain
refresh as well as scanning, revalidation, reference updates and deletion.
Category selection separates unused textures, shapes/models, backups (including
recognized MSTS `.bk` files), source artwork and logs/temporary files. RAW and
RAW-backup protection still applies even when a backup extension is recognized.
