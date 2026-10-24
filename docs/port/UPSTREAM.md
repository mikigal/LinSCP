# Pulling upstream WinSCP releases

OpenSCP keeps pulling upstream WinSCP (security and protocol fixes). Because the core is
mechanically converted from the C++Builder dialect (`tools/bcb2clang`, plan section 3.2),
upstream is never merged into `master` directly. Instead, a vendor branch carries the
*converted* upstream, so a merge only shows upstream's real changes plus our manual edits.

## Refs

| Ref | Content |
|---|---|
| `upstream/master` | pristine WinSCP, remote `upstream` = `https://github.com/winscp/winscp.git` |
| `upstream-converted` | local branch: each commit's tree is exactly `bcb2clang(upstream at some revision)` |
| `master` | OpenSCP: merges of `upstream-converted` + our own commits |

Fork point: `master` was created from upstream `3b3f02479` (2026-10-24, Issue 2477 fix).

## Rules that keep merges cheap

- Never hand-edit files on `upstream-converted`; its trees are produced by the converter only.
- Keep manual changes in upstream directories (`source/core`, `source/putty`, `source/filezilla`,
  `libs/neon`, `libs/libs3`) minimal; put new code in new directories (`source/rtl`,
  `source/linux`, `source/qt`, `tools`, `tests`).
- Do not delete or modify Windows-only upstream directories (`source/forms`, `source/windows`,
  `source/packages`, `source/components`, `source/dragext`, `dotnet`, `deployment`); they are
  not built, but changing them would only create merge conflicts. They also serve as the
  reference for the Qt GUI.
- When the converter itself changes, its new output arrives through the next
  `upstream-converted` commit like any other change.

## Initial setup (milestone M2, once the converter exists)

```bash
git switch -c upstream-converted 3b3f02479
tools/bcb2clang/convert.py .            # converts the tree in place
git add -A && git commit -m "Convert upstream 3b3f02479 with bcb2clang"
git switch master
git merge upstream-converted            # no conflicts: master has not touched these files yet
```

## Updating to a new upstream revision

The new `upstream-converted` commit must contain the converter's output for *pristine* new
upstream sources (not the converter run over a merge result), while recording upstream as a
parent so that history stays connected:

```bash
git fetch upstream
NEW=$(git rev-parse upstream/master)
git worktree add ../openscp-convert "$NEW"           # pristine new upstream
tools/bcb2clang/convert.py ../openscp-convert        # converter from master's working tree
git -C ../openscp-convert add -A
TREE=$(git -C ../openscp-convert write-tree)
COMMIT=$(git commit-tree "$TREE" -p upstream-converted -p "$NEW" \
  -m "Convert upstream ${NEW:0:9} with bcb2clang")
git branch -f upstream-converted "$COMMIT"
git worktree remove --force ../openscp-convert

git switch master
git merge upstream-converted            # base = previous upstream-converted commit
```

The merge base is the previous `upstream-converted` commit, so the merge applies
`convert(new) − convert(old)` onto `master`. Conflicts appear only where our manual changes
overlap upstream changes. After resolving them: build, run the full test suite, commit.

This sequence will be wrapped in `tools/bcb2clang/update-upstream.sh` in M2.
