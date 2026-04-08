/*
 * weathercli - terminal weather forecast application
 * Written by xir. (github.com/xernot)
 * Copyright (C) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LOCATION_H
#define LOCATION_H

#include "config.h"

#include <stddef.h>

/* Represents a geographic location */
typedef struct {
  char name[MAX_LOCATION_NAME];
  char country[MAX_COUNTRY_CODE];
  double latitude;
  double longitude;
} location_t;

/* List of locations (used for search results and saved locations) */
typedef struct {
  location_t items[MAX_SAVED_LOCATIONS];
  int count;
} location_list_t;

/* Search for locations by name via geocoding API. Returns number of results, -1
 * on error. */
int location_search(const char *query, location_list_t *results);

/* Load saved locations from disk. Returns 0 on success, -1 on error. */
int location_load(location_list_t *list);

/* Save locations to disk. Returns 0 on success, -1 on error. */
int location_save(const location_list_t *list);

/* Sort locations alphabetically by name. */
void location_sort(location_list_t *list);

/* Add a location to the saved list. Returns 0 on success, -1 if full. */
int location_add(location_list_t *list, const location_t *loc);

/* Remove a location from the list by index. Returns 0 on success, -1 on error.
 */
int location_remove(location_list_t *list, int index);

/* Search saved locations by name (case-insensitive substring match). */
int location_filter(const location_list_t *all, const char *query,
                    location_list_t *filtered);

/* Get the storage file path. Returns 0 on success, -1 on error. */
int location_get_filepath(char *buf, size_t bufsize);

#endif /* LOCATION_H */
