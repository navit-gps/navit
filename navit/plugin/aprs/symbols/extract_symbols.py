#!/usr/bin/env python3
"""
Extract individual APRS symbols from hessu/aprs-symbols sprite sheets.

The sprite sheets are organized as:
- 16 columns x 6 rows for 48x48 symbols
- Each symbol is 48x48 pixels
- Primary table: aprs-symbols-48-0.png
- Alternate table: aprs-symbols-48-1.png
"""

import os
import sys
import yaml
from PIL import Image

def load_symbol_definitions(yaml_path):
    """Load symbol definitions from YAML file."""
    with open(yaml_path, 'r') as f:
        data = yaml.safe_load(f)
    return data.get('symbols', [])

def get_expected_filename(symbol_code):
    """
    Get the expected filename for a symbol code based on aprs_symbols.c lookup table.

    Args:
        symbol_code (str): Symbol code (e.g., '/!', '/>', '\\\\!', etc.)

    Returns:
        str: Expected filename matching the C code lookup table
    """
    # Mapping from symbol code to expected filename (matches aprs_symbols.c)
    SYMBOL_FILENAME_MAP = {
        # Primary table symbols
        '/!': 'digipeater.png',
        '/"': 'mailbox.png',
        '/#': 'phone.png',
        '/$': 'dx_cluster.png',
        '/%': 'hf_gateway.png',
        '/&': 'small_circle.png',
        "/'": 'mob_station.png',
        '/(': 'repeater.png',
        '/)': 'repeater.png',
        '/*': 'aircraft.png',
        '/+': 'snowmobile.png',
        '/,': 'red_x.png',
        '/-': 'house.png',
        '/.': 'circle.png',
        '//': 'file_server.png',
        '/0': 'fire.png',
        '/1': 'campground.png',
        '/2': 'motorcycle.png',
        '/3': 'rail_engine.png',
        '/4': 'car.png',
        '/5': 'file_server.png',
        '/6': 'cloudy.png',
        '/7': 'hail.png',
        '/8': 'rain.png',
        '/9': 'snow.png',
        '/:': 'tornado.png',
        '/;': 'hurricane.png',
        '/<': 'tropical_storm.png',
        '/=': 'hurricane.png',
        '/>': 'vehicle.png',
        '/?': 'unknown.png',
        '/@': 'digipeater.png',
        '/A': 'aid_station.png',
        '/B': 'bike.png',
        '/C': 'canoe.png',
        '/D': 'fire_dept.png',
        '/E': 'horse.png',
        '/F': 'fire_truck.png',
        '/G': 'glider.png',
        '/H': 'hospital.png',
        '/I': 'iota.png',
        '/J': 'jeep.png',
        '/K': 'school.png',
        '/L': 'user.png',
        '/M': 'macap.png',
        '/N': 'nws_station.png',
        '/O': 'balloon.png',
        '/P': 'police.png',
        '/Q': 'truck.png',
        '/R': 'repeater.png',
        '/S': 'boat.png',
        '/T': 'truck_stop.png',
        '/U': 'truck.png',
        '/V': 'van.png',
        '/W': 'water_station.png',
        '/X': 'x.png',
        '/Y': 'yacht.png',
        '/Z': 'truck.png',
        '/[': 'picnic_table.png',
        '/\\': 'car.png',
        '/]': 'wall.png',
        '/^': 'aircraft.png',
        '/_': 'weather_station.png',
        '/`': 'house.png',
        '/a': 'ambulance.png',
        '/b': 'bike.png',
        '/c': 'car.png',
        '/d': 'fire_dept.png',
        '/e': 'eye.png',
        '/f': 'farm.png',
        '/g': 'grid_square.png',
        '/h': 'hotel.png',
        '/i': 'tcpip.png',
        '/j': 'triangle.png',
        '/k': 'school.png',
        '/l': 'lighthouse.png',
        '/m': 'mara.png',
        '/n': 'nav_beacon.png',
        '/o': 'balloon.png',
        '/p': 'parking.png',
        '/q': 'truck.png',
        '/r': 'repeater.png',
        '/s': 'satellite.png',
        '/t': 'sstv.png',
        '/u': 'bus.png',
        '/v': 'van.png',
        '/w': 'water_station.png',
        '/x': 'x.png',
        '/y': 'yacht.png',
        '/z': 'z.png',
        '/{': 'shelter.png',
        '/|': 'truck.png',
        '/}': 'truck.png',
        '/~': 'grid_square.png',
        # Alternate table symbols
        '\\!': 'digipeater.png',
        '\\"': 'mailbox.png',
        '\\#': 'phone.png',
        '\\$': 'dx_cluster.png',
        '\\%': 'hf_gateway.png',
        '\\&': 'small_circle.png',
        "\\'": 'mob_station.png',
        '\\(': 'repeater.png',
        '\\)': 'repeater.png',
        '\\*': 'aircraft.png',
        '\\+': 'snowmobile.png',
        '\\,': 'red_x.png',
        '\\-': 'house.png',
        '\\.': 'circle.png',
        '\\/': 'file_server.png',
        '\\0': 'circle.png',  # Circle, IRLP / Echolink/WIRES
        '\\8': 'tcpip.png',  # 802.11 WiFi or other network node
        '\\9': 'truck_stop.png',  # Gas station
        '\\:': 'hail.png',
        '\\;': 'picnic_table.png',  # Park, picnic area
        '\\<': 'unknown.png',  # Advisory, single red flag
        '\\=': 'unknown.png',
        '\\>': 'vehicle.png',
        '\\?': 'unknown.png',  # Info kiosk
        '\\@': 'hurricane.png',  # Hurricane, Tropical storm
        '\\A': 'unknown.png',  # White box
        '\\B': 'snow.png',  # Blowing snow
        '\\C': 'aid_station.png',  # Coast Guard
        '\\D': 'rain.png',  # Drizzling rain
        '\\E': 'unknown.png',  # Smoke, Chimney
        '\\F': 'rain.png',  # Freezing rain
        '\\G': 'snow.png',  # Snow shower
        '\\H': 'cloudy.png',  # Haze
        '\\I': 'rain.png',  # Rain shower
        '\\J': 'unknown.png',  # Lightning
        '\\K': 'unknown.png',  # Kenwood HT
        '\\L': 'lighthouse.png',
        '\\N': 'nav_beacon.png',  # Navigation buoy
        '\\O': 'satellite.png',  # Rocket
        '\\P': 'parking.png',
        '\\Q': 'unknown.png',  # Earthquake
        '\\R': 'unknown.png',  # Restaurant
        '\\S': 'satellite.png',
        '\\T': 'tornado.png',  # Thunderstorm
        '\\U': 'cloudy.png',  # Sunny (using cloudy as placeholder)
        '\\V': 'nav_beacon.png',  # VORTAC, Navigational aid
        '\\W': 'nws_station.png',  # NWS site
        '\\X': 'unknown.png',  # Pharmacy
        '\\[': 'cloudy.png',  # Wall Cloud
        '\\\\': 'unknown.png',  # Backslash (unused in C code)
        '\\]': 'unknown.png',  # Unused
        '\\^': 'aircraft.png',
        '\\_': 'weather_station.png',  # Weather site
        '\\`': 'rain.png',
        '\\a': 'unknown.png',  # Red diamond
        '\\b': 'unknown.png',  # Blowing dust, sand
        '\\c': 'triangle.png',  # CD triangle, RACES, CERTS, SATERN
        '\\d': 'dx_cluster.png',  # DX spot
        '\\e': 'rain.png',  # Sleet
        '\\f': 'tornado.png',  # Funnel cloud
        '\\g': 'hurricane.png',  # Gale, two red flags
        '\\h': 'unknown.png',  # Store
        '\\i': 'unknown.png',  # Black box, point of interest
        '\\j': 'unknown.png',  # Work zone, excavating machine
        '\\k': 'truck.png',  # SUV, ATV
        '\\l': 'unknown.png',  # Unused
        '\\m': 'unknown.png',  # Value sign, 3 digit display
        '\\n': 'triangle.png',  # Red triangle
        '\\o': 'circle.png',  # Small circle
        '\\p': 'cloudy.png',  # Partly cloudy
        '\\q': 'unknown.png',  # Unused
        '\\r': 'unknown.png',  # Restrooms
        '\\s': 'boat.png',  # Ship, boat
        '\\t': 'tornado.png',
        '\\u': 'truck.png',
        '\\v': 'van.png',
        '\\w': 'unknown.png',  # Flooding
        '\\x': 'unknown.png',  # Unused
        '\\y': 'unknown.png',  # Skywarn
        '\\z': 'shelter.png',
        '\\{': 'cloudy.png',  # Fog
        '\\}': 'unknown.png',  # Unused
    }

    return SYMBOL_FILENAME_MAP.get(symbol_code, 'unknown.png')

