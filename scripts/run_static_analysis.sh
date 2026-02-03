#!/usr/bin/env bash
# Run static analysis tools on Navit codebase.
# Tools: cppcheck, clang-tidy, flawfinder, scan-build (clang), gcc -fanalyzer
set -e

SRCDIR="$(cd "${1:-.}" && pwd)"
BUILDDIR="${2:-build-analyze}"

cd "$SRCDIR"

echo "=== Static analysis for Navit ==="
echo "Source: $SRCDIR"
echo "Build dir: $BUILDDIR"
echo ""

# Generate compile_commands.json if needed
if [ ! -f "$BUILDDIR/compile_commands.json" ]; then
    echo "[1/5] Configuring CMake (compile_commands.json)..."
    mkdir -p "$BUILDDIR"
    cmake -B "$BUILDDIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

# 1. cppcheck
echo ""
echo "[2/5] cppcheck (general static analyzer)..."
if command -v cppcheck >/dev/null 2>&1; then
    cppcheck --project="$BUILDDIR/compile_commands.json" \
        --enable=warning,style,performance,portability \
        --suppress=missingIncludeSystem \
        -q 2>&1 | head -100 || true
else
    echo "  cppcheck not found. Install: apt install cppcheck"
fi

# 2. clang-tidy
echo ""
echo "[3/5] clang-tidy (comprehensive linter)..."
if command -v clang-tidy >/dev/null 2>&1; then
    find navit/plugin -name "*.c" 2>/dev/null | head -5 | while read f; do
        clang-tidy "$f" -p "$BUILDDIR" 2>/dev/null | grep -E "warning:|error:" | head -3
    done || true
else
    echo "  clang-tidy not found. Install: apt install clang-tidy"
fi

# 3. flawfinder
echo ""
echo "[4/5] flawfinder (security vulnerability scanner)..."
if command -v flawfinder >/dev/null 2>&1; then
    flawfinder --minlevel=3 navit/plugin/ 2>&1 | tail -40
else
    echo "  flawfinder not found. Install: apt install flawfinder"
fi

# 4. scan-build, gcc -fanalyzer
echo ""
echo "[5/5] scan-build / gcc -fanalyzer..."
echo "  scan-build: apt install clang-tools && scan-build -o scan-build-out cmake --build $BUILDDIR"
echo "  gcc -fanalyzer: cmake -B $BUILDDIR -DENABLE_ANALYZER=ON && cmake --build $BUILDDIR"
echo ""
echo "=== Done ==="
