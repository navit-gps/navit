#!/usr/bin/env bash
# Local code quality analysis script for Navit
# Similar to CodeFactor but runs locally using cppcheck and clang-tidy

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="${PROJECT_ROOT}/code_quality_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

mkdir -p "$OUTPUT_DIR"

echo "=========================================="
echo "Navit Code Quality Analysis (Local)"
echo "=========================================="
echo ""

# Check if tools are available
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${YELLOW}Warning: $1 is not installed. Skipping $1 analysis.${NC}"
        echo "  Install with: sudo apt-get install $1"
        return 1
    fi
    return 0
}

# Run cppcheck analysis
run_cppcheck() {
    if ! check_tool cppcheck; then
        return 1
    fi

    echo -e "${GREEN}[1/3] Running cppcheck static analysis...${NC}"

    local target="${1:-navit/plugin/driver_break}"
    local report_file="${OUTPUT_DIR}/cppcheck_${TIMESTAMP}.txt"

    cppcheck \
        --enable=all \
        --suppress=missingIncludeSystem \
        --suppress=unusedFunction \
        --suppress=unmatchedSuppression \
        --inline-suppr \
        --xml --xml-version=2 \
        --output-file="${OUTPUT_DIR}/cppcheck_${TIMESTAMP}.xml" \
        "$target" 2>&1 | tee "$report_file"

    local issues=$(grep -c "\[.*\]" "$report_file" 2>/dev/null || echo "0")
    echo -e "${GREEN}  Found $issues potential issues${NC}"
    echo "  Report saved to: $report_file"
    echo ""
}

# Run clang-tidy analysis (if available)
run_clang_tidy() {
    if ! check_tool clang-tidy; then
        return 1
    fi

    echo -e "${GREEN}[2/5] Running clang-tidy analysis...${NC}"

    local target="${1:-navit/plugin/driver_break}"
    local report_file="${OUTPUT_DIR}/clang-tidy_${TIMESTAMP}.txt"

    # Create compile_commands.json if it doesn't exist
    if [ ! -f "$PROJECT_ROOT/compile_commands.json" ]; then
        echo -e "${YELLOW}  Warning: compile_commands.json not found.${NC}"
        echo "  Generate it with: cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .."
        echo "  Running clang-tidy without compile commands (limited analysis)..."
    fi

    local issues=0
    find "$target" -name "*.c" -o -name "*.h" | while read file; do
        if [ -f "$PROJECT_ROOT/compile_commands.json" ]; then
            clang-tidy "$file" \
                -checks='-*,readability-*,performance-*,portability-*,misc-*' \
                2>&1 | tee -a "$report_file"
        else
            clang-tidy "$file" \
                -checks='-*,readability-*,performance-*,portability-*,misc-*' \
                --warnings-as-errors='*' 2>&1 | tee -a "$report_file"
        fi
    done

    issues=$(grep -c "warning:\|error:" "$report_file" 2>/dev/null || echo "0")
    echo -e "${GREEN}  Found $issues potential issues${NC}"
    echo "  Report saved to: $report_file"
    echo ""
}

# Run flawfinder analysis (security-focused)
run_flawfinder() {
    if ! check_tool flawfinder; then
        return 1
    fi

    echo -e "${GREEN}[3/5] Running flawfinder security analysis...${NC}"

    local target="${1:-navit/plugin/driver_break}"
    local report_file="${OUTPUT_DIR}/flawfinder_${TIMESTAMP}.txt"

    flawfinder --html --context --columns --minlevel=1 "$target" > "${OUTPUT_DIR}/flawfinder_${TIMESTAMP}.html" 2>&1
    flawfinder --context --columns --minlevel=1 "$target" > "$report_file" 2>&1

    local issues=$(grep -c "Hits = " "$report_file" 2>/dev/null | tail -1 | grep -oP '\d+' || echo "0")
    echo -e "${GREEN}  Found $issues potential security issues${NC}"
    echo "  Report saved to: $report_file"
    echo "  HTML report: ${OUTPUT_DIR}/flawfinder_${TIMESTAMP}.html"
    echo ""
}

# Run splint analysis (if available)
run_splint() {
    if ! check_tool splint; then
        return 1
    fi

    echo -e "${GREEN}[4/5] Running splint analysis...${NC}"

    local target="${1:-navit/plugin/driver_break}"
    local report_file="${OUTPUT_DIR}/splint_${TIMESTAMP}.txt"

    # Splint needs include paths - try to find them from CMake or use defaults
    local include_dirs="-I${PROJECT_ROOT}/navit -I${PROJECT_ROOT}"

    find "$target" -name "*.c" | while read file; do
        splint $include_dirs -weak -preproc -warnposix -nolib -DHAVE_API_ANDROID=0 -DHAVE_CURL=1 "$file" 2>&1 | tee -a "$report_file" || true
    done

    local issues=$(grep -c "Splint" "$report_file" 2>/dev/null || echo "0")
    echo -e "${GREEN}  Found $issues potential issues${NC}"
    echo "  Report saved to: $report_file"
    echo ""
}

# Run code complexity analysis
run_complexity() {
    echo -e "${GREEN}[5/5] Analyzing code complexity...${NC}"

    local target="${1:-navit/plugin/driver_break}"
    local report_file="${OUTPUT_DIR}/complexity_${TIMESTAMP}.txt"

    echo "Code Complexity Analysis" > "$report_file"
    echo "=======================" >> "$report_file"
    echo "" >> "$report_file"

    echo "Largest files:" >> "$report_file"
    find "$target" -name "*.c" -exec wc -l {} \; | sort -rn | head -10 >> "$report_file"
    echo "" >> "$report_file"

    echo "Functions with high complexity (potential issues):" >> "$report_file"
    echo "  (Functions with many nested levels or long parameter lists)" >> "$report_file"
    echo "" >> "$report_file"

    # Count function parameters (functions with >5 parameters are complex)
    grep -n "^[a-zA-Z_].*(" "$target"/*.c 2>/dev/null | \
        awk -F: '{print $2}' | \
        grep -o "([^)]*)" | \
        awk -F, '{if (NF > 5) print}' | \
        head -10 >> "$report_file" || true

    echo "  Report saved to: $report_file"
    echo ""
}

# Main execution
TARGET="${1:-navit/plugin/driver_break}"

if [ ! -d "$TARGET" ]; then
    echo -e "${RED}Error: Directory $TARGET does not exist${NC}"
    exit 1
fi

echo "Analyzing: $TARGET"
echo "Output directory: $OUTPUT_DIR"
echo ""

run_cppcheck "$TARGET"
run_clang_tidy "$TARGET"
run_flawfinder "$TARGET"
run_splint "$TARGET"
run_complexity "$TARGET"

echo "=========================================="
echo -e "${GREEN}Analysis complete!${NC}"
echo "Reports saved in: $OUTPUT_DIR"
echo "=========================================="
