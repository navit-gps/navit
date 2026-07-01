#!/bin/bash
# Update APRS symbols from hessu/aprs-symbols repository

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Updating APRS symbols from hessu/aprs-symbols repository..."

# Remove old clone if exists
if [ -d "aprs-symbols" ]; then
    rm -rf aprs-symbols
fi

# Clone repository (shallow clone for speed)
echo "Cloning repository..."
git clone --depth 1 https://github.com/hessu/aprs-symbols.git

# Extract individual symbols from sprite sheets
if [ -f "aprs-symbols/png/aprs-symbols-48-0.png" ] && [ -f "aprs-symbols/png/aprs-symbols-48-1.png" ]; then
    echo "Extracting symbols from sprite sheets..."

    # Check if Python3, PIL, and PyYAML are available
    if ! python3 -c "from PIL import Image" 2>/dev/null; then
        echo "Error: Python3 with PIL (Pillow) is required"
        echo "Install with: pip3 install Pillow"
        exit 1
    fi
    if ! python3 -c "import yaml" 2>/dev/null; then
        echo "Error: Python3 with PyYAML is required"
        echo "Install with: pip3 install PyYAML"
        exit 1
    fi

    # Run extraction script
    python3 extract_symbols.py

    if [ -d "48x48/primary" ] && [ -d "48x48/alternate" ]; then
        PRIMARY_COUNT=$(find 48x48/primary -name "*.png" 2>/dev/null | wc -l)
        ALTERNATE_COUNT=$(find 48x48/alternate -name "*.png" 2>/dev/null | wc -l)
        echo "Symbols extracted successfully"
        echo "Primary symbols: $PRIMARY_COUNT files"
        echo "Alternate symbols: $ALTERNATE_COUNT files"
    else
        echo "Error: Symbol extraction failed"
        exit 1
    fi
else
    echo "Error: Sprite sheets not found in repository"
    exit 1
fi

# Clean up
rm -rf aprs-symbols


