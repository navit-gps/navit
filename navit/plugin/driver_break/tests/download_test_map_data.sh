#!/bin/bash
# Download OSM map data for test routes
# Test routes cover Norway: 60.8-61.3°N, 7.2-11.0°E
# Uses reliable sources: osmtoday.com (primary) and Geofabrik (fallback)
# NOTE: Overpass API is NOT used (unreliable, frequent timeouts)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAP_DATA_DIR="${SCRIPT_DIR}/map_data"
MAP_NAME="osm_bbox_7.2,60.8,11.0,61.3"

echo "Downloading OSM map data for test routes..."
echo "Bounding box: 7.2°E, 60.8°N to 11.0°E, 61.3°N (Norway)"
echo "Using reliable sources: osmtoday.com (primary) and Geofabrik (fallback)"
echo "NOTE: Overpass API is NOT used (unreliable, frequent timeouts)"

# Create map data directory
mkdir -p "${MAP_DATA_DIR}"

OSM_FILE="${MAP_DATA_DIR}/${MAP_NAME}.osm"
OSM_PBF="${MAP_DATA_DIR}/norway-latest.osm.pbf"
BIN_FILE="${MAP_DATA_DIR}/${MAP_NAME}.bin"

# Method 1: Try osmtoday.com for small bounding box extract (fastest, most reliable for test routes)
echo ""
echo "Method 1: Downloading from osmtoday.com (reliable source for small extracts)..."
OSM_TODAY_URL="https://osmtoday.com/api/download?bbox=7.2,60.8,11.0,61.3"

OSM_INPUT=""
if command -v curl &> /dev/null; then
    echo "Downloading from osmtoday.com..."
    if curl -L -f -o "${OSM_FILE}" "${OSM_TODAY_URL}" 2>&1 | tail -3; then
        if [ -f "${OSM_FILE}" ] && [ -s "${OSM_FILE}" ]; then
            # Check if it's PBF or XML
            if file "${OSM_FILE}" | grep -q "Protocol Buffer"; then
                echo "Downloaded PBF from osmtoday.com: $(du -h "${OSM_FILE}" | cut -f1)"
                OSM_INPUT="${OSM_FILE}"
            elif head -1 "${OSM_FILE}" | grep -q "<?xml" && grep -q "<osm" "${OSM_FILE}"; then
                echo "Downloaded OSM XML from osmtoday.com: $(du -h "${OSM_FILE}" | cut -f1)"
                OSM_INPUT="${OSM_FILE}"
            else
                echo "Downloaded file is not valid OSM data"
                rm -f "${OSM_FILE}"
                OSM_INPUT=""
            fi
        else
            OSM_INPUT=""
        fi
    else
        echo "osmtoday.com download failed, trying Geofabrik..."
        OSM_INPUT=""
    fi
elif command -v wget &> /dev/null; then
    echo "Downloading from osmtoday.com..."
    if wget -O "${OSM_FILE}" "${OSM_TODAY_URL}" 2>&1 | tail -3; then
        if [ -f "${OSM_FILE}" ] && [ -s "${OSM_FILE}" ]; then
            if file "${OSM_FILE}" | grep -q "Protocol Buffer"; then
                echo "Downloaded PBF from osmtoday.com: $(du -h "${OSM_FILE}" | cut -f1)"
                OSM_INPUT="${OSM_FILE}"
            elif head -1 "${OSM_FILE}" | grep -q "<?xml" && grep -q "<osm" "${OSM_FILE}"; then
                echo "Downloaded OSM XML from osmtoday.com: $(du -h "${OSM_FILE}" | cut -f1)"
                OSM_INPUT="${OSM_FILE}"
            else
                OSM_INPUT=""
            fi
        else
            OSM_INPUT=""
        fi
    else
        echo "osmtoday.com download failed, trying Geofabrik..."
        OSM_INPUT=""
    fi
