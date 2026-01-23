# APRS Symbols

This directory contains APRS symbol icons from the hessu/aprs-symbols repository.

## Symbol Files

The symbols are organized as:
- `48x48/primary/` - Primary symbol table icons (94 individual PNG files)
- `48x48/alternate/` - Alternate symbol table icons (94 individual PNG files)

## Source

Symbols are from: https://github.com/hessu/aprs-symbols
Created by OH7LZB (aprs.fi author)

The hessu/aprs-symbols repository provides sprite sheets (large PNG files containing multiple symbols). The symbols in this directory are extracted from those sprite sheets using the `extract_symbols.py` script.

The extraction script uses the official symbol definitions from `aprs_symbols.yaml`, which contains the complete symbol table with descriptions and metadata from the hessu/aprs-symbols project. This ensures accurate symbol extraction and proper handling of unused symbols.

## Known Issues

**Symbol Mapping Discrepancies**: The sprite sheet from hessu/aprs-symbols may have some images in positions that don't match the APRS symbol code expectations used by the C code lookup table in `aprs_symbols.c`. The extraction script attempts to handle these mismatches by swapping sprite positions for certain symbol codes (e.g., `/F` vs `/f`, `/E` vs `/e`, `/U` vs `/u`), but some symbols may still be mapped incorrectly. If you notice incorrect symbol displays in Navit, the sprite sheet images may need to be manually verified against the expected symbol codes.

## License

The symbols are free to use for any APRS application.

## Initial Setup

The symbols are included in the codebase. If you need to update them:

```bash
cd navit/plugin/aprs/symbols
./update_symbols.sh
```

This script will:
1. Clone the hessu/aprs-symbols repository
2. Extract individual symbols from the sprite sheets using `extract_symbols.py`
3. Organize them into `48x48/primary/` and `48x48/alternate/` directories
4. Clean up the temporary repository

## Requirements

The extraction script requires Python3 with PIL (Pillow) and PyYAML:
```bash
pip3 install Pillow PyYAML
```

## Build Integration

The CMake build system automatically detects symbols in `48x48/primary/` and `48x48/alternate/` and installs them to `${CMAKE_INSTALL_PREFIX}/share/navit/aprs-symbols/48x48/` during the build process.

