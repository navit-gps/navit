"""Do not fill open water ways; retain them for multipolygon assembly.

Usage: python test_inland_water.py SOURCE MAPTOOL OUTPUT_DIRECTORY
Uses only the Python standard library and the existing binfile decoder.
"""
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

import math
import re
import struct
import zipfile

def geo(x, y):
    return math.degrees(x / 6371000), math.degrees(2 * math.atan(math.exp(y / 6371000)) - math.pi / 2)

def enum_value(path, macro, name):
    value = -1
    for line in path.read_text().splitlines():
        if line.strip() == macro + '_UNUSED':
            value += 1
        match = re.fullmatch(rf'{macro}(2)?\((?:([^,]+),\s*)?(\w+)\)', line.strip())
        if match:
            value = int(match[2], 0) if match[1] else value + 1
            if match[3] == name:
                return value
    raise RuntimeError(f'Missing {macro} {name}')


def polygons(source, binfile, item_type='poly_water_tiled', members=None):
    """Decode actual little-endian binfile items, including every hole attribute."""
    water_type = enum_value(source / 'navit/item_def.h', 'ITEM', item_type)
    hole_type = enum_value(source / 'navit/attr_def.h', 'ATTR', 'poly_hole')
    result = []
    with zipfile.ZipFile(binfile) as archive:
        for member in archive.infolist():
            if members is not None and member.filename not in members:
                continue
            raw = archive.read(member)
            pos = 0
            while pos < len(raw):
                length, kind, clen = struct.unpack_from('<III', raw, pos)
                end = pos + (length + 1) * 4
                if length < 2 or clen % 2 or pos + 12 + clen * 4 > end or end > len(raw):
                    raise RuntimeError(f'Malformed item in {member.filename} at {pos}')
                if kind == water_type:
                    outer = list(struct.iter_unpack('<ii', raw[pos + 12:pos + 12 + clen * 4]))
                    holes = []
                    attr = pos + 12 + clen * 4
                    while attr < end:
                        alen, atype = struct.unpack_from('<II', raw, attr)
                        aend = attr + (alen + 1) * 4
                        if alen < 1 or aend > end:
                            raise RuntimeError('Malformed polygon attribute')
                        if atype == hole_type:
                            count, = struct.unpack_from('<I', raw, attr + 8)
                            if count < 4 or alen != 2 + count * 2:
                                raise RuntimeError('Malformed polygon hole')
                            holes.append(list(struct.iter_unpack('<ii', raw[attr + 12:aend])))
                        attr = aend
                    result.append(dict(outer=outer, holes=holes))
                pos = end
    return result


def inside(ring, point):
    # Independent even/odd ray crossing oracle. Samples avoid polygon edges.
    x, y = point
    result = False
    for (ax, ay), (bx, by) in zip(ring, ring[1:] + ring[:1]):
        if (ay > y) != (by > y) and x < ax + (y - ay) * (bx - ax) / (by - ay):
            result = not result
    return result


def is_water(items, point):
    return any(inside(p['outer'], point) and not any(inside(h, point) for h in p['holes']) for p in items)



if __name__ == '__main__':
    source, binary, output = (Path(p).resolve() for p in sys.argv[1:4])
    output.mkdir(parents=True, exist_ok=False)
    osm = ET.Element('osm', version='0.6')
    ways = []
    node_id = 0

    def ring(x, y, size=1000):
        global node_id
        ids = []
        for dx, dy in [(0,0),(size,0),(size,size),(0,size)]:
            node_id += 1
            lon, lat = geo(x + dx + .25, y + dy + .25)
            ET.SubElement(osm,'node',id=str(node_id),version='1',lon=f'{lon:.12f}',lat=f'{lat:.12f}')
            ids.append(node_id)
        return ids + ids[:1]

    def way(refs, tags):
        wid = 1000 + len(ways)
        ways.append((wid, refs, tags))
        return wid

    # Both OSM water encodings, including distinct node IDs at identical positions.
    a = ring(100000,100000)
    way(a[:-1], {'natural':'water'})
    b = ring(102000,100000)
    way(b[:-1], {'waterway':'riverbank'})
    c = ring(104000,100000)
    way(c, {'natural':'water'})
    d = ring(106000,100000)
    e = ring(106000,100000)
    way(d[:-1] + e[:1], {'natural':'water'})
    way([c[0],c[1],c[0]], {'natural':'water'})  # Degenerate closed boundary.

    # Open tagged members must still assemble, including reversed inner segments.
    outer = ring(100000,104000,4000)
    inner = ring(101000,105000,2000)
    members = [(way(outer[:3], {'natural':'water'}),'outer'),
               (way(outer[2:], {'waterway':'riverbank'}),'outer'),
               (way(inner[:3], {'natural':'water'}),'inner'),
               (way(list(reversed(inner[2:])), {'natural':'water'}),'inner')]
    for wid, refs, tags in ways:
        w = ET.SubElement(osm,'way',id=str(wid),version='1')
        for ref in refs:
            ET.SubElement(w,'nd',ref=str(ref))
        for k,v in tags.items():
            ET.SubElement(w,'tag',k=k,v=v)
    r = ET.SubElement(osm,'relation',id='2000',version='1')
    for ref, role in members:
        ET.SubElement(r,'member',type='way',ref=str(ref),role=role)
    for k,v in {'type':'multipolygon','natural':'water','water':'river'}.items():
        ET.SubElement(r,'tag',k=k,v=v)
    ET.indent(osm)
    xml = output/'input.osm'
    ET.ElementTree(osm).write(xml, encoding='utf-8', xml_declaration=True)
    checks = [((100500,100500),False), ((102500,100500),False), ((104500,100500),True),
              ((106500,100500),False), ((102000,106000),False), ((100500,106000),True),
              ((103500,106000),True), ((102000,104500),True), ((102000,107500),True)]
    results = []
    for name, flags in [('default',[]), ('ignore-unknown',['-n'])]:
        folder=output/name
        folder.mkdir()
        with (folder/'conversion.log').open('w') as log:
            subprocess.run([str(binary),*flags,'-i',str(xml),str(folder/'map.bin')],cwd=folder,
                           stdout=log,stderr=subprocess.STDOUT,check=True)
        water = polygons(source,folder/'map.bin','poly_water')
        failures = [dict(point=p, expected=e, actual=is_water(water,p)) for p,e in checks if is_water(water,p)!=e]
        results.append(dict(mode=name, checks=len(checks), polygons=len(water), failures=failures))
    (output/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    print(json.dumps(results))
    assert all(not r['failures'] and r['polygons']==2 for r in results), results
