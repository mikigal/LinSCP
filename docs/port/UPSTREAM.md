# Pulling upstream WinSCP releases

LinSCP keeps pulling upstream WinSCP (security and protocol fixes). Because the core is
mechanically converted from the C++Builder dialect (`tools/bcb2clang`, plan section 3.2),
upstream is never merged into `master` directly. Instead, a vendor branch carries the
*converted* upstream, so a merge only shows upstream's real changes plus our manual edits.

## Refs

| Ref | Content |
|---|---|
| `upstream/master` | pristine WinSCP, remote `upstream` = `https://github.com/winscp/winscp.git` |
| `upstream-converted` | local branch: each commit's tree is exactly `bcb2clang(upstream at some revision)` |
| `master` | LinSCP: merges of `upstream-converted` + our own commits |

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
- `readme.md` is LinSCP's own: upstream's text was replaced, keeping the file's name and CRLF
  line endings. When a merge conflicts on it, keep LinSCP's version.

## Ported application files

The application layer of upstream (`source/windows`) mixes portable logic with VCL and Win32
code. The files that the Linux front ends are ported from are listed in `PORTED_FILES` of
`tools/bcb2clang/convert.py`: the converter writes their converted copies to the directories
of the Linux application layer (`source/app`, shared by the front ends, and `source/cli`),
while `source/windows` itself stays pristine. The copies are then edited on `master` for
Linux, so upstream changes of these files arrive through the regular merge of
`upstream-converted`, and conflicts show where they meet our edits. The other files in these
directories are our own.

To port another file, add it to `PORTED_FILES`, run `update-upstream.sh` for the revision
that is already converted (see below) and merge, then edit the copy.

## Texts and translations

The GUI is written in Qt, but its texts are the English texts of upstream's forms
(`source/forms/*.dfm`) and its resource strings are upstream's (`source/resource/*.rc`):
upstream's translations (`translations/*.ini`, kept as they are) translate them, through the
keys of the forms (`tools/dfm2cpp` generates the English texts at build time) and the names of
the strings (`tools/rc2cpp`). When a merge changes the English text of a form, change the
same text of the Qt dialog, or it stays untranslated. The name of the application in
upstream's texts is replaced (`rc2cpp --brand`, `BrandText`).

## Converter

`tools/bcb2clang/convert.py ROOT` converts a tree in place: `source/core` from the C++Builder
dialect (`bcb2clang.py`, with method references resolved against the classes of all core
files), and the `#include` paths of the other built upstream directories (`source/putty`,
`source/filezilla`). The conversion keeps every line where it was (a `try`/`__finally`
statement keeps its line count, its `__finally` block moves in front of the `try` block), so
diffs between converted revisions stay small.

## Updating to a new upstream revision

`tools/bcb2clang/update-upstream.sh [REVISION [MESSAGE_SUFFIX]]` (default `upstream/master`)
records a conversion on `upstream-converted`:

```bash
git fetch upstream
tools/bcb2clang/update-upstream.sh      # converts upstream/master
git merge upstream-converted            # on master
```

The script checks out REVISION pristine in a temporary worktree, runs `convert.py` from the
current working tree on it, and commits the result with the previous `upstream-converted`
commit and REVISION as parents (the converter's output for *pristine* upstream sources, not the
converter run over a merge result). The merge base of the following merge into `master` is the
previous `upstream-converted` commit, so the merge applies `convert(new) - convert(old)` onto
`master`. Conflicts appear only where our manual changes overlap upstream changes. After
resolving them: build, run the full test suite, commit.

When the new revision changes the core's version (`FileVersion` of `source/WinSCP.cbproj`),
`cmake` stops until LinSCP's version (`project()` in `CMakeLists.txt`, see
`cmake/LinSCPVersion.cmake`) is set to it: set it to the new X.Y.Z, without a fourth part, in
the merge commit.

**When the converter changes**, run the script again for the upstream revision that is already
converted, and merge: `master` then receives exactly the converter's changes. The script does
nothing if the conversion did not change.

The branch was created on 2026-10-24 by `update-upstream.sh 3b3f02479` (no previous
`upstream-converted` commit: the only parent is the upstream revision).
