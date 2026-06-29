/**
 * @file navit_safety_cache.h
 * @brief SQLite-backed confirmation cache for user-verified POIs.
 *
 * Persists which POIs a user has confirmed operational on a given trip so the
 * confidence of those stops can be upgraded to High for the rest of the trip
 * and across sessions. See docs/user/plugins/navit_safety.rst ("Unconfirmed
 * stops"). The cache is optional and only compiled when SQLite3 is available.
 *
 * Navit, a modular navigation system.
 *
 * Copyright (C) 2025-2026 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef NAVIT_PLUGIN_NAVIT_SAFETY_CACHE_H
#define NAVIT_PLUGIN_NAVIT_SAFETY_CACHE_H

/** @brief Opaque handle to a confirmation cache. */
struct navit_safety_cache;

/**
 * @brief Open (and create if needed) a confirmation cache.
 * @param path Database file path; ":memory:" for a transient cache.
 * @return A cache handle, or NULL on failure (caller owns it).
 */
struct navit_safety_cache *navit_safety_cache_open(const char *path);

/**
 * @brief Close a cache and release its resources.
 * @param cache Cache to close (NULL is ignored).
 */
void navit_safety_cache_close(struct navit_safety_cache *cache);

/**
 * @brief Record that a POI was confirmed operational on a trip.
 * @param cache Open cache (NULL fails).
 * @param poi_id Stable POI identifier (NULL fails).
 * @param trip_id Trip identifier (NULL fails).
 * @param timestamp Confirmation time (e.g. Unix seconds).
 * @return Nonzero on success, zero on failure.
 */
int navit_safety_cache_confirm(struct navit_safety_cache *cache, const char *poi_id,
                               const char *trip_id, long timestamp);

/**
 * @brief Test whether a POI is confirmed for a trip.
 * @param cache Open cache (NULL yields not-confirmed).
 * @param poi_id POI identifier.
 * @param trip_id Trip identifier.
 * @return Nonzero if a confirmation exists, zero otherwise.
 */
int navit_safety_cache_is_confirmed(struct navit_safety_cache *cache, const char *poi_id,
                                    const char *trip_id);

#endif /* NAVIT_PLUGIN_NAVIT_SAFETY_CACHE_H */
