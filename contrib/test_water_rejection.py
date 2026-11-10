"""Reject incomplete inner boundaries instead of borrowing unrelated edges.

Usage: python test_water_rejection.py SOURCE MAPTOOL OUTPUT_DIRECTORY
Rejection deliberately omits the broken relation, including its wet controls.
An independent valid water body must survive; complete boundaries still work.
"""
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET
from test_inland_water import geo, polygons, is_water

source,binary,out=map(lambda s:Path(s).resolve(),sys.argv[1:4])
out.mkdir(parents=True,exist_ok=False)
results=[]
for kind,tags,item_type in [('lake',('natural','water'),'poly_water'),('river',('waterway','riverbank'),'poly_water'),
                            ('reservoir',('landuse','reservoir'),'poly_reservoir'),('basin',('landuse','basin'),'poly_basin')]:
 for mode in ['complete','split-member','split-missing-inner','missing-inner','degenerate','tiny-inner','submetre','exact','reversed','duplicate','two-islands','absent','open-helper','missing-helper-node','missing-helper-endpoint','line-helper','distinct-closure-nodes','nearby','ambiguous','orphan','outer-gap']:
  root=ET.Element('osm',version='0.6')
  nodes=[]
  ways=[]
  members=[]
  def node(x,y):
   nid=len(nodes)+1
   nodes.append((nid,*geo(x+.25,y+.25)))
   return nid
  def shape(coords):return [node(*c) for c in coords]
  def way(ids,tag=None,role=None):
   wid=1000+len(ways)
   ways.append((wid,ids,tag))
   if role:
    members.append((wid,role))
   return wid
  outer=shape([(100000,100000),(110000,100000),(110000,104000),(100000,104000)])
  way(outer if mode=='orphan' else outer+outer[:1],role='outer')
  a=shape([(101000,101000),(103000,101000),(103000,103000),(101000,103000)])
  if mode=='tiny-inner':
   a=shape([(101000,101000),(101001,101000),(101000,101001)])
  ids=list(reversed(a)) if mode=='reversed' else a
  if mode in ['complete','split-member','split-missing-inner','tiny-inner']:
   ids=ids+a[:1]
  if mode=='degenerate':
   ids=[a[0],a[1],a[0]]
  way(ids,('highway','service') if mode.startswith('split-') else None,role='outer' if mode=='outer-gap' else 'inner')
  if mode=='missing-inner':
   members[-1]=(99999,'inner')
   ways.pop()
  if mode.startswith('split-'):
   way([node(102000,100500),a[1],node(104000,101500)],('highway','service'))
  if mode=='split-missing-inner':
   members.append((99999,'inner'))
  holes=[a]
  if mode in ['two-islands','ambiguous']:
   b=shape([(106000,101000),(108000,101000),(108000,103000),(106000,103000)])
   way(b,role='inner')
   holes.append(b)
  if mode!='absent':
   for index,h in enumerate(holes):
    x=100500 if index==0 else 105500
    helper=[h[-1],h[0],node(x,101000),node(x,103000),h[-1]]
    if mode=='open-helper':
     helper=helper[:-1]
    if mode=='missing-helper-node':
     helper.insert(1,999999)
    if mode=='missing-helper-endpoint':
     helper=[999999]+helper+[999999]
    if mode=='distinct-closure-nodes':
     helper[-1]=node(101000,103000)
    if mode in ['nearby','submetre']:
     offset=1 if mode=='nearby' else .4
     helper=shape([(101000+offset,103000),(101000+offset,101000),(100500,101000),(100500,103000)])
     helper+=helper[:1]
    if mode=='reversed':
     helper.reverse()
    tag=('highway','service') if mode=='line-helper' else ('natural','wetland')
    way(helper,tag)
    if mode=='duplicate':
     way(helper,tag)
   if mode=='ambiguous':
    way([a[0],b[0],node(104000,100500),a[0]],('natural','wood'))
  wet=shape([(120000,100000),(122000,100000),(122000,102000),(120000,102000)])
  way(wet+wet[:1],tags)
  for nid,lon,lat in nodes:
   ET.SubElement(root,'node',id=str(nid),version='1',lon=f'{lon:.12f}',lat=f'{lat:.12f}')
  for wid,ids,tag in ways:
   w=ET.SubElement(root,'way',id=str(wid),version='1')
   for nid in ids:
    ET.SubElement(w,'nd',ref=str(nid))
   if tag:
    ET.SubElement(w,'tag',k=tag[0],v=tag[1])
  r=ET.SubElement(root,'relation',id='2000',version='1')
  for wid,role in members:
   ET.SubElement(r,'member',type='way',ref=str(wid),role=role)
  for k,v in [('type','multipolygon'),tags]:
   ET.SubElement(r,'tag',k=k,v=v)
  d=out/(kind+'-'+mode)
  d.mkdir()
  ET.indent(root)
  ET.ElementTree(root).write(d/'input.osm',encoding='utf-8',xml_declaration=True)
  with (d/'conversion.log').open('w') as log:
   subprocess.run([str(binary),'-i',str(d/'input.osm'),str(d/'map.bin')],cwd=d,stdout=log,stderr=subprocess.STDOUT,check=True)
  water=polygons(source,d/'map.bin',item_type)
  rejected=mode not in ['complete','split-member','outer-gap','degenerate','tiny-inner']
  checks=[((100500,102000),not rejected),((104500,102000),not rejected),
          ((99000,102000),False),((102000,102000),mode in ['outer-gap','degenerate','tiny-inner']),((121000,101000),True)]
  log=(d/'conversion.log').read_text()
  failures=[dict(point=p,expected=e,actual=is_water(water,p)) for p,e in checks if is_water(water,p)!=e]
  warning='www.openstreetmap.org/relation/2000 multipolygon: incomplete inner boundary; skipping water relation'
  if (warning in log)!=rejected:
   failures.append(dict(expected_rejection=rejected,log=log))
  if 'recovered boundary edge' in log:
   failures.append(dict(error='Borrowed an unrelated boundary'))
  results.append(dict(case=d.name,checks=len(checks)+2,rejected=rejected,
                      omitted_water_controls=2 if rejected and mode!='orphan' else 0,failures=failures))
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results),flush=True)
assert not any(r['failures'] for r in results),'See results.json'
