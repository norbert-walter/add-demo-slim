#!/usr/bin/env bash
# verify_reference.sh - run ALL native tests against the reference implementation
# (referenz_logic.zip, delivered separately, NOT part of the repository) without
# touching the working tree. Proves that the frozen tests are consistent and
# satisfiable before an agent implements src/logic/*.cpp.
#
#   scripts/verify_reference.sh /path/to/referenz_logic.zip   (or an unpacked folder)
#
# Uses a temporary git worktree of HEAD, copies the reference .cpp files into
# src/logic/ there and runs "pio test -e native". Needs PlatformIO and a
# Linux/macOS host (test_command_queue uses pthreads).
set -euo pipefail
REF="${1:?usage: $0 <referenz_logic.zip | folder>}"
ROOT="$(git rev-parse --show-toplevel)"
TMP="$(mktemp -d)"
cleanup() { git -C "$ROOT" worktree remove --force "$TMP/wt" >/dev/null 2>&1 || true; rm -rf "$TMP"; }
trap cleanup EXIT

git -C "$ROOT" worktree add --detach "$TMP/wt" HEAD >/dev/null
if [[ -f "$REF" ]]; then
    mkdir "$TMP/ref" && unzip -q "$REF" -d "$TMP/ref"
    SRC="$(dirname "$(find "$TMP/ref" -name app_logic.cpp | head -1)")"
else
    SRC="$REF"
fi
[[ -f "$SRC/app_logic.cpp" ]] || { echo "no reference .cpp files found in $REF"; exit 1; }
cp "$SRC"/*.cpp "$TMP/wt/src/logic/"
echo "reference files: $(ls "$SRC"/*.cpp | wc -l) -> $TMP/wt/src/logic/"
cd "$TMP/wt"
pio test -e native
