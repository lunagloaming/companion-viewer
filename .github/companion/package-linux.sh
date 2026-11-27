#!/bin/bash
# Makes the Linux packages of a Companion Viewer build: .deb, .rpm and Arch
# .pkg.tar.zst, with nfpm (on PATH, or NFPM=/path/to/nfpm).
#
#   .github/companion/package-linux.sh <packaged dir> <nightly|release> <version> <out dir>
#
# <packaged dir> is build-linux-x86_64/newview/packaged. Nightlies install as
# companion-viewer-nightly in /opt/companion-viewer-nightly, releases as
# companion-viewer in /opt/companion-viewer, so both can be installed at once.
# The packages carry the viewer's own version (7.4.0.83479) with package
# revision 1.
set -euo pipefail

packaged="${1:?usage: $0 <packaged dir> <nightly|release> <version> <out dir>}"
kind="${2:?}"
version="${3:?}"
out="${4:?}"
nfpm="${NFPM:-nfpm}"

case "$kind" in
    nightly)
        pkg=companion-viewer-nightly
        label="Companion Viewer Nightly"
        summary="Nightly build of the nightly branch, untested."
        ;;
    release)
        pkg=companion-viewer
        label="Companion Viewer"
        summary="Release build of the release branch."
        ;;
    *) echo "unknown kind '$kind'" >&2; exit 2 ;;
esac
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "odd version '$version'" >&2; exit 1; }
[ -x "$packaged/companionviewer" ] || { echo "no companionviewer start script in $packaged" >&2; exit 1; }

# The window class the desktop entry must name: SDL3 builds set an app id, SDL2 builds use
# the start script's name.
if compgen -G "$packaged/lib/libSDL3*" >/dev/null; then
    wmclass=io.github.lunagloaming.CompanionViewer
else
    wmclass=do-not-directly-run-companionviewer-bin
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
stage="$work/stage"
mkdir -p "$stage/opt/$pkg" "$out"

# 1. A stripped copy of the build (as Linux release packaging does), without FMOD.
rsync -rptDLK --exclude='bin/.debug' "$packaged/" "$stage/opt/$pkg/"
if [ -n "$(find "$stage" -iname 'libfmod*' -print -quit)" ]; then
    echo "the FMOD library is in the build" >&2
    exit 1
fi
for f in dullahan_host chrome-sandbox SLVoice; do
    [ -e "$stage/opt/$pkg/bin/$f" ] && chmod 755 "$stage/opt/$pkg/bin/$f"
done
find "$stage/opt/$pkg/bin" "$stage/opt/$pkg/lib" -type f | while read -r f; do
    if [ "$(head -c4 "$f" | od -An -c | tr -d ' ')" = '177ELF' ]; then
        strip -S "$f"
    fi
done
# Global Accountant's Chromium sandbox helper must be setuid root where unprivileged user
# namespaces are restricted (Ubuntu 24.04), as Electron's own packages install it.
sandbox=""
if [ -e "$stage/opt/$pkg/slmoney/chrome-sandbox" ]; then
    sandbox="$stage/slmoney-chrome-sandbox"
    mv "$stage/opt/$pkg/slmoney/chrome-sandbox" "$sandbox"
fi
echo "staged: $(du -sh "$stage" | cut -f1)"

cat > "$stage/$pkg.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$label
Comment=Client for accessing 3D virtual worlds
Exec=/opt/$pkg/companionviewer
Path=/opt/$pkg
Icon=$pkg
Terminal=false
Categories=Network;
StartupNotify=true
StartupWMClass=$wmclass
EOF
cp "$stage/opt/$pkg/companionviewer_icon.png" "$stage/$pkg.png"

