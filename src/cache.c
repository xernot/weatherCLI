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

#include "cache.h"
#include "config.h"
#include "location.h"
#include "weather.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static pthread_t g_refresh_thread;
static volatile int g_refresh_running = 0;
static pthread_mutex_t g_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static int ensure_cache_dir(char *dir, size_t dirsize) {
  const char *home = getenv("HOME");
  if (!home) {
    return -1;
  }
  char parent[512];
  snprintf(parent, sizeof(parent), "%s/.weathercli", home);
  mkdir(parent, 0755);
  snprintf(dir, dirsize, "%s/%s", home, CACHE_DIR);
  mkdir(dir, 0755);
  return 0;
}

static unsigned long fnv1a_hash(const char *s) {
  unsigned long h = 14695981039346656037UL;
  while (*s) {
    h ^= (unsigned char)*s++;
    h *= 1099511628211UL;
  }
  return h;
}

int cache_path_for_url(const char *url, char *buf, size_t bufsize) {
  char dir[512];
  if (ensure_cache_dir(dir, sizeof(dir)) != 0) {
    return -1;
  }
  snprintf(buf, bufsize, "%s/%016lx.json", dir, fnv1a_hash(url));
  return 0;
}

long cache_age_seconds(const char *url) {
  char path[768];
  if (cache_path_for_url(url, path, sizeof(path)) != 0) {
    return -1;
  }
  struct stat st;
  if (stat(path, &st) != 0) {
    return -1;
  }
  time_t now = time(NULL);
  if (now < st.st_mtime) {
    return 0;
  }
  return (long)(now - st.st_mtime);
}

long cache_mtime(const char *url) {
  char path[768];
  if (cache_path_for_url(url, path, sizeof(path)) != 0) {
    return 0;
  }
  struct stat st;
  if (stat(path, &st) != 0) {
    return 0;
  }
  return (long)st.st_mtime;
}

int cache_write(const char *url, const char *data, size_t size) {
  char path[768];
  if (cache_path_for_url(url, path, sizeof(path)) != 0) {
    return -1;
  }
  pthread_mutex_lock(&g_cache_mutex);
  FILE *f = fopen(path, "wb");
  if (!f) {
    pthread_mutex_unlock(&g_cache_mutex);
    return -1;
  }
  size_t written = fwrite(data, 1, size, f);
  fclose(f);
  pthread_mutex_unlock(&g_cache_mutex);
  return (written == size) ? 0 : -1;
}

int cache_read(const char *url, http_buffer_t *response) {
  char path[768];
  if (cache_path_for_url(url, path, sizeof(path)) != 0) {
    return -1;
  }
  pthread_mutex_lock(&g_cache_mutex);
  FILE *f = fopen(path, "rb");
  if (!f) {
    pthread_mutex_unlock(&g_cache_mutex);
    return -1;
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz <= 0 || (size_t)sz >= HTTP_MAX_RESPONSE_SIZE) {
    fclose(f);
    pthread_mutex_unlock(&g_cache_mutex);
    return -1;
  }
  while (response->capacity < (size_t)sz + 1) {
    response->capacity *= 2;
    response->data = realloc(response->data, response->capacity);
  }
  size_t n = fread(response->data, 1, (size_t)sz, f);
  response->data[n] = '\0';
  response->size = n;
  fclose(f);
  pthread_mutex_unlock(&g_cache_mutex);
  return 0;
}

static void refresh_one_location(const location_t *loc) {
  forecast_t f = {0};
  today_detail_t today = {0};
  today_detail_t tomorrow = {0};
  weather_fetch(loc->latitude, loc->longitude, FORECAST_9DAY, &f, 1);
  weather_fetch_dwd(loc->latitude, loc->longitude, FORECAST_9DAY, &f, 1);
  weather_fetch_hourly(loc->latitude, loc->longitude, &today, &tomorrow, 1);
  weather_fetch_hourly_dwd(loc->latitude, loc->longitude, &today, &tomorrow, 1);
}

static void refresh_all_locations(void) {
  location_list_t list = {0};
  if (location_load(&list) != 0) {
    return;
  }
  for (int i = 0; i < list.count && g_refresh_running; i++) {
    refresh_one_location(&list.items[i]);
  }
}

static void *refresh_loop(void *arg) {
  (void)arg;
  while (g_refresh_running) {
    refresh_all_locations();
    for (int i = 0; i < CACHE_REFRESH_INTERVAL_SECONDS && g_refresh_running;
         i++) {
      sleep(1);
    }
  }
  return NULL;
}

void cache_refresh_start(void) {
  if (g_refresh_running) {
    return;
  }
  g_refresh_running = 1;
  if (pthread_create(&g_refresh_thread, NULL, refresh_loop, NULL) != 0) {
    g_refresh_running = 0;
  }
}

void cache_refresh_stop(void) {
  if (!g_refresh_running) {
    return;
  }
  g_refresh_running = 0;
  pthread_join(g_refresh_thread, NULL);
}