def generate_filename(symbol):
    """
    Generate filename for the symbol matching the C code lookup table.

    The sprite sheet images match YAML descriptions, but the C code expects
    different filenames for some symbol codes. We map based on what the
    sprite sheet actually contains (YAML description) to the C code filename.

    Args:
        symbol (dict): Symbol definition from YAML

    Returns:
        str: Filename matching the C code lookup table
    """
    code = symbol['code']
    description = symbol.get('description', '').lower()

    # Special cases where YAML description doesn't match C code expectations
    # These are handled by using get_expected_filename() which maps symbol codes
    # to C code filenames, regardless of what the sprite sheet actually contains
    description_to_filename = {
        # YAML says "Horse, equestrian" at /e, but C code expects eye.png
        'horse, equestrian': 'eye.png',
        'horse': 'eye.png',  # for /e
        # YAML says "Eyeball" at /E, but C code expects horse.png
        'eyeball': 'horse.png',  # for /E
        # YAML says "Fire" at /:, but C code expects tornado.png
        'fire': 'tornado.png',  # for /:
        # YAML says "Bus" at /U, but C code expects truck.png
        'bus': 'truck.png',  # for /U
        # YAML says "Semi-trailer truck, 18-wheeler" at /u, but C code expects bus.png
        'semi-trailer truck, 18-wheeler': 'bus.png',
        'semi-trailer truck': 'bus.png',
    }

    # Check if we have a special mapping for this description
    if description in description_to_filename:
        return description_to_filename[description]

    # Check partial matches
    for desc_key, filename in description_to_filename.items():
        if desc_key in description:
            return filename

    # Otherwise use the standard code-based mapping
    filename = get_expected_filename(code)

    # If not found in mapping, fall back to description-based generation
    if filename == 'unknown.png' and description:
        clean_desc = description \
            .replace(' ', '_') \
            .replace('/', '_') \
            .replace(',', '') \
            .replace('(', '') \
            .replace(')', '') \
            .replace(':', '') \
            .replace('-', '_') \
            .replace("'", '') \
            .replace('.', '') \
            .replace('&', 'and')

        clean_desc = clean_desc \
            .replace('_station', '') \
            .replace('_vehicle', '') \
            .replace('_site', '') \
            .replace('_by_', 'x')

        if not clean_desc.endswith('.png'):
            clean_desc += '.png'
        filename = clean_desc

    return filename

