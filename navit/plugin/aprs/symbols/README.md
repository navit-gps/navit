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

The extraction script requires Python3 with PIL (Pillow):
```bash
pip3 install Pillow
```

## Build Integration

The CMake build system automatically detects symbols in `48x48/primary/` and `48x48/alternate/` and installs them to `${CMAKE_INSTALL_PREFIX}/share/navit/aprs-symbols/48x48/` during the build process.

