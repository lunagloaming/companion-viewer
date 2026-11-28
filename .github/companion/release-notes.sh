#!/bin/bash
# Writes the release notes of a Companion Viewer build to stdout.
#
#   .github/companion/release-notes.sh <nightly|release> <version> <commit> <dist dir> [previous tag]
#
# A platform whose file is missing from <dist dir> failed to build; the notes say so.
# The notes open with the changes since the previous tag of the same kind: our
# own commits one by one, Firestorm's as a count. Upstream's commits are told
# apart by the refs under refs/remotes/upstream/, which the caller fetches.
# Without a previous tag, the changes are all of ours since Firestorm's version.
set -euo pipefail

kind="${1:?usage: $0 <nightly|release> <version> <commit> <dist dir> [previous tag]}"
version="${2:?}"
commit="${3:?}"
dist="${4:?}"
prev="${5:-}"
short="${version%.*}"
date="$(date -u '+%Y-%m-%d')"

case "$kind" in
    nightly) pkg=companion-viewer-nightly; label="Companion Viewer Nightly"; branch=CVNIGHTLY ;;
    release) pkg=companion-viewer; label="Companion Viewer"; branch=CVRELEASE ;;
    *) echo "unknown kind '$kind'" >&2; exit 2 ;;
esac
file() { find "$dist" -maxdepth 1 -name "$1" -printf '%f\n' | sort | head -1; }
deb="$(file '*.deb')"; rpm="$(file '*.rpm')"; arch="$(file '*.pkg.tar.zst')"
dmg="$(file '*.dmg')"; exe="$(file '*-Setup.exe')"

if [ "$kind" = nightly ]; then
    echo "Nightly build of Companion Viewer, a Second Life viewer based on Firestorm, from \`$branch\` at $commit ($date). Nightlies follow Firestorm's development branch and are untested: expect bugs."
else
    echo "Companion Viewer $version, a Second Life viewer based on Firestorm $short, from \`$branch\` at $commit ($date)."
fi
echo

if [ -n "$prev" ]; then
    echo "### Changes since $prev"
    range="$prev..$commit"
else
    echo "### Changes from Firestorm $short"
    range="$commit"
fi
ours="$(git log --no-merges --format='- %s' "$range" --not --remotes=upstream)"
if [ -n "$ours" ]; then echo "$ours"; else echo "- No changes of Companion Viewer's own."; fi
if [ -n "$prev" ]; then
    all="$(git rev-list --count --no-merges "$range")"
    own="$(git rev-list --count --no-merges "$range" --not --remotes=upstream)"
    upstream=$(( all - own ))
    [ "$upstream" -gt 0 ] && echo "- $upstream changes from Firestorm's development branch."
fi
echo

missing_note() { echo "The $1 build is missing from this release: it failed to build."; }

echo "### Linux (x86_64)"
if [ -n "$deb" ]; then
    echo "Needs glibc 2.38 or newer: Ubuntu 24.04+, Debian 13+, Fedora 39+, or current Arch."
    echo "- Debian, Ubuntu: \`sudo apt install ./$deb\`"
    echo "- Fedora, openSUSE: \`sudo dnf install ./$rpm\`"
    echo "- Arch: \`sudo pacman -U $arch\`"
    echo
    echo "It installs to \`/opt/$pkg\`. Start **$label** from the menu, or run \`$pkg\`."
else
    missing_note Linux
fi
echo

echo "### macOS (12 or newer)"
if [ -n "$dmg" ]; then
    echo "Open \`$dmg\` and drag the app to Applications. The app isn't signed with an Apple Developer ID, so macOS refuses to open it the first time. Open **System Settings → Privacy & Security** and click **Open Anyway**."
else
    missing_note macOS
fi
echo

echo "### Windows (10 or newer, 64-bit)"
if [ -n "$exe" ]; then
    echo "Run \`$exe\`. The installer isn't code-signed, so Windows SmartScreen may warn about an unrecognized app: click **More info**, then **Run anyway**."
else
    missing_note Windows
fi
echo

echo "### Notes"
echo "- Sound uses OpenAL; FMOD isn't included."
echo "- Help > Global Accountant opens the Global Accountant app, which comes with the viewer: every account's L\$ balance, income and expenses, read from the viewer's money logs."
echo "- Settings and logs are kept in \`~/.companionviewer\` (Linux), \`~/Library/Application Support/CompanionViewer\` (macOS) and \`%APPDATA%\\CompanionViewer\` (Windows)."
echo "- The source for this build is the tag's commit. \`SHA256SUMS\` lists the files' checksums."

