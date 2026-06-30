/**
 * Navit, a modular navigation system.
 *
 * ECU fuel monitoring plugin public API for Driver Break integration.
 *
 * Provides narrow configuration and runtime fuel state structures so OBD-II,
 * J1939, and MegaSquirt backends do not depend on driver_break_priv.
 */

#ifndef NAVIT_PLUGIN_DRIVER_ECU_H
#define NAVIT_PLUGIN_DRIVER_ECU_H

#include "config.h"

/** Fuel type values (match driver_break_config.fuel_type numbering). */
enum driver_ecu_fuel_type {
    DRIVER_ECU_FUEL_PETROL = 0,
    DRIVER_ECU_FUEL_DIESEL = 1,
    DRIVER_ECU_FUEL_FLEX = 2,
    DRIVER_ECU_FUEL_CNG = 3,
    DRIVER_ECU_FUEL_LNG = 4,
    DRIVER_ECU_FUEL_LPG = 5,
    DRIVER_ECU_FUEL_HYDROGEN = 6,
    DRIVER_ECU_FUEL_ETHANOL = 7
};

struct ecu_config {
    int fuel_type;
    int fuel_tank_capacity_l;
    int fuel_ethanol_manual_pct;
    int fuel_obd_available;
    int fuel_j1939_available;
    int fuel_megasquirt_available;
    int fuel_injector_flow_cc_min;
};

struct ecu_fuel_state { /* ECU writes, core reads */
    double fuel_current;
    double fuel_rate_l_h;
};

struct driver_ecu_obd;
struct driver_ecu_j1939;
struct driver_ecu_megasquirt;

struct driver_ecu_obd *driver_ecu_obd_start(const struct ecu_config *config, struct ecu_fuel_state *fuel);

void driver_ecu_obd_stop(struct driver_ecu_obd *obd);

struct driver_ecu_j1939 *driver_ecu_j1939_start(const struct ecu_config *config, struct ecu_fuel_state *fuel);

void driver_ecu_j1939_stop(struct driver_ecu_j1939 *ctx);

struct driver_ecu_megasquirt *driver_ecu_megasquirt_start(const struct ecu_config *config, struct ecu_fuel_state *fuel);

void driver_ecu_megasquirt_stop(struct driver_ecu_megasquirt *ctx);

#endif /* NAVIT_PLUGIN_DRIVER_ECU_H */
