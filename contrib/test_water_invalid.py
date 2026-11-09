"""Reject and report relations without shores instead of guessing at water.

Usage: python test_water_invalid.py SOURCE MAPTOOL OUTPUT_DIRECTORY
Models both isolated inner-only rings and a marsh also used by a valid lake.
"""
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

from test_inland_water import geo, is_water, polygons


def fixture(tags, reverse):
    root = ET.Element('osm', version='0.6')
    nodes, ways = [], []

    def square(x, size, wetland=False, water=False):
        ids = []
        for dx, dy in [(0, 0), (size, 0), (size, size), (0, size)]:
            nid = len(nodes) + 1
            nodes.append((nid, *geo(x + dx + .25, 100000 + dy + .25)))
            ids.append(nid)
        if reverse:
            ids.reverse()
        wid = 1000 + len(ways)
        ways.append((wid, ids + ids[:1], wetland, water))
        return wid

    shore = square(100000, 10000)
    marsh = square(102000, 4000, wetland=True)
    isolated = square(120000, 10000)
    independent = square(140000, 10000, water=True)
    for nid, lon, lat in nodes:
        ET.SubElement(root, 'node', id=str(nid), version='1', lon=str(lon), lat=str(lat))
    for wid, ids, wetland, water in ways:
        way = ET.SubElement(root, 'way', id=str(wid), version='1')
        for nid in ids:
            ET.SubElement(way, 'nd', ref=str(nid))
        for k, v in ([('natural', 'wetland'), ('wetland', 'marsh')] if wetland else tags if water else []):
            ET.SubElement(way, 'tag', k=k, v=v)
    members = [[(shore, 'outer'), (marsh, 'inner')], [(marsh, 'inner')],
               [(isolated, 'inner')], [(independent, 'inner')]]
    for rid, parts in enumerate(members, 2000):
        relation = ET.SubElement(root, 'relation', id=str(rid), version='1')
        for wid, role in parts:
            ET.SubElement(relation, 'member', type='way', ref=str(wid), role=role)
        for k, v in [('type', 'multipolygon'), *tags]:
            ET.SubElement(relation, 'tag', k=k, v=v)
    ET.indent(root)
    return ET.ElementTree(root)


if __name__ == '__main__':
    source, binary, output = (Path(p).resolve() for p in sys.argv[1:4])
    output.mkdir(parents=True, exist_ok=False)
    kinds = [('lake', [('natural', 'water')], 'poly_water'),
             ('river', [('waterway', 'riverbank')], 'poly_water'),
             ('reservoir', [('landuse', 'reservoir')], 'poly_reservoir'),
             ('basin', [('landuse', 'basin')], 'poly_basin')]
    checks = [((101000, 105000), True), ((104000, 102000), False),
              ((125000, 105000), False), ((145000, 105000), True)]
    results = []
    for kind, tags, item_type in kinds:
        for reverse in (False, True):
            work = output / f'{kind}-reverse{int(reverse)}'
            work.mkdir()
            fixture(tags, reverse).write(work / 'input.osm', encoding='utf-8', xml_declaration=True)
            with (work / 'conversion.log').open('w') as log:
                subprocess.run([str(binary), '-i', str(work / 'input.osm'), str(work / 'map.bin')],
                               cwd=work, stdout=log, stderr=log, check=True)
            water = polygons(source, work / 'map.bin', item_type)
            failures = [dict(point=p, expected=e, actual=is_water(water, p))
                        for p, e in checks if is_water(water, p) != e]
            log = (work / 'conversion.log').read_text()
            for rid in (2001, 2002, 2003):
                warning = (f'www.openstreetmap.org/relation/{rid} multipolygon: missing outer member; '
                           'skipping malformed relation, check source roles')
                if warning not in log:
                    failures.append(dict(missing_diagnostic=warning))
            results.append(dict(case=work.name, checks=checks, failures=failures))
    (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(dict(cases=len(results), failed=sum(bool(r['failures']) for r in results))))
    assert not any(r['failures'] for r in results), 'Invalid relation rejection or notification failed'
