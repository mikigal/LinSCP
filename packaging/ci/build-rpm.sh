#!/bin/bash
# Builds the .rpm of LinSCP for the Fedora or openSUSE release it runs on,
# from a source tarball of a release (as root, in a container of the
# release), with packaging/rpm/linscp.spec:
#   packaging/ci/build-rpm.sh SOURCE_TARBALL VERSION RELEASE OUTPUT_DIR [DIST]
# DIST is the suffix of the release where the distribution has none
# (openSUSE): .tumbleweed, .leap16.0.
set -euo pipefail

Tarball=$(realpath "$1")
Version=$2
Release=$3
Output=$(realpath -m "$4")
Dist=${5:-}
Spec=$(realpath "$(dirname "$0")/../rpm/linscp.spec")

. /etc/os-release
case " $ID ${ID_LIKE:-} " in
  *" fedora "*)
    Install=(dnf install -y)
    InstallPackage=(dnf install -y)
    Capability=(cat)
    ;;
  *" suse "*)
    Install=(zypper --non-interactive install --no-recommends)
    InstallPackage=(zypper --non-interactive install --no-recommends --allow-unsigned-rpm)
    # "cmake >= 3.28" -> "cmake>=3.28"
    Capability=(tr -d ' ')
    ;;
  *)
    echo "Not Fedora or openSUSE: $ID" >&2
    exit 1
    ;;
esac
"${Install[@]}" rpm-build

Top=$(mktemp -d)
Defines=(
  --define "_topdir $Top"
  --define "_sourcedir $(dirname "$Tarball")"
  --define "linscp_version $Version"
  --define "linscp_release $Release")
if [ -n "$Dist" ]
then
  Defines+=(--define "dist $Dist")
fi

mapfile -t BuildRequires < <(rpmspec -q --buildrequires "${Defines[@]}" "$Spec" | "${Capability[@]}")
"${Install[@]}" "${BuildRequires[@]}"

rpmbuild -bb "${Defines[@]}" "$Spec"

# Not the -debuginfo and -debugsource packages
Package=$(echo "$Top"/RPMS/*/"linscp-$Version-"*.rpm)
"${InstallPackage[@]}" "$Package"
"$(dirname "$0")/smoke-test.sh" "$Version"

mkdir -p "$Output"
cp "$Package" "$Output/"
