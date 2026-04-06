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
} Location;

/* List of locations (used for search results and saved locations) */
typedef struct {
  Location items[MAX_SAVED_LOCATIONS];
  int count;
} LocationList;

/* Search for locations by name via geocoding API. Returns number of results, -1
 * on error. */
int location_search(const char *query, LocationList *results);

/* Load saved locations from disk. Returns 0 on success, -1 on error. */
int location_load(LocationList *list);

/* Save locations to disk. Returns 0 on success, -1 on error. */
int location_save(const LocationList *list);

/* Sort locations alphabetically by name. */
void location_sort(LocationList *list);

/* Add a location to the saved list. Returns 0 on success, -1 if full. */
int location_add(LocationList *list, const Location *loc);

/* Remove a location from the list by index. Returns 0 on success, -1 on error.
 */
int location_remove(LocationList *list, int index);

/* Search saved locations by name (case-insensitive substring match). */
int location_filter(const LocationList *all, const char *query,
                    LocationList *filtered);

/* Get the storage file path. Returns 0 on success, -1 on error. */
int location_get_filepath(char *buf, size_t bufsize);

#endif /* LOCATION_H */
