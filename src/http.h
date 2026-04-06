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

#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

/* Dynamic buffer for HTTP response data */
typedef struct {
  char *data;
  size_t size;
  size_t capacity;
} HttpBuffer;

/* Initialize an HTTP buffer */
void http_buffer_init(HttpBuffer *buf);

/* Free an HTTP buffer */
void http_buffer_free(HttpBuffer *buf);

/* Perform an HTTP GET request. Returns 0 on success, -1 on failure. */
int http_get(const char *url, HttpBuffer *response);

/* Perform an HTTP POST request with JSON body. Returns 0 on success, -1 on
 * failure. */
int http_post_json(const char *url, const char *json_body,
                   const char *auth_header, HttpBuffer *response);

#endif /* HTTP_H */