else
    echo "Error: Neither curl nor wget found. Cannot download map data."
    exit 1
fi

# Method 2: Fallback to Geofabrik Norway extract if osmtoday failed
if [ -z "${OSM_INPUT}" ]; then
    echo ""
    echo "Method 2: Downloading Norway extract from Geofabrik (fallback)..."
    GEOFABRIK_URL="https://download.geofabrik.de/europe/norway-latest.osm.pbf"

    if command -v curl &> /dev/null; then
        echo "Downloading ${GEOFABRIK_URL}..."
        echo "Note: Full Norway extract is large (~1.2GB). This may take several minutes..."
        if curl -L -f -o "${OSM_PBF}" "${GEOFABRIK_URL}" 2>&1 | tail -3; then
            if [ -f "${OSM_PBF}" ] && [ -s "${OSM_PBF}" ]; then
                echo "Downloaded Norway extract: $(du -h "${OSM_PBF}" | cut -f1)"
                echo "Extracting test region (7.2°E, 60.8°N to 11.0°E, 61.3°N)..."
                EXTRACT_NEEDED=1
                OSM_INPUT="${OSM_PBF}"
            else
                EXTRACT_NEEDED=0
                OSM_INPUT=""
            fi
        else
            EXTRACT_NEEDED=0
            OSM_INPUT=""
        fi
    elif command -v wget &> /dev/null; then
        echo "Downloading ${GEOFABRIK_URL}..."
        echo "Note: Full Norway extract is large (~1.2GB). This may take several minutes..."
        if wget -O "${OSM_PBF}" "${GEOFABRIK_URL}" 2>&1 | tail -3; then
            if [ -f "${OSM_PBF}" ] && [ -s "${OSM_PBF}" ]; then
                echo "Downloaded Norway extract: $(du -h "${OSM_PBF}" | cut -f1)"
                EXTRACT_NEEDED=1
                OSM_INPUT="${OSM_PBF}"
            else
                EXTRACT_NEEDED=0
                OSM_INPUT=""
            fi
        else
            EXTRACT_NEEDED=0
            OSM_INPUT=""
        fi
    fi
else
    EXTRACT_NEEDED=0
fi

# Extract bounding box from PBF file if we downloaded full Norway extract
if [ "${EXTRACT_NEEDED}" = "1" ] && [ -f "${OSM_PBF}" ]; then
    echo "Extracting bounding box (7.2°E, 60.8°N to 11.0°E, 61.3°N) from Norway extract..."

    # Check if osmium-tool is available (best tool for PBF extraction)
    if command -v osmium &> /dev/null; then
        echo "Using osmium-tool to extract bounding box..."
        osmium extract -b 7.2,60.8,11.0,61.3 "${OSM_PBF}" -o "${OSM_FILE}.pbf" 2>&1 | tail -5
        if [ -f "${OSM_FILE}.pbf" ] && [ -s "${OSM_FILE}.pbf" ]; then
            echo "Extracted region: $(du -h "${OSM_FILE}.pbf" | cut -f1)"
            OSM_INPUT="${OSM_FILE}.pbf"
        else
            echo "Warning: Extraction failed, using full Norway extract"
            OSM_INPUT="${OSM_PBF}"
        fi
    elif command -v osmosis &> /dev/null; then
        echo "Using Osmosis to extract bounding box..."
        osmosis --read-pbf "${OSM_PBF}" --bounding-box left=7.2 bottom=60.8 right=11.0 top=61.3 --write-xml "${OSM_FILE}" 2>&1 | tail -5
        if [ -f "${OSM_FILE}" ] && [ -s "${OSM_FILE}" ]; then
            echo "Extracted region: $(du -h "${OSM_FILE}" | cut -f1)"
            OSM_INPUT="${OSM_FILE}"
        else
            echo "Warning: Extraction failed"
            OSM_INPUT="${OSM_PBF}"  # Fallback to full extract
        fi
    else
        echo "Warning: Neither osmium-tool nor osmosis found."
        echo "Cannot extract bounding box. Using full Norway extract (will be large)."
        echo "Install osmium-tool: sudo apt-get install osmium-tool"
        echo "Or install osmosis: sudo apt-get install osmosis"
        OSM_INPUT="${OSM_PBF}"
    fi
