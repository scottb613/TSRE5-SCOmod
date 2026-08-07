# GitHub Actions release workflow

TSRE GenX releases are deliberately manual. Pushing a branch or tag does not
create a GitHub Release and does not publish an executable.

The workflow files must be present on the repository's default branch before
GitHub displays their **Run workflow** controls.

## One-time repository setup

In **Settings > Environments**, create an environment named `release`. Add the
repository owner as a required reviewer when that option is available. Both
release workflows target this environment, so its protection rules provide the
last approval gate before GitHub grants write access.

Keep the repository's default workflow-token permission read-only. Each release
job requests only `contents: write`; all other jobs remain read-only.

## Prepare the public source branch locally

The development checkout and public source branch intentionally use different
layouts. The development checkout retains `TSREvcWIP`, private migration
records, build evidence, snapshots, and distribution staging. The public branch
contains only the flattened maintained source, CMake support, tests, workflows,
and the approved versioned document set.

Create a linked worktree for the existing public WIP branch, then run the
guarded exporter. The exporter accepts only a clean worktree below
`tmp/release-worktrees`, validates version metadata and the document manifest,
removes the prior tracked public tree, exports the approved layout, rejects
private/generated artifacts, stages the result, and prints the staged manifest.

```powershell
git worktree add -b tsre-scomod-wip .\tmp\release-worktrees\v0.11 origin/tsre-scomod-wip 2>&1 | Tee-Object -FilePath .\AAA_Git-v0.11-prepare.log
.\scripts\Prepare-SourceRelease.ps1 -Version v0.11 -Destination .\tmp\release-worktrees\v0.11 -Stage 2>&1 | Tee-Object -FilePath .\AAA_Git-v0.11-prepare.log -Append
```

Review `git -C .\tmp\release-worktrees\v0.11 diff --cached` before committing.
Never stage the development checkout wholesale.

## Source-only checkpoint

Run **Prepare TSRE GenX release** with:

- the new version tag, such as `v0.11`;
- the branch channel (`wip`, `master`, or `stable`);
- `source-checkpoint`;
- the exact displayed confirmation, such as
  `CREATE v0.11 FROM tsre-scomod-wip`.

The workflow verifies the branch head, version metadata, release notes, and
public source layout. It refuses an existing tag rather than moving it. A
successful source checkpoint creates only an annotated tag. It creates no
GitHub Release and uploads no executable or package.

## Prepare a full release

Complete the normal Push Local process first: operator compile, TST acceptance,
documentation review, approved `dist` ZIP, and a recorded SHA-256.

Run **Prepare TSRE GenX release** using `create-draft-release`. The workflow
creates the annotated tag and a draft GitHub Release using
`RELEASE-NOTES-<tag>.md`. A draft is not publicly released.

Upload the approved ZIP to that draft. This can be done in the GitHub draft
release editor or from the approved local checkout:

```powershell
gh release upload v0.11 dist\tsre-scomod-v0.11.zip --repo scottb613/TSRE5-SCOmod 2>&1 | Tee-Object -FilePath .\AAA_Git-v0.11-release.log
```

Do not upload loose executables, DLLs, PDBs, or unrelated archives.

## Publish the approved ZIP

Run **Publish approved TSRE GenX release** with:

- the existing version tag and containing branch;
- `stable` or `prerelease` classification;
- the exact uploaded ZIP filename;
- its 64-character SHA-256;
- the confirmation `PUBLISH <tag>`.

The workflow requires an unpublished draft, confirms that the tag belongs to
the selected branch, rejects unexpected assets, downloads the ZIP, and compares
its SHA-256. Only after all checks pass does it add a checksum file and publish
the draft. Stable releases become Latest; prereleases do not.

## Recovery and safety

- Existing tags are never moved or replaced.
- Published releases are never overwritten.
- Concurrent release operations are serialized.
- A failed preparation before the tag-push step changes nothing remotely.
- If draft creation fails after tag creation, inspect the run before manually
  creating the draft; do not delete or move the tag casually.
- A failed publication leaves the release as a draft unless the final GitHub
  API request already succeeded.
- Deleting tags or published releases remains a manual owner operation.
