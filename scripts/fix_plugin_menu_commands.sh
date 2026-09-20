#!/bin/bash
# Fix plugin command registration in ~/.navit/navit.xml
# Option A: Replace with installed config (simplest - use after rebuild/install)
# Option B: Patch existing config (enable OSDs, update onclick)
# Run this if you see "invalid command ignored" for driver_break or APRS commands.

NAVIT_XML="${HOME}/.navit/navit.xml"
INSTALLED_XML="/usr/local/share/navit/navit.xml"

# Option: use -f to force replace with installed config
if [ "$1" = "-f" ] && [ -f "$INSTALLED_XML" ]; then
    echo "Replacing $NAVIT_XML with installed config from $INSTALLED_XML"
    mkdir -p "$(dirname "$NAVIT_XML")"
    [ -f "$NAVIT_XML" ] && cp "$NAVIT_XML" "${NAVIT_XML}.bak"
    cp "$INSTALLED_XML" "$NAVIT_XML"
    echo "Done. Restart Navit."
    exit 0
fi

if [ ! -f "$NAVIT_XML" ]; then
    echo "No user config at $NAVIT_XML"
    echo "Copy installed: cp $INSTALLED_XML $NAVIT_XML"
    echo "Or run: $0 -f"
    exit 1
fi

echo "Backing up $NAVIT_XML to ${NAVIT_XML}.bak"
cp "$NAVIT_XML" "${NAVIT_XML}.bak"

echo "Adding APRS plugin path (required for OSD type 'aprs' to be found)..."
if ! grep -q 'libosd_aprs.so' "$NAVIT_XML"; then
    # Insert before </plugins>
    sed -i 's|</plugins>|\t<plugin path="$NAVIT_LIBDIR/plugin/${NAVIT_LIBPREFIX}libosd_aprs.so" ondemand="no"/>\n\t</plugins>|' "$NAVIT_XML"
fi

echo "Enabling driver_break and APRS OSDs (required for command registration)..."
# Uncomment driver_break OSD
sed -i "s|<!-- <osd enabled=\"yes\" type=\"driver_break\" data=\"\$HOME/.navit/driver_break_plugin.db\"/> -->|<osd enabled=\"yes\" type=\"driver_break\" data=\"\$HOME/.navit/driver_break_plugin.db\"/>|g" "$NAVIT_XML"
# Add APRS OSD if missing
if ! grep -q '<osd enabled="yes" type="aprs"/>' "$NAVIT_XML"; then
    perl -i -0pe "s|(<osd enabled=\"no\" type=\"button\" x=\"0\" y=\"0\")|\t\t<osd enabled=\"yes\" type=\"aprs\"/>\n\1|" "$NAVIT_XML" 2>/dev/null || \
    sed -i 's|<osd enabled="no" type="button" x="0" y="0"|<osd enabled="yes" type="aprs"/>\n\t\t<osd enabled="no" type="button" x="0" y="0"|' "$NAVIT_XML"
fi

echo "Updating plugin command invocations (navit. prefix)..."
sed -i "s/onclick='setting_aprs()'/onclick='navit.setting_aprs()'/g" "$NAVIT_XML"
sed -i "s/onclick='driver_break_configure_intervals()'/onclick='navit.driver_break_configure_intervals()'/g" "$NAVIT_XML"
sed -i "s/onclick='driver_break_configure_overnight()'/onclick='navit.driver_break_configure_overnight()'/g" "$NAVIT_XML"
sed -i "s/onclick='aprs_refresh_freq_index/onclick='navit.aprs_refresh_freq_index/g" "$NAVIT_XML"
sed -i "s/onclick='aprs_freq_/onclick='navit.aprs_freq_/g" "$NAVIT_XML"
sed -i "s/onclick='aprs_timeout_/onclick='navit.aprs_timeout_/g" "$NAVIT_XML"
sed -i "s/onclick='aprs_device_/onclick='navit.aprs_device_/g" "$NAVIT_XML"
sed -i "s/onclick='aprs_nmea_/onclick='navit.aprs_nmea_/g" "$NAVIT_XML"

echo "Done. Restart Navit to test."
echo "To restore: cp ${NAVIT_XML}.bak $NAVIT_XML"
