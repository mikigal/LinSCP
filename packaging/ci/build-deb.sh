#!/bin/bash
# Builds the .deb of LinSCP for the Debian or Ubuntu release it runs on, from
# the source tree it is in (as root, in a container of the release):
#   packaging/ci/build-deb.sh VERSION RELEASE OUTPUT_DIR
# The package's version is VERSION-RELEASE~debian13 (~ubuntu26.04, ...), so
# that a distribution's own package of the same version wins.
set -euo pipefail

Version=$1
Release=$2
Output=$(realpath -m "$3")
cd "$(dirname "$0")/../.."

. /etc/os-release
Distribution=$ID$VERSION_ID
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends dpkg-dev

rm -rf debian
cp -r packaging/debian debian
Maintainer=$(sed -n 's/^Maintainer: //p' debian/control)
cat > debian/changelog <<EOF
linscp ($Version-$Release~$Distribution) $VERSION_CODENAME; urgency=medium

  * LinSCP $Version-$Release.

 -- $Maintainer  $(date -R)
EOF

apt-get build-dep -y --no-install-recommends ./
export DEB_BUILD_OPTIONS="parallel=$(nproc) nocheck"
dpkg-buildpackage -b -us -uc

Architecture=$(dpkg --print-architecture)
Package=$(realpath "../linscp_$Version-$Release~${Distribution}_$Architecture.deb")
apt-get install -y --no-install-recommends "$Package"
packaging/ci/smoke-test.sh "$Version"

mkdir -p "$Output"
cp "$Package" "$Output/linscp_$Version-${Release}_${Distribution}_$Architecture.deb"
