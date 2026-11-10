"""Incomplete shores must not turn islands or open ways into water.

Usage: python test_water_incomplete.py SOURCE MAPTOOL OUTPUT_DIRECTORY
Exercises actual conversion with dry controls and retained water controls.
"""
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

from test_inland_water import geo, polygons, is_water


def fixture(tags, incomplete, reverse, outer_role):
    osm = ET.Element('osm', version='0.6')
    nodes, ways, relations = [], [], []

    def square(x, y, size, closed=True, tagged=False):
        ids = []
        for dx, dy in [(0, 0), (size, 0), (size, size), (0, size)]:
            nid = len(nodes) + 1
            lon, lat = geo(x + dx + .25, y + dy + .25)
            nodes.append((nid, lon, lat))
            ids.append(nid)
        if reverse:
            ids.reverse()
        wid = 1000 + len(ways)
        ways.append((wid, ids + ids[:1] if closed else ids, tagged))
        return wid

    # The outer way can be present but open, or entirely absent from an extract.
    outer = square(100000, 100000, 10000, closed=False)
    if incomplete == 'missing':
        ways.pop()
        outer = 9999
    island = square(102000, 102000, 6000)
    lake = square(104000, 104000, 2000)
    small_island = square(104500, 104500, 1000)
    members = [(outer, outer_role), (island, 'inner'), (lake, outer_role), (small_island, 'inner')]
    # A separate complete component must neither hide the regression nor be lost.
    separate = square(120000, 100000, 10000)
    separate_hole = square(122000, 102000, 6000)
    members += [(separate, outer_role), (separate_hole, 'inner')]
    relations.append(members)
    # Also test an orphan without any surviving outer component.
    outer2 = square(140000, 100000, 10000, closed=False)
    if incomplete == 'missing':
        ways.pop()
        outer2 = 9998
    relations.append([(outer2, outer_role), (square(142000, 102000, 6000), 'inner')])
    # Standalone ways, and tagged open members of a valid water multipolygon.
    square(160000, 100000, 10000, closed=False, tagged=True)
    square(180000, 100000, 10000, tagged=True)
    split = square(200000, 100000, 10000, tagged=True)
    wid, ids, tagged = ways.pop()
    ways.extend([(wid, ids[:3], tagged), (wid + 1, ids[2:], tagged)])
    hole = square(202000, 102000, 6000)
    relations.append([(split, outer_role), (split + 1, outer_role), (hole, 'inner')])
    for nid, lon, lat in nodes:
        ET.SubElement(osm, 'node', id=str(nid), version='1', lon=str(lon), lat=str(lat))
    for wid, refs, tagged in ways:
        way = ET.SubElement(osm, 'way', id=str(wid), version='1')
        for ref in refs:
            ET.SubElement(way, 'nd', ref=str(ref))
        if tagged:
            for k, v in tags:
                ET.SubElement(way, 'tag', k=k, v=v)
    for rid, members in enumerate(relations, 2000):
        relation = ET.SubElement(osm, 'relation', id=str(rid), version='1')
        for wid, role in reversed(members) if reverse else members:
            ET.SubElement(relation, 'member', type='way', ref=str(wid), role=role)
        for k, v in [('type', 'multipolygon'), *tags]:
            ET.SubElement(relation, 'tag', k=k, v=v)
    ET.indent(osm)
    return ET.ElementTree(osm)


if __name__ == '__main__':
    source, binary, output = (Path(p).resolve() for p in sys.argv[1:4])
    output.mkdir(parents=True, exist_ok=False)
    checks = [((103000, 103000), False), ((104250, 105000), True),
              ((105000, 105000), False), ((121000, 105000), True),
              ((125000, 105000), False), ((145000, 105000), False),
              ((165000, 105000), False), ((185000, 105000), True),
              ((201000, 105000), True), ((205000, 105000), False)]
    results = []
    kinds = [('lake', [('natural', 'water')], 'poly_water'),
             ('river', [('waterway', 'riverbank')], 'poly_water'),
             ('reservoir', [('landuse', 'reservoir')], 'poly_reservoir'),
             ('basin', [('landuse', 'basin')], 'poly_basin')]
    for kind, tags, item_type in kinds:
        for incomplete in ['open', 'missing']:
            for reverse in [False, True]:
                for outer_role in ['outer', '']:
                    for ignore in [False, True]:
                        name = f'{kind}-{incomplete}-reverse{int(reverse)}-role{outer_role}-ignore{int(ignore)}'
                        folder = output / name
                        folder.mkdir()
                        fixture(tags, incomplete, reverse, outer_role).write(folder / 'input.osm', encoding='utf-8', xml_declaration=True)
                        command = [str(binary), *(['-n'] if ignore else []), '-i', str(folder / 'input.osm'), str(folder / 'map.bin')]
                        with (folder / 'conversion.log').open('w') as log:
                            subprocess.run(command, cwd=folder, stdout=log, stderr=subprocess.STDOUT, check=True)
                        water = polygons(source, folder / 'map.bin', item_type)
                        failures = [dict(point=p, expected=e, actual=is_water(water, p))
                                    for p, e in checks if is_water(water, p) != e]
                        results.append(dict(case=name, checks=len(checks), failures=failures))
    (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    print(json.dumps(dict(cases=len(results), failed=sum(bool(r['failures']) for r in results))))
    assert not any(r['failures'] for r in results), 'Incomplete water regression; see results.json'
