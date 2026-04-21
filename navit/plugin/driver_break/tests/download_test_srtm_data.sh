#!/bin/bash
# Download elevation tiles for test_driver_break_srtm and test_driver_break_route_integration.
# Primary: Copernicus DEM GLO-30 (AWS S3). Fallback: Viewfinder dem3 (HGT zip), then NASA SRTMGL1.
# Rondanestien: N61E009, N61E010, N62E009. Legacy: N62E007.
# Output dir: /tmp/test_srtm_hgt_download (same as used by the tests).
#
# Copernicus URL: https://copernicus-dem-30m.s3.amazonaws.com/Copernicus_DSM_COG_10_{LAT}_{LON}_DEM/Copernicus_DSM_COG_10_{LAT}_{LON}_DEM.tif
# Viewfinder dem3: http://www.viewfinderpanoramas.org/dem3/{ZONE}/{TILENAME}.zip (e.g. M32/N61E009.zip)
# Viewfinder blocks scripted downloads without a browser User-Agent; tile index: dem3list.txt

set -e

SRTM_DIR="${SRTM_DIR:-/tmp/test_srtm_hgt_download}"
COPERNICUS_BASE="https://copernicus-dem-30m.s3.amazonaws.com/"
VIEWFINDER_BASE="http://www.viewfinderpanoramas.org/dem3/"
NASA_BASE="https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/2000.02.11/"
# Browser User-Agent required by Viewfinder to avoid "permission denied" on scripted downloads
CURL_USERAGENT="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"

mkdir -p "${SRTM_DIR}"
cd "${SRTM_DIR}"

# Return Viewfinder dem3 zone (e.g. M32) for given lat, lon (integer degrees).
get_viewfinder_zone() {
    local lat=$1
    local lon=$2
    local zone=""
    if [ "$lat" -ge 60 ] && [ "$lat" -le 63 ] && [ "$lon" -ge 6 ] && [ "$lon" -le 12 ]; then
        zone="M32"
    fi
    echo "$zone"
}

# Check if file looks like a valid TIFF (magic 0x4949 or 0x4D4D).
is_valid_tiff() {
    [ ! -f "$1" ] || [ ! -s "$1" ] && return 1
    local magic
    magic=$(head -c 2 "$1" | od -An -tx1 | tr -d ' ')
    [ "$magic" = "4949" ] || [ "$magic" = "4d4d" ]
}

download_tile() {
    local lat=$1
    local lon=$2
    local lat_dir="N"
    local lon_dir="E"
    [ "$lat" -lt 0 ] && lat_dir="S" && lat=$(( -lat ))
    [ "$lon" -lt 0 ] && lon_dir="W" && lon=$(( -lon ))
    local name="${lat_dir}$(printf '%02d' "$lat")${lon_dir}$(printf '%03d' "$lon")"
    local hgt="${name}.hgt"
    local zip="${name}.zip"
    local copernicus_tile="Copernicus_DSM_COG_10_${lat_dir}$(printf '%02d' "$lat")_00_${lon_dir}$(printf '%03d' "$lon")_00_DEM"
    local tif="${copernicus_tile}.tif"
    local vf_zone
    local nasa_zip="${name}.SRTMGL1.hgt.zip"
    local nasa_hgt="${name}.SRTMGL1.hgt"
    local curl_silent="-s"
    [ -n "${VERBOSE}" ] && curl_silent=""

    [ -f "${hgt}" ] && [ -s "${hgt}" ] && echo "  ${hgt} already present" && return 0
    if is_valid_tiff "${tif}"; then
        echo "  ${tif} already present" && return 0
    fi

    if command -v curl >/dev/null 2>&1; then
        # Primary: Copernicus DEM GLO-30 (S3). URL: {base}/{tilename}/{tilename}.tif
        if curl -L -f ${curl_silent} -o "${tif}" --connect-timeout 30 --max-time 300 "${COPERNICUS_BASE}${copernicus_tile}/${tif}"; then
            if is_valid_tiff "${tif}"; then
                echo "  ${tif} from Copernicus" && return 0
            fi
            rm -f "${tif}"
        fi

        # Fallback: Viewfinder dem3 (requires browser User-Agent; tile index at viewfinderpanoramas.org/dem3list.txt)
        vf_zone=$(get_viewfinder_zone "$lat" "$lon")
        if [ -n "$vf_zone" ]; then
            if curl -L -f ${curl_silent} -o "${zip}" -A "${CURL_USERAGENT}" --connect-timeout 30 --max-time 300 "${VIEWFINDER_BASE}${vf_zone}/${name}.zip"; then
                if unzip -o -q "${zip}" "${hgt}" 2>/dev/null; then
                    rm -f "${zip}"
                    [ -f "${hgt}" ] && echo "  ${hgt} from Viewfinder (${vf_zone})" && return 0
                fi
                if unzip -o -q "${zip}" "${nasa_hgt}" 2>/dev/null && [ -f "${nasa_hgt}" ]; then
                    mv "${nasa_hgt}" "${hgt}"
                    rm -f "${zip}"
                    echo "  ${hgt} from Viewfinder (${vf_zone})" && return 0
                fi
                rm -f "${zip}" "${hgt}" "${nasa_hgt}" 2>/dev/null
            fi
        fi

        # Second fallback: NASA SRTMGL1
        if curl -L -f ${curl_silent} -o "${zip}" --connect-timeout 30 --max-time 300 "${NASA_BASE}${nasa_zip}"; then
            if unzip -o -q "${zip}" "${hgt}" 2>/dev/null; then
                rm -f "${zip}"
                [ -f "${hgt}" ] && echo "  ${hgt} from NASA" && return 0
            fi
            if unzip -o -q "${zip}" "${nasa_hgt}" 2>/dev/null && [ -f "${nasa_hgt}" ]; then
                mv "${nasa_hgt}" "${hgt}"
                rm -f "${zip}"
                echo "  ${hgt} from NASA" && return 0
            fi
            rm -f "${zip}" "${hgt}" "${nasa_hgt}" 2>/dev/null
        fi
    fi
    echo "  ${name} download failed"
    echo "    Copernicus: curl -L -O ${COPERNICUS_BASE}${copernicus_tile}/${tif}"
    vf_zone=$(get_viewfinder_zone "$lat" "$lon")
    [ -n "$vf_zone" ] && echo "    Viewfinder: curl -L -O -A \"${CURL_USERAGENT}\" ${VIEWFINDER_BASE}${vf_zone}/${name}.zip"
    echo "    NASA: curl -L -O ${NASA_BASE}${nasa_zip}"
    return 1
}

echo "Downloading elevation tiles (Copernicus primary, Viewfinder/NASA fallback)..."
echo "Output directory: ${SRTM_DIR}"
download_tile 61 9 || true
download_tile 61 10 || true
download_tile 62 9 || true
download_tile 62 7 || true
echo "Done. Run test_driver_break_srtm and test_driver_break_route_integration to use this data."
