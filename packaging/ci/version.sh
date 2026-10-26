#!/bin/bash
# The version of a release from its tag, for the release workflow
# (.github/workflows/release.yml):
#   packaging/ci/version.sh [TAG] >> "$GITHUB_OUTPUT"
# A tag is vVERSION or vVERSION-R. VERSION must be LinSCP's version (project()
# in CMakeLists.txt, the version LinSCP reports): X.Y.Z, the version of the
# core (FileVersion of source/WinSCP.cbproj), for the first release on that
# core, X.Y.Z.N for LinSCP's further releases on it (see
# cmake/LinSCPVersion.cmake). R is the release of the packages (1 when left
# out), raised for new packages of the same version (packaging only). Without
# a tag (a test build): LinSCP's version, release 0. Prints the lines
# version=, release= and tag=.
set -euo pipefail

Tag=${1:-}
cd "$(dirname "$0")/../.."
Version=$(sed -n '/^project(LinSCP/,/)/s/^ *VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
if [[ ! "$Version" =~ ^([0-9]+\.[0-9]+\.[0-9]+)(\.[1-9][0-9]*)?$ ]]
then
  echo "No version X.Y.Z or X.Y.Z.N in project() of CMakeLists.txt: $Version" >&2
  exit 1
fi
Base=${BASH_REMATCH[1]}
FileVersion=$(sed -n 's/.*FileVersion=\([0-9.]*\);.*/\1/p' source/WinSCP.cbproj | head -n 1)
# 6.7.1.0 -> 6.7.1
Core=$(echo "$FileVersion" | cut -d . -f 1-3)
if [ "$Base" != "$Core" ]
then
  echo "LinSCP's version $Version is not of the core, $Core (source/WinSCP.cbproj)" >&2
  exit 1
fi

if [ -z "$Tag" ]
then
  Release=0
elif [[ "$Tag" =~ ^v([0-9]+\.[0-9]+\.[0-9]+(\.[1-9][0-9]*)?)(-([1-9][0-9]*))?$ ]]
then
  Release=${BASH_REMATCH[4]:-1}
  if [ "${BASH_REMATCH[1]}" != "$Version" ]
  then
    echo "The tag $Tag is not LinSCP's version, $Version (project() in CMakeLists.txt)" >&2
    exit 1
  fi
else
  echo "The tag $Tag is not vX.Y.Z, vX.Y.Z.N, vX.Y.Z-R or vX.Y.Z.N-R" >&2
  exit 1
fi

echo "version=$Version"
echo "release=$Release"
echo "tag=$Tag"
