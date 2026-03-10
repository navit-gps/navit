#!/bin/bash
# Generate GPX files for hiking test routes (DNT huts with name and elevation).
# Output: route_gpx/hiking_route_mogen_myrdal.gpx, route_gpx/hiking_route_gjendesheim_vetledalseter.gpx
# Use with Navit height profile and energy-based routing.

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="${SCRIPT_DIR}/route_gpx"
mkdir -p "${OUT_DIR}"

# Main route: Mogen (start) -> Tråastølen (via1) -> Finsehytta (via2) -> Myrdal Fjellstove (end)
# OSM: node 1356402304, 872147024, 444092409, way 1012805346 (center)
cat > "${OUT_DIR}/hiking_route_mogen_myrdal.gpx" << 'GPXMAIN'
<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="Navit Driver Break plugin - generate_hiking_route_gpx.sh"
     xmlns="http://www.topografix.com/GPX/1/1"
     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
  <metadata>
    <name>Hiking route: Mogen - Tråastølen - Finsehytta - Myrdal Fjellstove</name>
    <desc>DNT huts with elevation for height profile. Start node 1356402304, via 872147024, 444092409, end way 1012805346.</desc>
  </metadata>
  <rte>
    <name>Mogen to Myrdal Fjellstove</name>
    <rtept lat="60.0229368" lon="7.8902169">
      <ele>950</ele>
      <name>Mogen</name>
    </rtept>
    <rtept lat="60.3691699" lon="7.5217305">
      <ele>1240</ele>
      <name>Tråastølen</name>
    </rtept>
    <rtept lat="60.5979163" lon="7.5070391">
      <ele>1223</ele>
      <name>Finsehytta</name>
    </rtept>
    <rtept lat="60.7337393" lon="7.1206845">
      <ele>867</ele>
      <name>Myrdal Fjellstove</name>
    </rtept>
  </rte>
  <trk>
    <name>Mogen to Myrdal Fjellstove (track)</name>
    <trkseg>
      <trkpt lat="60.0229368" lon="7.8902169"><ele>950</ele></trkpt>
      <trkpt lat="60.3691699" lon="7.5217305"><ele>1240</ele></trkpt>
      <trkpt lat="60.5979163" lon="7.5070391"><ele>1223</ele></trkpt>
      <trkpt lat="60.7337393" lon="7.1206845"><ele>867</ele></trkpt>
    </trkseg>
  </trk>
  <wpt lat="60.0229368" lon="7.8902169"><ele>950</ele><name>Mogen</name></wpt>
  <wpt lat="60.3691699" lon="7.5217305"><ele>1240</ele><name>Tråastølen</name></wpt>
  <wpt lat="60.5979163" lon="7.5070391"><ele>1223</ele><name>Finsehytta</name></wpt>
  <wpt lat="60.7337393" lon="7.1206845"><ele>867</ele><name>Myrdal Fjellstove</name></wpt>
</gpx>
GPXMAIN

# Alternative route: Gjendesheim -> Vetledalseter
# OSM: node 791106132, 1356403887
cat > "${OUT_DIR}/hiking_route_gjendesheim_vetledalseter.gpx" << 'GPXALT'
<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="Navit Driver Break plugin - generate_hiking_route_gpx.sh"
     xmlns="http://www.topografix.com/GPX/1/1"
     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
  <metadata>
    <name>Hiking route: Gjendesheim - Vetledalseter</name>
    <desc>DNT huts with elevation for height profile. Start node 791106132, end node 1356403887.</desc>
  </metadata>
  <rte>
    <name>Gjendesheim to Vetledalseter</name>
    <rtept lat="61.4943459" lon="8.8126376">
      <ele>994</ele>
      <name>Gjendesheim</name>
    </rtept>
    <rtept lat="61.8450712" lon="7.1861445">
      <ele>515</ele>
      <name>Vetledalseter</name>
    </rtept>
  </rte>
  <trk>
    <name>Gjendesheim to Vetledalseter (track)</name>
    <trkseg>
      <trkpt lat="61.4943459" lon="8.8126376"><ele>994</ele></trkpt>
      <trkpt lat="61.8450712" lon="7.1861445"><ele>515</ele></trkpt>
    </trkseg>
  </trk>
  <wpt lat="61.4943459" lon="8.8126376"><ele>994</ele><name>Gjendesheim</name></wpt>
  <wpt lat="61.8450712" lon="7.1861445"><ele>515</ele><name>Vetledalseter</name></wpt>
</gpx>
GPXALT

echo "GPX files written to ${OUT_DIR}/"
echo "  hiking_route_mogen_myrdal.gpx"
echo "  hiking_route_gjendesheim_vetledalseter.gpx"
echo "Load in Navit and use energy-based routing / height profile to see hut names and elevations."