# 2. The packages.
deps_deb="libc6 (>= 2.38), libstdc++6 (>= 13), libgcc-s1, libnss3, libnspr4, libcups2t64 | libcups2, libgbm1,
  libatk1.0-0t64 | libatk1.0-0, libatk-bridge2.0-0t64 | libatk-bridge2.0-0, libatspi2.0-0t64 | libatspi2.0-0,
  libxcomposite1, libxdamage1, libxfixes3, libxrandr2, libxkbcommon0, libpango-1.0-0, libcairo2,
  libasound2t64 | libasound2, libdbus-1-3, libexpat1, libudev1, libglib2.0-0t64 | libglib2.0-0, libfontconfig1,
  libcrypt1, libgl1, libx11-6, libxcb1, libxext6, zlib1g, libuuid1"
deps_rpm="libc.so.6(GLIBC_2.38)(64bit), libstdc++.so.6(GLIBCXX_3.4.32)(64bit), libnss3.so()(64bit), libnspr4.so()(64bit),
  libcups.so.2()(64bit), libgbm.so.1()(64bit), libatk-1.0.so.0()(64bit), libatk-bridge-2.0.so.0()(64bit),
  libatspi.so.0()(64bit), libXcomposite.so.1()(64bit), libXdamage.so.1()(64bit), libXfixes.so.3()(64bit),
  libXrandr.so.2()(64bit), libxkbcommon.so.0()(64bit), libpango-1.0.so.0()(64bit), libcairo.so.2()(64bit),
  libasound.so.2()(64bit), libdbus-1.so.3()(64bit), libexpat.so.1()(64bit), libudev.so.1()(64bit),
  libglib-2.0.so.0()(64bit), libfontconfig.so.1()(64bit), libcrypt.so.1()(64bit), libGL.so.1()(64bit),
  libX11.so.6()(64bit), libxcb.so.1()(64bit), libXext.so.6()(64bit), libz.so.1()(64bit), libuuid.so.1()(64bit)"
deps_arch="glibc, gcc-libs, nss, libcups, mesa, at-spi2-core, libxcomposite, libxdamage, libxfixes, libxrandr,
  libxkbcommon, pango, cairo, alsa-lib, dbus, expat, systemd-libs, glib2, fontconfig, libxcrypt-compat,
  libglvnd, libx11, libxcb, libxext, zlib, util-linux-libs"
yaml_list() { tr ',' '\n' | sed 's/^[[:space:]]*//; /^$/d; s/.*/      - "&"/'; }
sandbox_entry=""
if [ -n "$sandbox" ]; then
    sandbox_entry="  - src: $sandbox
    dst: /opt/$pkg/slmoney/chrome-sandbox
    file_info:
      mode: 04755
      owner: root
      group: root"
fi

cat > "$work/nfpm.yaml" <<EOF
name: $pkg
arch: amd64
platform: linux
version: $version
version_schema: none
release: 1
section: net
priority: optional
maintainer: lunagloaming <lunagloaming@users.noreply.github.com>
vendor: Companion Viewer
homepage: https://github.com/lunagloaming/companion-viewer
license: LGPL-2.1-only
description: |-
  $label: a Second Life viewer based on Firestorm.
  $summary
contents:
  - src: $stage/opt/$pkg
    dst: /opt/$pkg
    type: tree
  - src: /opt/$pkg/companionviewer
    dst: /usr/bin/$pkg
    type: symlink
  - src: $stage/$pkg.desktop
    dst: /usr/share/applications/$pkg.desktop
  - src: $stage/$pkg.png
    dst: /usr/share/icons/hicolor/512x512/apps/$pkg.png
$sandbox_entry
deb:
  compression: xz
rpm:
  compression: zstd
overrides:
  deb:
    depends:
$(echo "$deps_deb" | tr '\n' ' ' | yaml_list)
    recommends:
      - libvlc5
  rpm:
    depends:
$(echo "$deps_rpm" | tr '\n' ' ' | yaml_list)
  archlinux:
    depends:
$(echo "$deps_arch" | tr '\n' ' ' | yaml_list)
archlinux:
  packager: lunagloaming <lunagloaming@users.noreply.github.com>
EOF

for p in deb rpm archlinux; do
    "$nfpm" package -f "$work/nfpm.yaml" -p "$p" -t "$out/"
done
ls -la "$out"
