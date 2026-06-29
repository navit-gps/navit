-- Driver Break: EV OBD vehicle profile table (SQLite 3)
-- Canonical DDL for documentation and future plugin integration (not yet executed by Navit).
-- Keep in sync with ev_vehicle_profile.rst.
--
-- PID columns store the ISO-TP payload as contiguous hexadecimal (no spaces),
-- without the PCI length nibble in the first frame (plugin prepends length).
-- Example: mode 0x22 PID 0x015B -> request body hex "22015B".
-- Formula columns use the same expression subset as iternio/ev-obd-pids (Torque-like):
-- A,B,... bytes from positive response after SID, Signed(), Int16(A,B), etc.

CREATE TABLE IF NOT EXISTS driver_break_ev_vehicle_profile (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    profile_code TEXT NOT NULL UNIQUE,
    make TEXT NOT NULL,
    model TEXT NOT NULL,
    year_from INTEGER,
    year_to INTEGER,
    soc_request_hex TEXT NOT NULL,
    soc_formula TEXT,
    power_kw_request_hex TEXT,
    power_kw_formula TEXT,
    battery_temp_request_hex TEXT,
    battery_temp_formula TEXT,
    battery_voltage_request_hex TEXT,
    battery_voltage_formula TEXT,
    protocol_hint INTEGER NOT NULL DEFAULT 0,
    elm_preamble TEXT,
    enabled INTEGER NOT NULL DEFAULT 1,
    source_note TEXT,
    CHECK (enabled IN (0, 1)),
    CHECK (protocol_hint >= 0 AND protocol_hint <= 15),
    CHECK (year_from IS NULL OR year_from >= 1900 AND year_from <= 2100),
    CHECK (year_to IS NULL OR year_to >= 1900 AND year_to <= 2100),
    CHECK (year_from IS NULL OR year_to IS NULL OR year_from <= year_to)
);

CREATE INDEX IF NOT EXISTS driver_break_ev_vehicle_profile_lookup
    ON driver_break_ev_vehicle_profile (make, model, year_from, year_to);

CREATE INDEX IF NOT EXISTS driver_break_ev_vehicle_profile_code
    ON driver_break_ev_vehicle_profile (profile_code);

-- Example row (placeholders only; replace with verified PIDs before use):
-- INSERT INTO driver_break_ev_vehicle_profile (
--     profile_code, make, model, year_from, year_to,
--     soc_request_hex, soc_formula,
--     power_kw_request_hex, power_kw_formula,
--     battery_temp_request_hex, battery_temp_formula,
--     battery_voltage_request_hex, battery_voltage_formula,
--     protocol_hint, elm_preamble, enabled, source_note
-- ) VALUES (
--     'example_ev_placeholder',
--     'Example', 'EV', 2020, 2024,
--     '220000', 'A',
--     NULL, NULL,
--     NULL, NULL,
--     NULL, NULL,
--     6,
--     NULL,
--     0,
--     'Disabled template; replace hex and formulas with validated data.'
-- );
