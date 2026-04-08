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

#include "http.h"
#include "config.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

void http_buffer_init(http_buffer_t *buf) {
  buf->data = malloc(HTTP_BUFFER_INIT_SIZE);
  buf->data[0] = '\0';
  buf->size = 0;
  buf->capacity = HTTP_BUFFER_INIT_SIZE;
}

void http_buffer_free(http_buffer_t *buf) {
  free(buf->data);
  buf->data = NULL;
  buf->size = 0;
  buf->capacity = 0;
}

static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             void *userdata) {
  size_t total = size * nmemb;
  http_buffer_t *buf = (http_buffer_t *)userdata;

  if (buf->size + total >= HTTP_MAX_RESPONSE_SIZE) {
    return 0;
  }

  while (buf->size + total + 1 > buf->capacity) {
    buf->capacity *= 2;
    buf->data = realloc(buf->data, buf->capacity);
  }

  memcpy(buf->data + buf->size, contents, total);
  buf->size += total;
  buf->data[buf->size] = '\0';

  return total;
}

static int http_perform(CURL *curl, http_buffer_t *response) {
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)HTTP_TIMEOUT_SECONDS);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "weathercli/1.0");

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  return (res == CURLE_OK) ? 0 : -1;
}

int http_get(const char *url, http_buffer_t *response) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return -1;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  return http_perform(curl, response);
}

int http_post_json(const char *url, const char *json_body,
                   const char *auth_header, http_buffer_t *response) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return -1;
  }

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  if (auth_header) {
    headers = curl_slist_append(headers, auth_header);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  int result = http_perform(curl, response);
  curl_slist_free_all(headers);

  return result;
}
