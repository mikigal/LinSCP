#!/bin/bash
# Records the bcb2clang conversion of an upstream WinSCP revision on the
# upstream-converted branch, see docs/port/UPSTREAM.md.
#
# Usage: tools/bcb2clang/update-upstream.sh [REVISION [MESSAGE_SUFFIX]]
#
# REVISION defaults to upstream/master (fetch it first). The conversion runs
# on a pristine checkout of REVISION, with the converter of the current
# working tree. The new commit has the previous upstream-converted commit (if
# any) and REVISION as parents. MESSAGE_SUFFIX, if given, is appended to the
# commit message as a separate paragraph. Merge the branch into master
# afterwards: git merge upstream-converted
set -euo pipefail

ROOT=$(git rev-parse --show-toplevel)
REVISION=${1:-upstream/master}
SUFFIX=${2:-}
NEW=$(git rev-parse --verify "$REVISION^{commit}")

WORK=$(mktemp -d "${TMPDIR:-/tmp}/linscp-convert.XXXXXX")
cleanup()
{
  git -C "$ROOT" worktree remove --force "$WORK/tree" 2>/dev/null || true
  rm -rf "$WORK"
}
trap cleanup EXIT

git -C "$ROOT" worktree add --quiet --detach "$WORK/tree" "$NEW"
"$ROOT/tools/bcb2clang/convert.py" "$WORK/tree"
git -C "$WORK/tree" add --all
TREE=$(git -C "$WORK/tree" write-tree)

if PREVIOUS=$(git -C "$ROOT" rev-parse --quiet --verify refs/heads/upstream-converted); then
  if [ "$(git -C "$ROOT" rev-parse "$PREVIOUS^{tree}")" = "$TREE" ]; then
    echo "upstream-converted already has this conversion of ${NEW:0:9}"
    exit 0
  fi
  PARENTS=(-p "$PREVIOUS" -p "$NEW")
else
  PARENTS=(-p "$NEW")
fi

MESSAGE="Convert upstream ${NEW:0:9} with bcb2clang"
if [ -n "$SUFFIX" ]; then
  MESSAGE=$(printf '%s\n\n%s' "$MESSAGE" "$SUFFIX")
fi
COMMIT=$(printf '%s\n' "$MESSAGE" | git -C "$ROOT" commit-tree "$TREE" "${PARENTS[@]}")
git -C "$ROOT" branch --force upstream-converted "$COMMIT"
echo "upstream-converted is now ${COMMIT:0:9}; merge it: git merge upstream-converted"
