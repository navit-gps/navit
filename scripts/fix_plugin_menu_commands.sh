#!/bin/bash
# Fix plugin command registration in ~/.navit/navit.xml
# Option A: Replace with installed config (simplest - use after rebuild/install)
# Option B: Patch existing config (enable OSD, update onclick)
# Run this if you see "invalid command ignored" for driver_break commands.

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

echo "Enabling driver_break OSD (required for command registration)..."
sed -i "s|<!-- <osd enabled=\"yes\" type=\"driver_break\" data=\"\$HOME/.navit/driver_break_plugin.db\"/> -->|<osd enabled=\"yes\" type=\"driver_break\" data=\"\$HOME/.navit/driver_break_plugin.db\"/>|g" "$NAVIT_XML"

echo "Updating plugin command invocations (navit. prefix)..."
sed -i "s/onclick='driver_break_configure_intervals()'/onclick='navit.driver_break_configure_intervals()'/g" "$NAVIT_XML"
sed -i "s/onclick='driver_break_configure_overnight()'/onclick='navit.driver_break_configure_overnight()'/g" "$NAVIT_XML"

echo "Done. Restart Navit to test."
echo "To restore: cp ${NAVIT_XML}.bak $NAVIT_XML"
