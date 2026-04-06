#define _GNU_SOURCE

#include "location.h"
#include "../vendor/cJSON.h"
#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int location_get_filepath(char *buf, size_t bufsize) {
  const char *home = getenv("HOME");
  if (!home) {
    return -1;
  }
  snprintf(buf, bufsize, "%s/%s", home, LOCATIONS_FILE);
  return 0;
}

static int ensure_directory_exists(const char *filepath) {
  char dir[512];
  snprintf(dir, sizeof(dir), "%s", filepath);

  char *last_slash = strrchr(dir, '/');
  if (last_slash) {
    *last_slash = '\0';
    mkdir(dir, 0755);
  }
  return 0;
}

static void parse_geocoding_result(cJSON *item, Location *loc) {
  cJSON *name = cJSON_GetObjectItem(item, "name");
  cJSON *country = cJSON_GetObjectItem(item, "country_code");
  cJSON *lat = cJSON_GetObjectItem(item, "latitude");
  cJSON *lon = cJSON_GetObjectItem(item, "longitude");

  if (name && name->valuestring) {
    snprintf(loc->name, MAX_LOCATION_NAME, "%s", name->valuestring);
  }
  if (country && country->valuestring) {
    snprintf(loc->country, MAX_COUNTRY_CODE, "%s", country->valuestring);
  }
  if (lat) {
    loc->latitude = lat->valuedouble;
  }
  if (lon) {
    loc->longitude = lon->valuedouble;
  }
}

static void url_encode(const char *src, char *dst, size_t dst_size) {
  static const char *hex = "0123456789ABCDEF";
  size_t j = 0;
  for (size_t i = 0; src[i] && j < dst_size - 3; i++) {
    unsigned char c = (unsigned char)src[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
        c == '~') {
      dst[j++] = c;
    } else {
      dst[j++] = '%';
      dst[j++] = hex[c >> 4];
      dst[j++] = hex[c & 0x0F];
    }
  }
  dst[j] = '\0';
}

static int search_open_meteo(const char *encoded, LocationList *results) {
  char url[512];
  snprintf(url, sizeof(url), "%s?name=%s&count=%d&language=en&format=json",
           GEOCODING_API_URL, encoded, GEOCODING_MAX_RESULTS);

  HttpBuffer response;
  http_buffer_init(&response);

  if (http_get(url, &response) != 0) {
    http_buffer_free(&response);
    return -1;
  }

  cJSON *json = cJSON_Parse(response.data);
  http_buffer_free(&response);
  if (!json)
    return -1;

  cJSON *results_arr = cJSON_GetObjectItem(json, "results");
  if (cJSON_IsArray(results_arr)) {
    int n = cJSON_GetArraySize(results_arr);
    for (int i = 0; i < n && results->count < MAX_SAVED_LOCATIONS; i++) {
      parse_geocoding_result(cJSON_GetArrayItem(results_arr, i),
                             &results->items[results->count]);
      results->count++;
    }
  }

  cJSON_Delete(json);
  return results->count;
}

static void parse_nominatim_result(cJSON *item, Location *loc) {
  cJSON *name = cJSON_GetObjectItem(item, "name");
  cJSON *lat = cJSON_GetObjectItem(item, "lat");
  cJSON *lon = cJSON_GetObjectItem(item, "lon");
  cJSON *display = cJSON_GetObjectItem(item, "display_name");

  if (name && name->valuestring) {
    snprintf(loc->name, MAX_LOCATION_NAME, "%s", name->valuestring);
  }

  /* Extract country code from the end of display_name */
  loc->country[0] = '\0';
  if (display && display->valuestring) {
    const char *last_comma = strrchr(display->valuestring, ',');
    if (last_comma) {
      while (*last_comma == ',' || *last_comma == ' ')
        last_comma++;
      snprintf(loc->country, MAX_COUNTRY_CODE, "%s", last_comma);
    }
  }

  if (lat && lat->valuestring)
    loc->latitude = atof(lat->valuestring);
  if (lon && lon->valuestring)
    loc->longitude = atof(lon->valuestring);
}

