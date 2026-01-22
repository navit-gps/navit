# APRS Symbols Integration

## Overview

The APRS plugin integrates with the hessu/aprs-symbols symbol set, providing modern, high-quality icons for APRS stations on the map.

## Symbol Set

The plugin uses symbols from:
- **hessu/aprs-symbols**: https://github.com/hessu/aprs-symbols
- Created by OH7LZB (aprs.fi author)
- Compatible with Updated APRS Symbol Set (Rev H)

## Symbol Format

APRS symbols are defined by two characters:
- **Symbol Table**: Primary table `/` or alternate table `\`
- **Symbol Code**: Single character indicating the specific icon

Example: `/` + `>` = Vehicle symbol

## Icon Files

The plugin expects icon files in PNG format at 48x48 pixels, organized as:
```
/usr/share/navit/aprs-symbols/48x48/primary/
  vehicle.png
  car.png
  bike.png
  ...
```

## Installation

### Bundled with Plugin

The APRS symbols are integrated into the plugin and installed automatically during the Navit build process. No separate installation step is required.

### Build-Time Symbol Integration

To include symbols in the build:

1. Download the hessu/aprs-symbols repository:
```bash
cd navit/plugin/aprs
git clone https://github.com/hessu/aprs-symbols.git symbols
```

2. The symbols will be automatically installed during `make install` or when building Navit.

The build system will:
- Detect symbols in `navit/plugin/aprs/symbols/48x48/`
- Install them to `${CMAKE_INSTALL_PREFIX}/share/navit/aprs-symbols/48x48/`
- Make them available to the plugin at runtime

### Manual Symbol Installation (Optional)

If symbols are not included at build time, you can install them manually:
```bash
sudo ./navit/plugin/aprs/install_aprs_symbols.sh
```

## Configuration

The symbol lookup system is initialized automatically when the APRS plugin loads. The plugin automatically detects the symbol installation path using Navit's standard directories.

## Symbol Lookup

The plugin includes a built-in lookup table for common APRS symbols. The lookup function:
- Maps symbol_table + symbol_code to icon filename
- Returns full path to icon file
- Returns NULL if symbol not found

## Supported Symbols

The plugin supports symbols from:
- Primary symbol table (`/`): ~95 symbols
- Alternate symbol table (`\`): ~95 symbols

Common symbols include:
- `/` + `>` = Vehicle
- `/` + `-` = House
- `/` + `*` = Aircraft
- `/` + `S` = Boat
- `/` + `B` = Bike
- `/` + `H` = Hospital
- `/` + `P` = Police

See the APRS Protocol Specification for complete symbol tables.

## Layout Configuration

APRS stations use `type_poi_custom0` to support custom icons. Ensure your Navit layout includes:
```xml
<itemgra item_types="poi_custom0" order="10-">
    <icon src="%s"/>
</itemgra>
```

The `%s` format string is substituted with the icon path from `attr_icon_src`.

## Troubleshooting

### Icons Not Displaying

1. Verify icon files are installed:
```bash
ls /usr/share/navit/aprs-symbols/48x48/primary/vehicle.png
```

2. Check symbol lookup:
```c
char *icon = aprs_symbol_get_icon('/', '>');
printf("Icon path: %s\n", icon);
```

3. Verify layout configuration includes custom POI icon support

4. Check Navit debug logs for icon loading errors

### Missing Symbols

If a symbol is not found:
- Check if symbol_table and symbol_code are valid
- Verify the icon file exists in the expected directory
- The plugin falls back to default POI icon if symbol not found

## References

- hessu/aprs-symbols: https://github.com/hessu/aprs-symbols
- hessu/aprs-symbol-index: https://github.com/hessu/aprs-symbol-index
- APRS Protocol Specification: http://www.aprs.org
- aprs.fi: https://aprs.fi

