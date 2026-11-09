#!/usr/bin/env python3
"""Prepare maptool --ocean-polygons from full-resolution global OSM land.

Requires Python 3, pyshp and Shapely >= 2.0 (conversion host only).
Input: land-polygons-split-3857 from https://osmdata.openstreetmap.de/.
Use the same pinned source snapshot for adjoining maps. Regional coastlines
cannot establish which side of missing ways is ocean; this tool never guesses.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import sys

import shapefile
from shapely import affinity, get_num_coordinates, set_precision
from shapely.geometry import box, shape
from shapely.ops import unary_union

# Stable Navit binfile ABI; maptool validates both types on import.
WATER = 0xc0000065
HOLE = 0x4000d
RADIUS = 6371000
SCALE = RADIUS / 6378137


def projected(lon, lat):
    return math.radians(lon) * RADIUS, math.log(math.tan(math.pi / 4 + math.radians(lat) / 2)) * RADIUS


def polygons(geometry):
    if geometry.is_empty:
        return
    if geometry.geom_type == 'Polygon':
        yield geometry
    elif geometry.geom_type in ('MultiPolygon', 'GeometryCollection'):
        for part in geometry.geoms:
            yield from polygons(part)
    elif geometry.area:
        raise ValueError('Non-polygonal water geometry')


def split_polygon(polygon):
    """Bound native item size without simplifying any coastline vertices."""
    if get_num_coordinates(polygon) <= 8192:
        yield polygon
        return
    x0, y0, x1, y1 = polygon.bounds
    if x1 - x0 >= y1 - y0:
        middle = round((x0 + x1) / 2)
        halves = [box(x0, y0, middle, y1), box(middle, y0, x1, y1)]
    else:
        middle = round((y0 + y1) / 2)
        halves = [box(x0, y0, x1, middle), box(x0, middle, x1, y1)]
    for half in halves:
        for part in polygons(polygon.intersection(half)):
            yield from split_polygon(part)


def pack_polygon(polygon):
    if not polygon.is_valid:
        raise ValueError('Invalid water polygon after clipping/quantization')
    outer = [(int(x), int(y)) for x, y in polygon.exterior.coords]
    attrs = bytearray()
    for hole in polygon.interiors:
        coords = [(int(x), int(y)) for x, y in hole.coords]
        attrs.extend(struct.pack('=III', 2 + 2 * len(coords), HOLE, len(coords)))
        attrs.extend(b''.join(struct.pack('=ii', *p) for p in coords))
    raw = b''.join(struct.pack('=ii', *p) for p in outer)
    return struct.pack('=III', 2 + (len(raw) + len(attrs)) // 4, WATER, len(outer) * 2) + raw + attrs


def sha256(path):
    with Path(path).open('rb') as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
        return digest.hexdigest()


def prepare(land_file, bounds, output):
    west, south, east, north = bounds
    if not (-180 <= west < east <= 180 and -85 <= south < north <= 85):
        raise ValueError('Bounds must be west,south,east,north within Mercator coverage; split at the dateline')
    land_file, output = Path(land_file).resolve(), Path(output).resolve()
    projection = land_file.with_suffix('.prj').read_text()
    if 'Mercator' not in projection or '6378137' not in projection:
        raise ValueError('Expected the EPSG:3857 global land dataset')
    source_bounds = [v / SCALE for p in (projected(west, south), projected(east, north)) for v in p]
    domain = set_precision(box(*projected(west, south), *projected(east, north)), 1)
    local_land = []
    with shapefile.Reader(str(land_file)) as reader:
        # pyshp skips nonintersecting records by their stored bounds, keeping
        # memory proportional to the requested map instead of the planet.
        for record in reader.iterShapes(bbox=source_bounds):
            land = shape(record.__geo_interface__)
            if not land.is_valid:
                raise ValueError('Invalid source land polygon; use a validated global snapshot')
            land = affinity.scale(land, SCALE, SCALE, origin=(0, 0))
            local_land.append(land.intersection(domain))
    water = set_precision(domain.difference(unary_union(local_land)), 1)
    if not water.is_valid:
        raise ValueError('Invalid water coverage')
    output.parent.mkdir(parents=True, exist_ok=True)
    pending = output.with_name(output.name + '.partial')
    count = 0
    with pending.open('wb') as stream:
        for polygon in polygons(water):
            for part in split_polygon(polygon):
                stream.write(pack_polygon(part))
                count += 1
    os.replace(pending, output)
    metadata = {
        'source': str(land_file), 'source_sha256': sha256(land_file),
        'source_url': 'https://osmdata.openstreetmap.de/download/land-polygons-split-3857.zip',
        'source_readme': land_file.with_name('README.txt').read_text(),
        'copyright': 'OpenStreetMap contributors, ODbL; https://www.openstreetmap.org/copyright',
        'bounds': list(bounds), 'projection': 'Navit spherical Mercator, R=6371000m',
        'integer_grid_m': 1, 'simplification': False, 'byte_order': sys.byteorder,
        'land_records': len(local_land), 'water_polygons': count,
        'output_sha256': sha256(output),
        'generator_sha256': sha256(__file__),
    }
    output.with_suffix(output.suffix + '.json').write_text(json.dumps(metadata, indent=2) + '\n')
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--land-shapefile', type=Path, required=True)
    parser.add_argument('--bounds', required=True, help='west,south,east,north, from the extract header')
    parser.add_argument('output', type=Path, help='native item stream for maptool --ocean-polygons')
    args = parser.parse_args()
    try:
        bounds = tuple(map(float, args.bounds.split(',')))
        if len(bounds) != 4:
            raise ValueError('Four bounds are required')
        print(json.dumps(prepare(args.land_shapefile, bounds, args.output), indent=2))
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