def extract_symbols(sprite_path, output_dir, symbols, table_type):
    """Extract individual symbols from a sprite sheet."""
    if not os.path.exists(sprite_path):
        print(f"Error: Sprite sheet not found: {sprite_path}")
        return False

    try:
        img = Image.open(sprite_path)
    except Exception as e:
        print(f"Error opening sprite sheet: {e}")
        return False

    width, height = img.size
    symbol_size = 48
    cols = 16
    rows = height // symbol_size

    print(f"Extracting from {sprite_path} ({width}x{height})")
    print(f"Symbol size: {symbol_size}x{symbol_size}, Grid: {cols}x{rows}")

    os.makedirs(output_dir, exist_ok=True)

    # Filter symbols for the current table
    # Use YAML order (not ASCII order) because the sprite sheet matches YAML order
    # The YAML descriptions indicate what image is actually at each position
    all_table_symbols = [s for s in symbols if s['code'].startswith(table_type)]

    # Map symbol codes to their sprite sheet indices
    code_to_index = {}
    for idx, sym in enumerate(all_table_symbols):
        code_to_index[sym['code']] = idx

    # Swaps: when C code expects filename X for symbol Y, but sprite sheet has wrong image at Y,
    # we extract from a different position that has the correct image
    sprite_position_swaps = {
        # C expects /F -> fire_truck.png, but sprite at /F has farm, so extract from /f (has fire_truck)
        '/F': '/f',
        # C expects /f -> farm.png, but sprite at /f has fire_truck, so extract from /F (has farm)
        '/f': '/F',
        # C expects /E -> horse.png, but sprite at /E has eyeball, so extract from /e (has horse)
        '/E': '/e',
        # C expects /e -> eye.png, but sprite at /e has horse, so extract from /E (has eyeball)
        '/e': '/E',
        # C expects /U -> truck.png, but sprite at /U has bus, so extract from /u (has truck)
        '/U': '/u',
        # C expects /u -> bus.png, but sprite at /u has truck, so extract from /U (has bus)
        '/u': '/U',
    }

    extracted = 0
    skipped = 0

    # Extract symbols in YAML order
    for idx, symbol in enumerate(all_table_symbols):
        if idx >= cols * rows:
            break

        # Skip unused symbols
        if symbol.get('unused', 0):
            skipped += 1
            continue

        code = symbol['code']

        # Get the sprite position to extract from (might be swapped)
        extract_from_code = sprite_position_swaps.get(code, code)
        extract_from_idx = code_to_index.get(extract_from_code, idx)

        row = extract_from_idx // cols
        col = extract_from_idx % cols

        x = col * symbol_size
        y = row * symbol_size

        # Extract symbol from the (possibly swapped) position
        symbol_img = img.crop((x, y, x + symbol_size, y + symbol_size))

        # Get the filename the C code expects for this symbol code
        filename = get_expected_filename(code)
        output_path = os.path.join(output_dir, filename)

        symbol_img.save(output_path, 'PNG')
        if extract_from_code != code:
            print(f"Saved: {filename} (extracted from {extract_from_code} position for {code})")
        else:
            print(f"Saved: {filename}")
        extracted += 1

    print(f"Extracted {extracted} symbols to {output_dir}")
    if skipped > 0:
        print(f"Skipped {skipped} unused symbols")
    return True

