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
from PIL import Image

# Symbol code mapping (ASCII order)
SYMBOL_CODES = [
    '!', '"', '#', '$', '%', '&', "'", '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '[', '\\', ']', '^', '_', '`',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '{', '|', '}', '~'
]

# Symbol filename mapping (from aprs_symbols.c lookup table)
SYMBOL_FILENAMES = {
    '!': 'digipeater.png', '"': 'mailbox.png', '#': 'phone.png', '$': 'dx_cluster.png',
    '%': 'hf_gateway.png', '&': 'small_circle.png', "'": 'mob_station.png',
    '(': 'repeater.png', ')': 'repeater.png', '*': 'aircraft.png', '+': 'snowmobile.png',
    ',': 'red_x.png', '-': 'house.png', '.': 'circle.png', '/': 'file_server.png',
    '0': 'fire.png', '1': 'campground.png', '2': 'motorcycle.png', '3': 'rail_engine.png',
    '4': 'car.png', '5': 'file_server.png', '6': 'cloudy.png', '7': 'hail.png',
    '8': 'rain.png', '9': 'snow.png', ':': 'tornado.png', ';': 'hurricane.png',
    '<': 'tropical_storm.png', '=': 'hurricane.png', '>': 'vehicle.png', '?': 'unknown.png',
    '@': 'digipeater.png',
    'A': 'aid_station.png', 'B': 'bike.png', 'C': 'canoe.png', 'D': 'fire_dept.png',
    'E': 'horse.png', 'F': 'fire_truck.png', 'G': 'glider.png', 'H': 'hospital.png',
    'I': 'iota.png', 'J': 'jeep.png', 'K': 'school.png', 'L': 'user.png',
    'M': 'macap.png', 'N': 'nws_station.png', 'O': 'balloon.png', 'P': 'police.png',
    'Q': 'truck.png', 'R': 'repeater.png', 'S': 'boat.png', 'T': 'truck_stop.png',
    'U': 'truck.png', 'V': 'van.png', 'W': 'water_station.png', 'X': 'x.png',
    'Y': 'yacht.png', 'Z': 'truck.png',
    '[': 'picnic_table.png', '\\': 'car.png', ']': 'wall.png', '^': 'aircraft.png',
    '_': 'weather_station.png', '`': 'house.png',
    'a': 'ambulance.png', 'b': 'bike.png', 'c': 'car.png', 'd': 'fire_dept.png',
    'e': 'eye.png', 'f': 'farm.png', 'g': 'grid_square.png', 'h': 'hotel.png',
    'i': 'tcpip.png', 'j': 'triangle.png', 'k': 'school.png', 'l': 'lighthouse.png',
    'm': 'mara.png', 'n': 'nav_beacon.png', 'o': 'balloon.png', 'p': 'parking.png',
    'q': 'truck.png', 'r': 'repeater.png', 's': 'satellite.png', 't': 'sstv.png',
    'u': 'bus.png', 'v': 'van.png', 'w': 'water_station.png', 'x': 'x.png',
    'y': 'yacht.png', 'z': 'z.png',
    '{': 'shelter.png', '|': 'truck.png', '}': 'truck.png', '~': 'grid_square.png',
}

def extract_symbols(sprite_path, output_dir, symbol_codes):
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
    
    extracted = 0
    for idx, code in enumerate(symbol_codes):
        if idx >= cols * rows:
            break
        
        row = idx // cols
        col = idx % cols
        
        x = col * symbol_size
        y = row * symbol_size
        
        # Extract symbol
        symbol_img = img.crop((x, y, x + symbol_size, y + symbol_size))
        
        # Get filename from mapping
        filename = SYMBOL_FILENAMES.get(code, f'symbol_{ord(code)}.png')
        output_path = os.path.join(output_dir, filename)
        
        symbol_img.save(output_path, 'PNG')
        extracted += 1
    
    print(f"Extracted {extracted} symbols to {output_dir}")
    return True

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_dir = os.path.join(script_dir, 'aprs-symbols', 'png')
    
    # Primary table
    primary_sprite = os.path.join(repo_dir, 'aprs-symbols-48-0.png')
    primary_output = os.path.join(script_dir, '48x48', 'primary')
    
    # Alternate table
    alternate_sprite = os.path.join(repo_dir, 'aprs-symbols-48-1.png')
    alternate_output = os.path.join(script_dir, '48x48', 'alternate')
    
    success = True
    
    if os.path.exists(primary_sprite):
        success = extract_symbols(primary_sprite, primary_output, SYMBOL_CODES) and success
    else:
        print(f"Warning: Primary sprite not found: {primary_sprite}")
        success = False
    
    if os.path.exists(alternate_sprite):
        success = extract_symbols(alternate_sprite, alternate_output, SYMBOL_CODES) and success
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

