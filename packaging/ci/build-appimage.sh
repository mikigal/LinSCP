#!/bin/bash
# Builds the AppImage of LinSCP and a tarball of the same files (the
# application with its Qt), from the source tree it is in (as root, in a
# container of Ubuntu 24.04, the oldest system the binaries run on):
#   packaging/ci/build-appimage.sh VERSION OUTPUT_DIR
# Ubuntu 24.04's Qt is too old (6.4): Qt comes from the Qt Company's binaries,
# through aqtinstall. OpenSSL is the system's (its configuration and the
# certificates of the system are where its own build expects them).
set -euo pipefail

Version=$1
Output=$(realpath -m "$2")
cd "$(dirname "$0")/../.."
Source=$PWD

QtVersion=6.12.0
AqtinstallVersion=3.3.0
ClangVersion=20
LinuxdeployVersion=1-alpha-20251107-1
# The plugin's last numbered release (2025-02) predates Qt 6.12
LinuxdeployPluginQtVersion=continuous
AppimagetoolVersion=1.9.1
Architecture=$(uname -m)

export DEBIAN_FRONTEND=noninteractive
apt-get update
# The build, and the libraries the plugins of Qt load (linuxdeploy resolves
# them)
apt-get install -y --no-install-recommends \
  ca-certificates curl file python3 python3-venv \
  clang-$ClangVersion cmake ninja-build libexpat1-dev libssl-dev \
  libgl-dev libegl-dev libxkbcommon-dev libfontconfig1-dev libfreetype-dev \
  libdbus-1-3 libglib2.0-0t64 libwayland-client0 libwayland-cursor0 libwayland-egl1 \
  libx11-xcb1 libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 libxcb-image0 \
  libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-sync1 \
  libxcb-xfixes0 libxcb-xinerama0 libxcb-xkb1 libsm6 libice6

Work=$(mktemp -d)
python3 -m venv "$Work/aqt"
"$Work/aqt/bin/pip" install --quiet "aqtinstall==$AqtinstallVersion"
"$Work/aqt/bin/aqt" install-qt --outputdir "$Work/qt" linux desktop "$QtVersion" linux_gcc_64
QtDir=$Work/qt/$QtVersion/gcc_64

cmake -S "$Source" -B "$Work/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_C_COMPILER=clang-$ClangVersion \
  -DCMAKE_CXX_COMPILER=clang++-$ClangVersion \
  -DCMAKE_PREFIX_PATH="$QtDir" \
  -DLINSCP_SANITIZE=OFF \
  -DLINSCP_BUILD_TESTS=OFF \
  -DLINSCP_BUILD_HARNESS=OFF \
  -DLINSCP_AUTOMATION=OFF \
  -DLINSCP_RELEASE_BUILD=ON
cmake --build "$Work/build"
AppDir=$Work/AppDir
DESTDIR="$AppDir" cmake --install "$Work/build"

Tools=$Work/tools
mkdir -p "$Tools"
Download()
{
  curl --fail --location --silent --show-error --output "$Tools/$2" "$1"
  chmod +x "$Tools/$2"
}
Download "https://github.com/linuxdeploy/linuxdeploy/releases/download/$LinuxdeployVersion/linuxdeploy-$Architecture.AppImage" \
  "linuxdeploy-$Architecture.AppImage"
Download "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/$LinuxdeployPluginQtVersion/linuxdeploy-plugin-qt-$Architecture.AppImage" \
  "linuxdeploy-plugin-qt-$Architecture.AppImage"
Download "https://github.com/AppImage/appimagetool/releases/download/$AppimagetoolVersion/appimagetool-$Architecture.AppImage" \
  "appimagetool-$Architecture.AppImage"

# No FUSE in a container
export APPIMAGE_EXTRACT_AND_RUN=1
export PATH="$Tools:$PATH"
export LD_LIBRARY_PATH="$QtDir/lib"
export QMAKE="$QtDir/bin/qmake"
# The SVG icons, and Wayland next to X11
export EXTRA_QT_MODULES="svg"
EXTRA_PLATFORM_PLUGINS=$(cd "$QtDir/plugins/platforms" && ls libqwayland*.so | paste -sd ';')
export EXTRA_PLATFORM_PLUGINS
"linuxdeploy-$Architecture.AppImage" --appdir "$AppDir" \
  --executable "$AppDir/usr/bin/linscp" \
  --executable "$AppDir/usr/bin/linscp-cli" \
  --desktop-file "$AppDir/usr/share/applications/linscp.desktop" \
  --icon-file "$AppDir/usr/share/icons/hicolor/scalable/apps/linscp.svg" \
  --custom-apprun "$Source/packaging/appimage/AppRun" \
  --exclude-library 'libssl.so*' \
  --exclude-library 'libcrypto.so*' \
  --plugin qt
unset LD_LIBRARY_PATH
echo "The plugins of Qt:"
(cd "$AppDir/usr" && find plugins -name '*.so' | sort)

mkdir -p "$Output"
AppImage=$Output/LinSCP-$Version-$Architecture.AppImage
ARCH=$Architecture "appimagetool-$Architecture.AppImage" --no-appstream "$AppDir" "$AppImage"

# The same files as a tarball, started by its links linscp and linscp-cli
Name=linscp-$Version-$Architecture
cp -a "$AppDir" "$Work/$Name"
ln -s usr/bin/linscp "$Work/$Name/linscp"
ln -s usr/bin/linscp-cli "$Work/$Name/linscp-cli"
tar -C "$Work" --owner=0 --group=0 -czf "$Output/$Name.tar.gz" "$Name"

ln -s "$AppImage" "$Work/linscp-cli"
packaging/ci/smoke-test.sh "$Version" "$Work/linscp-cli" "$AppDir/usr/bin/linscp"
