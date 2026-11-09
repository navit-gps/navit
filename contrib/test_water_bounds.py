"""Valid geometry outside the legacy tile grid must remain queryable.

Usage: python test_water_bounds.py SOURCE MAPTOOL OUTPUT_DIRECTORY
Checks the actual submap index, not just polygons anywhere in the ZIP archive.
"""
import copy
import json
from pathlib import Path
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET
import zipfile

from test_inland_water import enum_value, geo, is_water, polygons


def selected_tiles(source, path, point, order):
    submap = enum_value(source / 'navit/item_def.h', 'ITEM', 'submap')
    attrs = {name: enum_value(source / 'navit/attr_def.h', 'ATTR', name)
             for name in ('order', 'zipfile_ref')}
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        pending, visited = ['index'], set()
        while pending:
            name = pending.pop()
            assert name not in visited, 'Cyclic or repeated submap reference'
            visited.add(name)
            raw = archive.read(name)
            pos = 0
            while pos < len(raw):
                length, kind, clen = struct.unpack_from('<III', raw, pos)
                end = pos + 4 * (length + 1)
                if kind == submap:
                    assert clen == 4
                    x0, y0, x1, y1 = struct.unpack_from('<iiii', raw, pos + 12)
                    low, high, child = 0, 255, None
                    attr = pos + 12 + 4 * clen
                    while attr < end:
                        alen, atype = struct.unpack_from('<II', raw, attr)
                        if atype == attrs['order']:
                            low, high = struct.unpack_from('<hh', raw, attr + 8)
                        if atype == attrs['zipfile_ref']:
                            child, = struct.unpack_from('<I', raw, attr + 8)
                        attr += 4 * (alen + 1)
                    if (min(x0, x1) <= point[0] <= max(x0, x1)
                            and min(y0, y1) <= point[1] <= max(y0, y1)
                            and low <= order <= high):
                        assert child is not None and child < len(names)
                        pending.append(names[child])
                pos = end
        return visited


def fixture(tags, axis, sign, large):
    root = ET.Element('osm', version='0.6')
    nodes, ways = [], []

    def point(d, t):
        xy = (sign * (20000000 + d), 1000000 + t)
        return xy if axis == 'x' else xy[::-1]

    def ring(corners):
        ids = []
        for d, t in corners:
            nid = len(nodes) + 1
            x, y = point(d, t)
            nodes.append((nid, *geo(x + .25, y + .25)))
            ids.append(nid)
        wid = 1000 + len(ways)
        ways.append((wid, ids + ids[:1]))
        return wid

    lo, span = (-1000000, 3000000) if large else (2000, 2000)
    outer = ring([(lo, -span), (14000, -span), (14000, span), (lo, span)])
    hole = ring([(4000, -1000), (10000, -1000), (10000, 1000), (4000, 1000)])
    town = len(nodes) + 1
    for d in (12000, 11000, 13000):
        x, y = point(d, 0)
        nodes.append((len(nodes) + 1, *geo(x + .25, y + .25)))
    for nid, lon, lat in nodes:
        node = ET.SubElement(root, 'node', id=str(nid), version='1', lon=f'{lon:.12f}', lat=f'{lat:.12f}')
        if nid == town:
            ET.SubElement(node, 'tag', k='place', v='village')
            ET.SubElement(node, 'tag', k='name', v='Outside grid town')
    for wid, ids in ways:
        way = ET.SubElement(root, 'way', id=str(wid), version='1')
        for nid in ids:
            ET.SubElement(way, 'nd', ref=str(nid))
    road = ET.SubElement(root, 'way', id='1100', version='1')
    for nid in (town + 1, town + 2):
        ET.SubElement(road, 'nd', ref=str(nid))
    ET.SubElement(road, 'tag', k='highway', v='residential')
    ET.SubElement(road, 'tag', k='name', v='Outside grid road')
    relation = ET.SubElement(root, 'relation', id='2000', version='1')
    for wid, role in [(outer, 'outer'), (hole, 'inner')]:
        ET.SubElement(relation, 'member', type='way', ref=str(wid), role=role)
    for k, v in [('type', 'multipolygon'), *tags]:
        ET.SubElement(relation, 'tag', k=k, v=v)
    if not large:
        # Keep the edge tile above the 64-KiB merge threshold. Otherwise these
        # tiny maps collapse to the root and cannot expose an incorrect index.
        for i in range(128):
            extra = copy.deepcopy(relation)
            extra.set('id', str(3000 + i))
            ET.SubElement(extra, 'tag', k='name', v=f'Index filler {i} ' + 'x' * 800)
            root.append(extra)
    ET.indent(root)
    checks = [(point(12000, 0), True), (point(7000, 0), False),
              (point(15000, 0), False), (point(lo + 1000, 1500), True)]
    return ET.ElementTree(root), checks


if __name__ == '__main__':
    source, binary, output = (Path(p).resolve() for p in sys.argv[1:4])
    output.mkdir(parents=True, exist_ok=False)
    kinds = [('lake', [('natural', 'water')], 'poly_water'),
             ('river', [('waterway', 'riverbank')], 'poly_water'),
             ('reservoir', [('landuse', 'reservoir')], 'poly_reservoir'),
             ('basin', [('landuse', 'basin')], 'poly_basin')]
    results = []
    for kind, tags, item_type in kinds:
        for axis in ('x', 'y'):
            for sign in (-1, 1):
                for large in (False, True):
                    name = f'{kind}-{axis}{sign}-large{int(large)}'
                    work = output / name
                    work.mkdir()
                    tree, checks = fixture(tags, axis, sign, large)
                    tree.write(work / 'input.osm', encoding='utf-8', xml_declaration=True)
                    with (work / 'conversion.log').open('w') as log:
                        subprocess.run([str(binary),
                                        '-i', str(work / 'input.osm'), str(work / 'map.bin')],
                                       cwd=work, stdout=log, stderr=log, check=True)
                    failures = []
                    for point, expected in checks:
                        for order in ([18] if expected else range(19)):
                            selected = selected_tiles(source, work / 'map.bin', point, order)
                            actual = is_water(polygons(source, work / 'map.bin', item_type, selected), point)
                            if actual != expected:
                                failures.append(dict(point=point, order=order, expected=expected, actual=actual))
                    selected = selected_tiles(source, work / 'map.bin', checks[0][0], 18)
                    if not large and len(selected) == 1:
                        failures.append(dict(error='Expected an indexed edge tile, not just the root'))
                    with zipfile.ZipFile(work / 'map.bin') as archive:
                        raw = b''.join(archive.read(n) for n in selected)
                    for label in (b'Outside grid town', b'Outside grid road'):
                        if label not in raw:
                            failures.append(dict(missing_indexed_feature=label.decode()))
                    results.append(dict(case=name, checks=checks, failures=failures))
    (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(dict(cases=len(results), failed=sum(bool(r['failures']) for r in results))))
    assert not any(r['failures'] for r in results), 'Geometry outside the legacy grid is lost; see results.json'