fi

# Verify we have valid OSM data
if [ -n "${OSM_INPUT}" ] && [ -f "${OSM_INPUT}" ] && [ -s "${OSM_INPUT}" ]; then
    # Check if it's PBF or XML
    if file "${OSM_INPUT}" | grep -q "Protocol Buffer"; then
        echo "PBF file detected: ${OSM_INPUT}"
        FINAL_OSM="${OSM_INPUT}"
    elif head -1 "${OSM_INPUT}" | grep -q "<?xml" && grep -q "<osm" "${OSM_INPUT}"; then
        echo "Valid OSM XML data: ${OSM_INPUT}"
        FINAL_OSM="${OSM_INPUT}"
    else
        echo "Error: Downloaded file is not valid OSM data"
        echo "File type: $(file "${OSM_INPUT}")"
        exit 1
    fi
    echo "OSM data ready: $(du -h "${FINAL_OSM}" | cut -f1)"
else
    echo "Error: Failed to download OSM data from any source"
    echo "Tried: osmtoday.com and Geofabrik"
    exit 1
fi

# Convert OSM data to Navit binary format
echo ""
echo "Converting OSM data to Navit binary format..."

# Check if maptool is available
MAPTOOL=""
if [ -f "${SCRIPT_DIR}/../../../../build/maptool/maptool" ]; then
    MAPTOOL="${SCRIPT_DIR}/../../../../build/maptool/maptool"
elif command -v maptool &> /dev/null; then
    MAPTOOL="maptool"
else
    echo "Warning: maptool not found. OSM data downloaded but not converted to Navit format."
    echo "To convert, run:"
    if file "${FINAL_OSM}" | grep -q "Protocol Buffer"; then
        echo "  ${MAPTOOL} -i ${FINAL_OSM} ${BIN_FILE}"
    else
        echo "  grep -v '^[[:space:]]*$' ${FINAL_OSM} | ${MAPTOOL} ${BIN_FILE}"
    fi
    exit 0
fi

# Convert using maptool
echo "Running maptool conversion..."
if file "${FINAL_OSM}" | grep -q "Protocol Buffer"; then
    # PBF file - use -i flag
    echo "Converting PBF file to Navit binary format..."
    "${MAPTOOL}" -i "${FINAL_OSM}" "${BIN_FILE}" 2>&1 | tail -10
else
    # OSM XML file - pipe through maptool
    echo "Converting OSM XML to Navit binary format..."
    # Pre-process: remove empty lines (maptool requirement)
    grep -v "^[[:space:]]*$" "${FINAL_OSM}" | "${MAPTOOL}" "${BIN_FILE}" 2>&1 | tail -10
fi

if [ -f "${BIN_FILE}" ] && [ -s "${BIN_FILE}" ]; then
    echo ""
    echo "Converted to Navit binary format: $(du -h "${BIN_FILE}" | cut -f1)"
    echo "Map data ready: ${BIN_FILE}"
    echo ""
    echo "Map data download and conversion complete!"
    echo "Map file: ${BIN_FILE}"
    echo "Source: osmtoday.com or Geofabrik (reliable sources, NOT Overpass API)"
else
    echo "Warning: Conversion may have failed. Check ${BIN_FILE}"
    echo "Input file: ${FINAL_OSM} ($(du -h "${FINAL_OSM}" | cut -f1))"
    echo ""
    echo "Note: If conversion failed, you may need to:"
    echo "1. Install osmium-tool for PBF extraction: sudo apt-get install osmium-tool"
    echo "2. Or use a smaller extract from osmtoday.com"
fi
