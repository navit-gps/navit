#!/bin/bash
# Install APRS symbols from hessu/aprs-symbols repository
# This script downloads and installs the symbol PNG files for the Navit APRS plugin

set -e

SYMBOL_SIZE="48x48"
INSTALL_DIR="/usr/share/navit/aprs-symbols"
TEMP_DIR=$(mktemp -d)

echo "APRS Symbols Installation Script"
echo "================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Error: This script must be run as root (use sudo)"
    exit 1
fi

# Create install directory
echo "Creating install directory: $INSTALL_DIR/$SYMBOL_SIZE"
mkdir -p "$INSTALL_DIR/$SYMBOL_SIZE/primary"
mkdir -p "$INSTALL_DIR/$SYMBOL_SIZE/alternate"

# Download repository
echo "Downloading hessu/aprs-symbols repository..."
cd "$TEMP_DIR"
if [ -d "aprs-symbols" ]; then
    echo "Repository already exists, updating..."
    cd aprs-symbols
    git pull
else
    git clone https://github.com/hessu/aprs-symbols.git
    cd aprs-symbols
fi

# Check if symbol directory exists
if [ ! -d "$SYMBOL_SIZE/primary" ]; then
    echo "Error: Symbol directory $SYMBOL_SIZE/primary not found in repository"
    echo "Available directories:"
    ls -d ./*/ | head -10
    exit 1
fi

# Copy primary symbols
echo "Installing primary symbols..."
cp -v "$SYMBOL_SIZE/primary"/*.png "$INSTALL_DIR/$SYMBOL_SIZE/primary/" 2>/dev/null || {
    echo "Warning: Some primary symbols may not have been copied"
}

# Copy alternate symbols if they exist
if [ -d "$SYMBOL_SIZE/alternate" ]; then
    echo "Installing alternate symbols..."
    cp -v "$SYMBOL_SIZE/alternate"/*.png "$INSTALL_DIR/$SYMBOL_SIZE/alternate/" 2>/dev/null || {
        echo "Warning: Some alternate symbols may not have been copied"
    }
fi

# Set permissions
echo "Setting permissions..."
chmod -R 644 "$INSTALL_DIR/$SYMBOL_SIZE"/*/*.png
chown -R root:root "$INSTALL_DIR"

# Count installed files
PRIMARY_COUNT=$(find "$INSTALL_DIR/$SYMBOL_SIZE/primary" -name "*.png" | wc -l)
ALTERNATE_COUNT=$(find "$INSTALL_DIR/$SYMBOL_SIZE/alternate" -name "*.png" 2>/dev/null | wc -l || echo "0")

echo ""
echo "Installation complete!"
echo "Primary symbols installed: $PRIMARY_COUNT"
echo "Alternate symbols installed: $ALTERNATE_COUNT"
echo ""
echo "Symbol files are now available at: $INSTALL_DIR/$SYMBOL_SIZE"
echo ""
echo "To verify installation, check:"
echo "  ls $INSTALL_DIR/$SYMBOL_SIZE/primary/vehicle.png"

# Cleanup
rm -rf "$TEMP_DIR"

echo ""
echo "Done!"

