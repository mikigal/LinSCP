#!/bin/bash
# Fills in packaging/aur/PKGBUILD for a release, builds and checks the
# package with it from the release's source tarball, and writes its .SRCINFO
# (as root, in a container of Arch Linux; makepkg runs as a user of its own):
#   packaging/ci/build-aur.sh SOURCE_TARBALL TAG VERSION RELEASE OUTPUT_DIR
# OUTPUT_DIR gets the PKGBUILD and .SRCINFO to publish in the AUR.
set -euo pipefail

Tarball=$(realpath "$1")
Tag=$2
Version=$3
Release=$4
Output=$(realpath -m "$5")
Here=$(realpath "$(dirname "$0")")

pacman -Syu --noconfirm --needed base-devel

Work=$(mktemp -d)
Sha256=$(sha256sum "$Tarball" | cut -d ' ' -f 1)
sed -e "s/@VERSION@/$Version/" -e "s/@RELEASE@/$Release/" -e "s/@TAG@/$Tag/" \
  -e "s/@SHA256@/$Sha256/" "$Here/../aur/PKGBUILD" > "$Work/PKGBUILD"
# makepkg takes the source that is there already instead of downloading it
cp "$Tarball" "$Work/linscp-$Version-$Release.tar.gz"

# The dependencies, as makepkg --syncdeps would install them
mapfile -t Dependencies < <(cd "$Work" && bash -c \
  'source PKGBUILD && printf "%s\n" "${depends[@]}" "${makedepends[@]}"')
pacman -S --noconfirm --needed "${Dependencies[@]}"

useradd --create-home linscp-builder
chown -R linscp-builder "$Work"
(cd "$Work" && runuser -u linscp-builder -- makepkg --noconfirm)
(cd "$Work" && runuser -u linscp-builder -- makepkg --printsrcinfo > .SRCINFO)

pacman -U --noconfirm "$Work/linscp-$Version-$Release-"*.pkg.tar.zst
"$Here/smoke-test.sh" "$Version"

mkdir -p "$Output"
cp "$Work/PKGBUILD" "$Work/.SRCINFO" "$Output/"
