/*
 * weathercli - terminal weather forecast application
 * Written by xir. (github.com/xernot)
 * Copyright (C) 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef CACHE_H
#define CACHE_H

#include "http.h"

#include <stddef.h>

/* Compute the on-disk cache file path for a given URL.
 * Returns 0 on success, -1 on error. */
int cache_path_for_url(const char *url, char *buf, size_t bufsize);

/* Return age of the cached entry for the given URL in seconds, or -1 if no
 * cache entry exists. */
long cache_age_seconds(const char *url);

/* Return the on-disk mtime of the cached entry for the given URL, or 0 if
 * no cache entry exists. */
long cache_mtime(const char *url);

/* Persist a raw HTTP response body to the cache, keyed by URL.
 * Returns 0 on success, -1 on error. */
int cache_write(const char *url, const char *data, size_t size);

/* Load a previously cached response into the provided http_buffer_t.
 * Returns 0 on success, -1 if no cache entry exists. */
int cache_read(const char *url, http_buffer_t *response);

/* Start the background refresh thread that pre-warms the cache for
 * every saved location every CACHE_REFRESH_INTERVAL_SECONDS. */
void cache_refresh_start(void);

/* Signal the background thread to exit and join it. */
void cache_refresh_stop(void);

#endif /* CACHE_H */
