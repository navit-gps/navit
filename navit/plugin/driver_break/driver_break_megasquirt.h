/**
 * Navit, a modular navigation system.
 *
 * MegaSquirt serial backend for Driver Break plugin.
 *
 * This backend talks to a MegaSquirt family ECU (MS1/MS2/MS3/MS3-Pro/MicroSquirt)
 * over a serial (RS-232/USB-serial) connection using the realtime data command.
 * It derives a fuel rate in L/h from injector pulse width and RPM and writes it
 * into the same fields that OBD-II and J1939 use.
 *
 * The backend is intentionally conservative: if the serial port cannot be
 * opened or no plausible realtime data is received, it will quietly fail and
 * the plugin falls back to manual/adaptive estimation.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_MEGASQUIRT_H
#define NAVIT_PLUGIN_DRIVER_BREAK_MEGASQUIRT_H

struct driver_break_priv;
struct driver_break_megasquirt;

/**
 * @brief Create and start a MegaSquirt backend for the given plugin instance.
 *
 * The backend will:
 *  - Open the configured serial device in raw 115200 8N1 mode.
 *  - Optionally detect firmware by sending a version command.
 *  - Periodically poll realtime data and update fuel_rate_l_h.
 *
 * The backend only starts if the configuration flag fuel_megasquirt_available
 * is set and a valid injector flow rate has been configured. On failure it
 * returns NULL and logs a warning.
 *
 * @param priv Driver Break plugin private data.
 * @return Opaque backend pointer, or NULL on error / disabled.
 */
struct driver_break_megasquirt *driver_break_megasquirt_start(struct driver_break_priv *priv);

/**
 * @brief Stop MegaSquirt backend and free resources.
 *
 * @param ctx MegaSquirt backend pointer returned from start.
 */
void driver_break_megasquirt_stop(struct driver_break_megasquirt *ctx);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_MEGASQUIRT_H */

/**
 * Navit, a modular navigation system.
 *
 * MegaSquirt serial backend for Driver Break plugin.
 *
 * Supports MegaSquirt family (MS1, MS2, MS3, MS3-Pro, MicroSquirt) over
 * RS-232/USB serial. Uses the legacy "A" (realtime data) command; RPM and
 * pulse width are parsed to compute fuel rate when fuel_injector_flow_cc_min
 * is set. Protocol reference: MSExtra documentation.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_MEGASQUIRT_H
#define NAVIT_PLUGIN_DRIVER_BREAK_MEGASQUIRT_H

#include "config.h"
#include "driver_break.h"

struct driver_break_megasquirt;

/**
 * @brief Create and start a MegaSquirt serial backend for the given plugin instance.
 *
 * Opens the serial device (default /dev/ttyUSB0), sends version request to detect
 * MS2/MS3/MicroSquirt, then polls realtime data. Feeds fuel_rate_l_h into
 * driver_break_priv. Returns NULL if not configured or init fails.
 */
struct driver_break_megasquirt *driver_break_megasquirt_start(struct driver_break_priv *priv);

/**
 * @brief Stop MegaSquirt backend and free resources.
 */
void driver_break_megasquirt_stop(struct driver_break_megasquirt *ms);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_MEGASQUIRT_H */