static int search_nominatim(const char *encoded, LocationList *results) {
  char url[512];
  snprintf(url, sizeof(url), "%s?q=%s&format=json&limit=%d&accept-language=en",
           NOMINATIM_API_URL, encoded, GEOCODING_MAX_RESULTS);

  HttpBuffer response;
  http_buffer_init(&response);

  if (http_get(url, &response) != 0) {
    http_buffer_free(&response);
    return -1;
  }

  cJSON *json = cJSON_Parse(response.data);
  http_buffer_free(&response);
  if (!json || !cJSON_IsArray(json)) {
    cJSON_Delete(json);
    return -1;
  }

  int n = cJSON_GetArraySize(json);
  for (int i = 0; i < n && results->count < MAX_SAVED_LOCATIONS; i++) {
    parse_nominatim_result(cJSON_GetArrayItem(json, i),
                           &results->items[results->count]);
    results->count++;
  }

  cJSON_Delete(json);
  return results->count;
}

int location_search(const char *query, LocationList *results) {
  char encoded[256];
  url_encode(query, encoded, sizeof(encoded));
  results->count = 0;

  /* Try Open-Meteo first, fall back to Nominatim for fuzzy matching */
  int count = search_open_meteo(encoded, results);
  if (count > 0)
    return count;

  return search_nominatim(encoded, results);
}

static cJSON *location_to_json(const Location *loc) {
  cJSON *obj = cJSON_CreateObject();
  cJSON_AddStringToObject(obj, "name", loc->name);
  cJSON_AddStringToObject(obj, "country", loc->country);
  cJSON_AddNumberToObject(obj, "latitude", loc->latitude);
  cJSON_AddNumberToObject(obj, "longitude", loc->longitude);
  return obj;
}

int location_load(LocationList *list) {
  char filepath[512];
  if (location_get_filepath(filepath, sizeof(filepath)) != 0) {
    return -1;
  }

  list->count = 0;

  FILE *f = fopen(filepath, "r");
  if (!f) {
    return 0; /* no file yet is fine */
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *content = malloc(fsize + 1);
  size_t nread = fread(content, 1, fsize, f);
  (void)nread;
  content[fsize] = '\0';
  fclose(f);

  cJSON *json = cJSON_Parse(content);
  free(content);
  if (!json || !cJSON_IsArray(json)) {
    cJSON_Delete(json);
    return -1;
  }

  int n = cJSON_GetArraySize(json);
  for (int i = 0; i < n && i < MAX_SAVED_LOCATIONS; i++) {
    parse_geocoding_result(cJSON_GetArrayItem(json, i),
                           &list->items[list->count]);
    list->count++;
  }

  cJSON_Delete(json);
  location_sort(list);
  return 0;
}

int location_save(const LocationList *list) {
  char filepath[512];
  if (location_get_filepath(filepath, sizeof(filepath)) != 0) {
    return -1;
  }

  ensure_directory_exists(filepath);

  cJSON *arr = cJSON_CreateArray();
  for (int i = 0; i < list->count; i++) {
    cJSON_AddItemToArray(arr, location_to_json(&list->items[i]));
  }

  char *json_str = cJSON_Print(arr);
  cJSON_Delete(arr);

  FILE *f = fopen(filepath, "w");
  if (!f) {
    free(json_str);
    return -1;
  }

  fputs(json_str, f);
  fclose(f);
  free(json_str);

  return 0;
}

static int compare_locations(const void *a, const void *b) {
  const Location *la = (const Location *)a;
  const Location *lb = (const Location *)b;
  return strcasecmp(la->name, lb->name);
}

void location_sort(LocationList *list) {
  if (list->count > 1) {
    qsort(list->items, list->count, sizeof(Location), compare_locations);
  }
}

int location_add(LocationList *list, const Location *loc) {
  if (list->count >= MAX_SAVED_LOCATIONS) {
    return -1;
  }
  list->items[list->count] = *loc;
  list->count++;
  location_sort(list);
  return 0;
}

int location_remove(LocationList *list, int index) {
  if (index < 0 || index >= list->count) {
    return -1;
  }
  for (int i = index; i < list->count - 1; i++) {
    list->items[i] = list->items[i + 1];
  }
  list->count--;
  return 0;
}

int location_filter(const LocationList *all, const char *query,
                    LocationList *filtered) {
  filtered->count = 0;
  for (int i = 0; i < all->count; i++) {
    if (strcasestr(all->items[i].name, query) != NULL) {
      filtered->items[filtered->count] = all->items[i];
      filtered->count++;
    }
  }
  return filtered->count;
}
