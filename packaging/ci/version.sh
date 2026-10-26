#!/bin/bash
# The version of a release from its tag, for the release workflow
# (.github/workflows/release.yml):
#   packaging/ci/version.sh [TAG] >> "$GITHUB_OUTPUT"
# A tag is vX.Y.Z or vX.Y.Z-N: X.Y.Z must be the version of the core
# (FileVersion of source/WinSCP.cbproj, the version LinSCP reports), N is the
# release of the packages (1 when left out), raised for releases of LinSCP
# on the same core. Without a tag (a test build): the core's version,
# release 0. Prints the lines version=, release= and tag=.
set -euo pipefail

Tag=${1:-}
cd "$(dirname "$0")/../.."
FileVersion=$(sed -n 's/.*FileVersion=\([0-9.]*\);.*/\1/p' source/WinSCP.cbproj | head -n 1)
# 6.7.1.0 -> 6.7.1
Core=$(echo "$FileVersion" | cut -d . -f 1-3)
if [[ ! "$Core" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
then
  echo "No FileVersion in source/WinSCP.cbproj" >&2
  exit 1
fi

if [ -z "$Tag" ]
then
  Version=$Core
  Release=0
elif [[ "$Tag" =~ ^v([0-9]+\.[0-9]+\.[0-9]+)(-([1-9][0-9]*))?$ ]]
then
  Version=${BASH_REMATCH[1]}
  Release=${BASH_REMATCH[3]:-1}
  if [ "$Version" != "$Core" ]
  then
    echo "The tag $Tag is not the version of the core, $Core (source/WinSCP.cbproj)" >&2
    exit 1
  fi
else
  echo "The tag $Tag is not vX.Y.Z or vX.Y.Z-N" >&2
  exit 1
fi

echo "version=$Version"
echo "release=$Release"
echo "tag=$Tag"
