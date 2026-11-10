"""Reject malformed inner networks; preserve independent valid water.
Usage: python test_water_junctions.py SOURCE MAPTOOL OUTPUT_DIRECTORY
"""
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET
from test_inland_water import geo, polygons, is_water

source, binary, output = map(lambda p: Path(p).resolve(), sys.argv[1:4])
output.mkdir(parents=True, exist_ok=False)
results = []
for kind, tags, item_type in [('lake', ('natural','water'), 'poly_water'),
                              ('river', ('waterway','riverbank'), 'poly_water'),
                              ('reservoir', ('landuse','reservoir'), 'poly_reservoir'),
                              ('basin', ('landuse','basin'), 'poly_basin')]:
    for mode in ['junction', 'reverse', 'permuted', 'duplicate', 'spur', 'gap']:
        root = ET.Element('osm', version='0.6')
        coords = [(100000,100000),(110000,100000),(110000,104000),(100000,104000),
                  (103000,101000),(103000,102000),(103000,103000),(105000,103000),(105000,101000),
                  (103600,100600),(104000,100800),(104400,100600),(102500,102000)]
        for nid,(x,y) in enumerate(coords,1):
            lon,lat=geo(x+.25,y+.25)
            ET.SubElement(root,'node',id=str(nid),version='1',lon=f'{lon:.12f}',lat=f'{lat:.12f}')
        ways = [[1,2,3,4,1], [5,6,7,8,9], [5,10,11,5], [11,12,9,11]]
        if mode == 'gap':
            ways.pop()
        if mode == 'spur':
            ways.append([6,13])
        if mode == 'duplicate':
            ways.append(ways[2][:])
        if mode == 'reverse':
            ways = [list(reversed(w)) for w in ways]
        for wid,ids in enumerate(ways,1000):
            w=ET.SubElement(root,'way',id=str(wid),version='1')
            for nid in ids:
                ET.SubElement(w,'nd',ref=str(nid))
        r=ET.SubElement(root,'relation',id='2000',version='1')
        order=list(range(len(ways)))
        if mode=='permuted':
            order=[3,2,1,0]
        for i in order:
            ET.SubElement(r,'member',type='way',ref=str(1000+i),role='outer' if i==0 else 'inner')
        for k,v in [('type','multipolygon'),tags]:
            ET.SubElement(r,'tag',k=k,v=v)
        # A separate valid relation proves rejection is local to the broken one.
        valid=ET.SubElement(root,'relation',id='2001',version='1')
        ET.SubElement(valid,'member',type='way',ref='1002',role='outer')
        for k,v in [('type','multipolygon'),tags]:
            ET.SubElement(valid,'tag',k=k,v=v)
        work=output/f'{kind}-{mode}'
        work.mkdir()
        ET.indent(root)
        ET.ElementTree(root).write(work/'input.osm',encoding='utf-8',xml_declaration=True)
        with (work/'conversion.log').open('w') as log:
            subprocess.run([str(binary),'-i',str(work/'input.osm'),str(work/'map.bin')],cwd=work,stdout=log,stderr=log,check=True)
        water=polygons(source,work/'map.bin',item_type)
        checks=[((104000,102000),False),((102400,102000),False),((106000,102000),False),((99000,102000),False),((103550,100850),True)]
        failures=[dict(point=p,expected=e,actual=is_water(water,p)) for p,e in checks if is_water(water,p)!=e]
        log=(work/'conversion.log').read_text()
        if 'www.openstreetmap.org/relation/2000 multipolygon: incomplete inner boundary; skipping water relation' not in log:
            failures.append(dict(error='Missing rejection diagnostic'))
        results.append(dict(case=work.name,checks=len(checks)+1,rejected=True,omitted_water_controls=2,failures=failures))
(output/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results),flush=True)
assert not any(r['failures'] for r in results), 'See results.json'
