/**
 * Navit, a modular navigation system.
 *
 * OBD-II (ELM327) backend for Driver Break plugin.
 *
 * This module provides a best-effort, portable implementation that:
 * - Talks to an ELM327-compatible adapter over a serial port.
 * - Discovers supported PIDs via 0x00/0x20/0x40 bitmasks.
 * - Polls key PIDs (0x2F, 0x5E, 0x10, 0x0D, 0x51, 0x52) at reasonable intervals.
 * - Updates driver_break_priv fuel fields (fuel_rate_l_h, fuel_current, ethanol %, etc.).
 *
 * It is intentionally conservative: if the adapter or ECU does not respond as
 * expected, the backend disables itself and leaves manual/learning paths intact.
 */

#ifndef NAVIT_PLUGIN_DRIVER_BREAK_OBD_H
#define NAVIT_PLUGIN_DRIVER_BREAK_OBD_H

#include "config.h"
#include "driver_break.h"

struct driver_break_obd;

/**
 * @brief Create and start an OBD-II backend for the given plugin instance.
 *
 * The backend will attempt to open a serial device (default /dev/ttyUSB0) and
 * initialize an ELM327-style adapter. On success, it schedules periodic polling
 * of supported PIDs and feeds results into the supplied driver_break_priv.
 *
 * If initialization fails, the function returns NULL and the caller should fall
 * back to manual and adaptive estimation without live OBD-II data.
 */
struct driver_break_obd *driver_break_obd_start(struct driver_break_priv *priv);

/**
 * @brief Stop OBD-II backend and free resources.
 */
void driver_break_obd_stop(struct driver_break_obd *obd);

#endif /* NAVIT_PLUGIN_DRIVER_BREAK_OBD_H */

