#!/bin/bash
# Prints the version, tag and branch of a Companion Viewer build:
#
#   .github/companion/version.sh <nightly|release> [commit]
#   7.4.0.83479 Companion_Nightly_7.4.0.83479 nightly
#
# The version is indra/newview/VIEWER_VERSION_FS.txt plus the commit count, as
# indra/cmake/BuildVersion.cmake computes it, so a tag's name is known before
# the build and can be checked against it afterwards. Tags follow Firestorm's
# (Firestorm_Release_7.2.5.81788): Companion_Nightly_<version> for nightlies
# from nightly, Companion_Release_<version> for releases from
# release.
#
# Used by .github/workflows/companion-build.yml and by the maintainer's own
# tag script, so both name tags the same way. Needs the whole history.
set -euo pipefail

kind="${1:-}"
commit="${2:-HEAD}"
case "$kind" in
    nightly) prefix=Companion_Nightly_; branch=nightly ;;
    release) prefix=Companion_Release_; branch=release ;;
    *) echo "usage: $0 <nightly|release> [commit]" >&2; exit 2 ;;
esac

short="$(git show "$commit:indra/newview/VIEWER_VERSION_FS.txt" | tr -d '[:space:]')"
if ! [[ "$short" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "odd version '$short' in indra/newview/VIEWER_VERSION_FS.txt" >&2
    exit 1
fi
if [ "$(git rev-parse --is-shallow-repository)" = true ]; then
    echo "the build number is the commit count: fetch the whole history" >&2
    exit 1
fi
build="$(git rev-list --count "$commit")"

echo "$short.$build $prefix$short.$build $branch"
