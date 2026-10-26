# The package of LinSCP for Fedora and openSUSE, built from the source
# tarball of a release by packaging/ci/build-rpm.sh, which defines the
# version and the release.

# The core relies on Clang's extensions (-fdeclspec): Fedora's flags for
# Clang; the LTO options of the distributions' flags are GCC's
%global toolchain clang
%global _lto_cflags %{nil}

Name:           linscp
Version:        %{linscp_version}
Release:        %{linscp_release}%{?dist}
Summary:        SFTP, SCP, FTP, WebDAV and S3 client with a two-panel file manager
License:        GPL-3.0-or-later
URL:            https://github.com/mikigal/LinSCP
Source0:        %{name}-%{version}-source.tar.gz

BuildRequires:  clang
BuildRequires:  cmake >= 3.28
%if 0%{?suse_version}
BuildRequires:  ninja
%else
BuildRequires:  ninja-build
%endif
BuildRequires:  python3 >= 3.11
BuildRequires:  pkgconfig(expat) >= 2.5
%if 0%{?suse_version}
# Not LibreSSL, which provides pkgconfig(openssl) too
BuildRequires:  libopenssl-devel >= 3
%else
BuildRequires:  pkgconfig(openssl) >= 3
%endif
BuildRequires:  cmake(Qt6DBus) >= 6.5
BuildRequires:  cmake(Qt6Widgets) >= 6.5
BuildRequires:  desktop-file-utils

Requires:       hicolor-icon-theme
Suggests:       putty
Suggests:       xdg-utils
%if 0%{?suse_version}
Requires:       libQt6Svg6
Recommends:     qt6-wayland
Suggests:       libcanberra0
Suggests:       openssh-clients
Suggests:       qt6-translations
%else
Requires:       qt6-qtsvg
Recommends:     qt6-qtwayland
Suggests:       libcanberra
Suggests:       openssh-clients
Suggests:       qt6-qttranslations
%endif

%description
LinSCP is a native Linux (Wayland) port of WinSCP. It runs WinSCP's own core
for SFTP, SCP, FTP, FTPS, WebDAV and Amazon S3 with a Qt 6 interface: the
Commander and Explorer interfaces, the transfer queue, synchronization,
a remote editor and the scripting of winscp.com (linscp-cli).

%prep
%autosetup

%build
%{?set_build_flags}
export CFLAGS="${CFLAGS:-%{optflags}}"
export CXXFLAGS="${CXXFLAGS:-%{optflags}}"
export CC=clang
export CXX=clang++
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DLINSCP_SANITIZE=OFF \
  -DLINSCP_BUILD_TESTS=OFF \
  -DLINSCP_BUILD_HARNESS=OFF \
  -DLINSCP_AUTOMATION=OFF \
  -DLINSCP_RELEASE_BUILD=ON
cmake --build build %{?_smp_mflags}

%install
DESTDIR=%{buildroot} cmake --install build

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/linscp.desktop

%files
%dir %{_datadir}/licenses/%{name}
%license %{_datadir}/licenses/%{name}/license.txt
%{_bindir}/linscp
%{_bindir}/linscp-cli
%{_datadir}/applications/linscp.desktop
%{_datadir}/icons/hicolor/scalable/apps/linscp.svg
%{_datadir}/linscp/

%changelog
