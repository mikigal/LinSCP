#!/bin/bash
# Fills in the PKGBUILDs of the AUR (packaging/aur) for a release, builds and
# checks their packages, and writes their .SRCINFO (as root, in a container
# of Arch Linux; makepkg runs as a user of its own):
#   packaging/ci/build-aur.sh SOURCE_TARBALL TAG VERSION RELEASE OUTPUT_DIR
# linscp builds the release's source tarball; its package goes to the
# release, and linscp-bin repackages it. OUTPUT_DIR gets aur/linscp and
# aur/linscp-bin (the PKGBUILD and .SRCINFO to publish in the AUR) and
# release/ (the package of linscp).
set -euo pipefail

Tarball=$(realpath "$1")
Tag=$2
Version=$3
Release=$4
Output=$(realpath -m "$5")
Here=$(realpath "$(dirname "$0")")
Architecture=$(uname -m)

pacman -Syu --noconfirm --needed base-devel
useradd --create-home linscp-builder
Work=$(mktemp -d)
chown linscp-builder "$Work"

# Fills in the PKGBUILD of a package of the AUR, with SOURCE as its source,
# copied next to it under the name the PKGBUILD gives it (makepkg takes the
# source that is there already instead of downloading it):
#   Prepare PACKAGE SOURCE NAME
Prepare()
{
  local Sha256
  Sha256=$(sha256sum "$2" | cut -d ' ' -f 1)
  mkdir "$Work/$1"
  sed -e "s/@VERSION@/$Version/" -e "s/@RELEASE@/$Release/" -e "s/@TAG@/$Tag/" \
    -e "s/@SHA256@/$Sha256/" "$Here/../aur/$1/PKGBUILD" > "$Work/$1/PKGBUILD"
  cp "$2" "$Work/$1/$3"
  chown -R linscp-builder "$Work/$1"
}

# Builds the package of a PKGBUILD prepared by Prepare, writes its .SRCINFO,
# installs it and checks it
Build()
{
  (cd "$Work/$1" && runuser -u linscp-builder -- makepkg --noconfirm)
  (cd "$Work/$1" && runuser -u linscp-builder -- makepkg --printsrcinfo > .SRCINFO)
  pacman -U --noconfirm "$Work/$1/$1-$Version-$Release-$Architecture.pkg.tar.zst"
  "$Here/smoke-test.sh" "$Version"
}

Prepare linscp "$Tarball" "linscp-$Version-$Release.tar.gz"
# The dependencies, as makepkg --syncdeps would install them
mapfile -t Dependencies < <(cd "$Work/linscp" && bash -c \
  'source PKGBUILD && printf "%s\n" "${depends[@]}" "${makedepends[@]}"')
pacman -S --noconfirm --needed "${Dependencies[@]}"
Build linscp
Package=$Work/linscp/linscp-$Version-$Release-$Architecture.pkg.tar.zst

pacman -R --noconfirm linscp
Prepare linscp-bin "$Package" "$(basename "$Package")"
Build linscp-bin

for Name in linscp linscp-bin
do
  mkdir -p "$Output/aur/$Name"
  cp "$Work/$Name/PKGBUILD" "$Work/$Name/.SRCINFO" "$Output/aur/$Name/"
done
mkdir -p "$Output/release"
cp "$Package" "$Output/release/"