def main():
    # Determine script and repo directories
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_dir = os.path.join(script_dir, 'aprs-symbols', 'png')

    # Path to YAML symbol definitions
    yaml_path = os.path.join(script_dir, 'aprs_symbols.yaml')

    # Load symbol definitions
    try:
        symbols = load_symbol_definitions(yaml_path)
        print(f"Loaded {len(symbols)} symbol definitions from {yaml_path}")
    except Exception as e:
        print(f"Error loading symbol definitions: {e}")
        sys.exit(1)

    # Sprite sheet paths
    primary_sprite = os.path.join(repo_dir, 'aprs-symbols-48-0.png')
    primary_output = os.path.join(script_dir, '48x48', 'primary')

    alternate_sprite = os.path.join(repo_dir, 'aprs-symbols-48-1.png')
    alternate_output = os.path.join(script_dir, '48x48', 'alternate')

    success = True

    # Extract primary table symbols
    if os.path.exists(primary_sprite):
        success = extract_symbols(primary_sprite, primary_output, symbols, '/') and success
    else:
        print(f"Warning: Primary sprite not found: {primary_sprite}")
        success = False

    # Extract alternate table symbols
    if os.path.exists(alternate_sprite):
        success = extract_symbols(alternate_sprite, alternate_output, symbols, '\\') and success
    else:
        print(f"Warning: Alternate sprite not found: {alternate_sprite}")
        success = False

    if success:
        print("\nSymbol extraction complete!")
        print(f"Primary symbols: {primary_output}")
        print(f"Alternate symbols: {alternate_output}")
    else:
        print("\nSymbol extraction failed!")
        sys.exit(1)

if __name__ == '__main__':
    main()
