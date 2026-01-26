# Debugging Rest Plugin Command Issues

## Problem
Commands `rest_configure_intervals` and `rest_configure_overnight` show as "invalid command ignored" and menu items do nothing.

## Verification Steps

1. **Check if plugin is built:**
   ```bash
   ls -la build/navit/plugin/rest/libplugin_rest.so
   ```
   Should show the .so file exists.

2. **Check if plugin is loaded:**
   Run Navit with debug output:
   ```bash
   NAVIT_LOGFILE=navit_debug.log navit
   ```
   Then check the log for:
   - "Rest plugin initializing" - confirms plugin_init() was called
   - "Rest plugin OSD initialized successfully" - confirms rest_osd_new() was called
   - "Rest plugin: Menu commands registered" - confirms commands were registered
   - "Rest plugin: rest_cmd_configure_intervals called" - confirms command was invoked

3. **Check XML configuration:**
   Verify in `make/navit/navit.xml`:
   ```xml
   <osd enabled="yes" type="rest" data="$HOME/.navit/rest_plugin.db"/>
   ```
   This entry must exist for the OSD to be instantiated.

4. **Check plugin path:**
   The XML should have:
   ```xml
   <plugin path="$NAVIT_LIBDIR/*/${NAVIT_LIBPREFIX}lib*.so" ondemand="yes"/>
   ```
   When running from build directory, `$NAVIT_LIBDIR` should point to where plugins are located.

5. **Verify OSD instantiation:**
   If you see "Rest plugin initializing" but NOT "Rest plugin OSD initialized successfully", the OSD is not being instantiated. This could be due to:
   - XML syntax error
   - Missing dependencies
   - Database initialization failure

## Common Issues

### Plugin not loading
- Plugin .so file not in expected location
- Plugin dependencies not available
- Check: Look for "Rest plugin initializing" in debug log

### OSD not instantiating
- XML entry missing or incorrect
- Database path issue
- Check: Look for "Rest plugin OSD initialized successfully" in debug log

### Commands not registered
- OSD not instantiated (commands registered in rest_osd_new)
- Check: Look for "Rest plugin: Menu commands registered" in debug log

### Commands called but plugin not found
- OSD was destroyed or not yet created
- Check: Look for "Rest plugin: Plugin not found" in debug log

## Solution

If the plugin is not loading, ensure:
1. Plugin is built: `cd build && make`
2. Plugin is in the path Navit expects (check $NAVIT_LIBDIR)
3. Restart Navit completely after XML changes
4. Check debug log for error messages

If OSD is not instantiating:
1. Verify XML syntax is correct
2. Check database path is valid
3. Look for initialization errors in debug log
