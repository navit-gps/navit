#!/usr/bin/python
import json
import argparse
parser = argparse.ArgumentParser()

parser.add_argument("-csv" , help="Output all files opmitted as a csv file, saperated by comma or if set by opmitted seperator", action="store_true")
parser.add_argument("-csv-seperator", help="Set the seperator used by csv output", default=",")
parser.add_argument("files", type=argparse.FileType('r'), nargs='+')

args=parser.parse_args()

for f in args.files:
	content=f.read()
	json_obj = json.loads(content)
	minx=999999
	maxx=-999999
	miny=999999
	maxy=-999999
	if json_obj['geometry']['type'] == "Polygon":
		for region in json_obj['geometry']['coordinates'][0]:
			minx=min(minx,region[0])
			maxx=max(maxx,region[0])
			miny=min(miny,region[1])
			maxy=max(maxy,region[1])
	elif json_obj['geometry']['type'] == "MultiPolygon":
		for sub in json_obj['geometry']['coordinates']:
			for region in sub[0]:
				minx=min(minx,region[0])
				maxx=max(maxx,region[0])
				miny=min(miny,region[1])
				maxy=max(maxy,region[1])
	if args.csv:
		print(f.name+args.csv_seperator+str(minx)+args.csv_seperator+str(miny)+args.csv_seperator+str(maxx)+args.csv_seperator+str(maxy))
	else:
		print(f.name+": "+str(minx)+" "+str(miny)+" x "+str(maxx)+" "+str(maxy))
