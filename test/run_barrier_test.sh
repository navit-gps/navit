#!/usr/bin/env bash
set -u

# Regression test for barrier routing (bollard, cycle barrier, lift gate).
#
# Generates small binfile maps from the OSM fixtures in test/barrier with the
# maptool, compiles a small routing harness against the freshly built
# navit_core, and asserts that restricted profiles cannot cross each barrier
# while unrestricted ones still can.
#
# Usage: test/run_barrier_test.sh [BUILD_DIR]
#   BUILD_DIR defaults to <repo>/build.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
BUILD="${1:-$ROOT/build}"

MAPTOOL="$BUILD/navit/maptool/maptool"
NAVIT_CORE="$BUILD/navit/libnavit_core.so"
PLUGIN="$(find "$BUILD/navit/map/binfile" -name 'libmap_binfile.*' 2>/dev/null | head -1)"

if [[ ! -x "$MAPTOOL" ]]; then
    echo "ERROR: maptool not found at $MAPTOOL" >&2
    echo "Build it first, e.g.: make -C $BUILD maptool" >&2
    exit 1
fi
if [[ ! -f "$NAVIT_CORE" ]]; then
    echo "ERROR: navit_core not found at $NAVIT_CORE" >&2
    echo "Build it first, e.g.: make -C $BUILD navit_core" >&2
    exit 1
fi
if [[ -z "$PLUGIN" ]]; then
    echo "ERROR: binfile plugin not found under $BUILD/navit/map/binfile" >&2
    exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

for osm in "$SCRIPT_DIR"/barrier/*.osm; do
    name="$(basename "${osm%.osm}")"
    "$MAPTOOL" -i "$osm" "$WORK/$name.bin" >/dev/null 2>&1 ||
        { echo "ERROR: maptool failed on $osm" >&2; exit 1; }
done

gcc -o "$WORK/routetest" "$SCRIPT_DIR/routetest_main.c" \
    $(pkg-config --cflags --libs glib-2.0) \
    -I"$ROOT" -I"$BUILD" -I"$ROOT/navit" \
    -L"$BUILD/navit" -lnavit_core -Wl,-rpath,"$BUILD/navit" ||
    { echo "ERROR: could not build test harness" >&2; exit 1; }

"$WORK/routetest" "$WORK/corridor.bin" "$WORK/detour.bin" "$WORK/cycle_barrier.bin" "$WORK/lift_gate.bin" \
    "$WORK/prev_node_flags.bin" "$PLUGIN"
rc=$?

if [[ $rc -eq 0 ]]; then
    echo "barrier routing test: PASS"
else
    echo "barrier routing test: FAIL" >&2
fi
exit $rc