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

#include "weather.h"
#include "../vendor/cJSON.h"
#include "cache.h"
#include "http.h"

#include <stdio.h>
#include <string.h>

/* Try a live HTTP GET; on success cache the body, on failure fall back
 * to a previously cached response for the same URL. */
static int http_get_with_cache(const char *url, http_buffer_t *response,
                               int force) {
  if (!force) {
    long age = cache_age_seconds(url);
    if (age >= 0 && age < CACHE_MAX_AGE_SECONDS) {
      if (cache_read(url, response) == 0) {
        return 0;
      }
    }
  }
  if (http_get(url, response) == 0 && response->size > 0 &&
      response->data[0] == '{') {
    cache_write(url, response->data, response->size);
    return 0;
  }
  /* Live request failed or returned a non-JSON body (e.g. 502 HTML).
   * Reset the buffer and try the cache. */
  response->size = 0;
  if (response->data) {
    response->data[0] = '\0';
  }
  return cache_read(url, response);
}

static void build_weather_url(char *url, size_t url_size, const char *base_url,
                              double lat, double lon, int num_days) {
  snprintf(url, url_size,
           "%s?latitude=%.4f&longitude=%.4f&daily="
           "temperature_2m_max,temperature_2m_min,"
           "precipitation_sum,weathercode,"
           "windspeed_10m_max,relative_humidity_2m_mean,"
           "uv_index_max&timezone=auto&forecast_days=%d",
           base_url, lat, lon, num_days);
}

static int parse_day(cJSON *daily, int index, day_weather_t *day) {
  cJSON *dates = cJSON_GetObjectItem(daily, "time");
  cJSON *tmax = cJSON_GetObjectItem(daily, "temperature_2m_max");
  cJSON *tmin = cJSON_GetObjectItem(daily, "temperature_2m_min");
  cJSON *precip = cJSON_GetObjectItem(daily, "precipitation_sum");
  cJSON *wcode = cJSON_GetObjectItem(daily, "weathercode");
  cJSON *wind = cJSON_GetObjectItem(daily, "windspeed_10m_max");
  cJSON *humid = cJSON_GetObjectItem(daily, "relative_humidity_2m_mean");
  cJSON *uv = cJSON_GetObjectItem(daily, "uv_index_max");

  cJSON *date_item = cJSON_GetArrayItem(dates, index);
  if (date_item && date_item->valuestring) {
    snprintf(day->date, sizeof(day->date), "%s", date_item->valuestring);
  }

  cJSON *item;

  item = cJSON_GetArrayItem(tmax, index);
  day->temp_max = item ? item->valuedouble : 0;

  item = cJSON_GetArrayItem(tmin, index);
  day->temp_min = item ? item->valuedouble : 0;

  item = cJSON_GetArrayItem(precip, index);
  day->precipitation = item ? item->valuedouble : 0;

  item = cJSON_GetArrayItem(wcode, index);
  day->weather_code = item ? item->valueint : 0;

  item = cJSON_GetArrayItem(wind, index);
  day->wind_speed_max = item ? item->valuedouble : 0;

  item = cJSON_GetArrayItem(humid, index);
  day->humidity = item ? (int)item->valuedouble : 0;

  item = cJSON_GetArrayItem(uv, index);
  day->uv_index = item ? item->valuedouble : 0;

  return 0;
}

static int parse_forecast_json(cJSON *json, forecast_t *forecast) {
  cJSON *daily = cJSON_GetObjectItem(json, "daily");
  if (!daily)
    return -1;

  cJSON *tz = cJSON_GetObjectItem(json, "timezone");
  if (tz && tz->valuestring) {
    snprintf(forecast->timezone, sizeof(forecast->timezone), "%s",
             tz->valuestring);
  }

  cJSON *dates = cJSON_GetObjectItem(daily, "time");
  int n = cJSON_GetArraySize(dates);
  if (n > FORECAST_9DAY)
    n = FORECAST_9DAY;

  forecast->num_days = n;
  for (int i = 0; i < n; i++) {
    parse_day(daily, i, &forecast->days[i]);
  }
  return 0;
}

static int fetch_forecast(const char *base_url, double latitude,
                          double longitude, int num_days, forecast_t *forecast,
                          int force) {
  char url[WEATHER_URL_MAX];
  build_weather_url(url, sizeof(url), base_url, latitude, longitude, num_days);

  http_buffer_t response;
  http_buffer_init(&response);
  if (http_get_with_cache(url, &response, force) != 0) {
    http_buffer_free(&response);
    return -1;
  }

  cJSON *json = cJSON_Parse(response.data);
  http_buffer_free(&response);
  if (!json)
    return -1;

  int result = parse_forecast_json(json, forecast);
  cJSON_Delete(json);
  return result;
}

static void build_hourly_url(char *url, size_t url_size, const char *base_url,
                             double lat, double lon, int days) {
  snprintf(url, url_size,
           "%s?latitude=%.4f&longitude=%.4f&hourly="
           "temperature_2m,weathercode,windspeed_10m,"
           "precipitation,relativehumidity_2m"
           "&timezone=auto&forecast_days=%d",
           base_url, lat, lon, days);
}

static void init_section(hourly_section_t *s, const char *label) {
  s->label = label;
  s->temp_max = -999;
  s->temp_min = 999;
  s->precipitation = 0;
  s->wind_speed_max = 0;
  s->weather_code = 0;
  s->humidity = 0;
  s->valid = 0;
}

static void aggregate_hour(hourly_section_t *s, double temp, int wcode,
                           double wind, double precip, int humid) {
  if (temp > s->temp_max)
    s->temp_max = temp;
  if (temp < s->temp_min)
    s->temp_min = temp;
  s->precipitation += precip;
  if (wind > s->wind_speed_max)
    s->wind_speed_max = wind;
  if (wcode > s->weather_code)
    s->weather_code = wcode;
  s->humidity += humid;
  s->valid++;
}

static void finalize_section(hourly_section_t *s) {
  if (s->valid > 0) {
    s->humidity /= s->valid;
  }
}

static int hour_to_section(int hour) {
  if (hour >= SECTION_MORNING_START && hour < SECTION_MORNING_END)
    return 0;
  if (hour >= SECTION_AFTERNOON_START && hour < SECTION_AFTERNOON_END)
    return 1;
  if (hour >= SECTION_EVENING_START && hour < SECTION_EVENING_END)
    return 2;
  return 3;
}

/* Scan the hourly time array and write the date string of the Nth distinct
 * day into out. Returns 0 on success, -1 if not found. */
static int find_nth_date(cJSON *times, int day_index, char *out,
                         size_t out_size) {
  int n = cJSON_GetArraySize(times);
  int dates_seen = -1;
  char prev_date[DATE_STR_MAX] = {0};

  for (int i = 0; i < n; i++) {
    cJSON *t = cJSON_GetArrayItem(times, i);
    if (!t || !t->valuestring)
      continue;
    char cur_date[DATE_STR_MAX];
    snprintf(cur_date, sizeof(cur_date), "%.10s", t->valuestring);
    if (strcmp(cur_date, prev_date) != 0) {
      dates_seen++;
      snprintf(prev_date, sizeof(prev_date), "%s", cur_date);
      if (dates_seen == day_index) {
        snprintf(out, out_size, "%s", cur_date);
        return 0;
      }
    }
  }
  return -1;
}

/* Extract a numeric value from a cJSON array at index i, defaulting to 0. */
static double json_array_double(cJSON *arr, int i) {
  cJSON *ci = cJSON_GetArrayItem(arr, i);
  return ci ? ci->valuedouble : 0;
}

static int json_array_int(cJSON *arr, int i) {
  cJSON *ci = cJSON_GetArrayItem(arr, i);
  return ci ? ci->valueint : 0;
}

/* Bundles the per-metric arrays for a single hourly response. */
typedef struct {
  cJSON *times;
  cJSON *temps;
  cJSON *codes;
  cJSON *winds;
  cJSON *precs;
  cJSON *humids;
} hourly_arrays_t;

/* Aggregate one matching hour entry into the appropriate section. */
static void process_hour_entry(const hourly_arrays_t *a, int i,
                               today_detail_t *detail) {
  cJSON *t = cJSON_GetArrayItem(a->times, i);
  const char *time_str = t->valuestring;
  int hour = 0;
  const char *th = strchr(time_str, 'T');
  if (th)
    hour = (th[1] - '0') * 10 + (th[2] - '0');

  int sec = hour_to_section(hour);
  double temp = json_array_double(a->temps, i);
  int wcode = json_array_int(a->codes, i);
  double wind = json_array_double(a->winds, i);
  double precip = json_array_double(a->precs, i);
  int humid = (int)json_array_double(a->humids, i);

  aggregate_hour(&detail->sections[sec], temp, wcode, wind, precip, humid);
}

static int parse_hourly_for_day(cJSON *hourly, today_detail_t *detail,
                                int day_index) {
  hourly_arrays_t a = {
      .times = cJSON_GetObjectItem(hourly, "time"),
      .temps = cJSON_GetObjectItem(hourly, "temperature_2m"),
      .codes = cJSON_GetObjectItem(hourly, "weathercode"),
      .winds = cJSON_GetObjectItem(hourly, "windspeed_10m"),
      .precs = cJSON_GetObjectItem(hourly, "precipitation"),
      .humids = cJSON_GetObjectItem(hourly, "relativehumidity_2m"),
  };
  if (!a.times || !a.temps)
    return -1;

  char target_date[DATE_STR_MAX] = {0};
  if (find_nth_date(a.times, day_index, target_date, sizeof(target_date)) != 0)
    return -1;

  snprintf(detail->date, sizeof(detail->date), "%s", target_date);

  int n = cJSON_GetArraySize(a.times);
  for (int i = 0; i < n; i++) {
    cJSON *t = cJSON_GetArrayItem(a.times, i);
    if (!t || !t->valuestring)
      continue;
    if (strncmp(t->valuestring, target_date, 10) != 0)
      continue;
    process_hour_entry(&a, i, detail);
  }

  for (int i = 0; i < SECTION_COUNT; i++) {
    finalize_section(&detail->sections[i]);
  }
  return 0;
}

static int fetch_and_parse_hourly(const char *base_url, double latitude,
                                  double longitude, today_detail_t *today,
                                  today_detail_t *tomorrow, int force) {
  char url[WEATHER_URL_MAX];
  build_hourly_url(url, sizeof(url), base_url, latitude, longitude,
                   HOURLY_FORECAST_DAYS);

  http_buffer_t response;
  http_buffer_init(&response);

  if (http_get_with_cache(url, &response, force) != 0) {
    http_buffer_free(&response);
    return -1;
  }

  cJSON *json = cJSON_Parse(response.data);
  http_buffer_free(&response);
  if (!json)
    return -1;

  cJSON *hourly = cJSON_GetObjectItem(json, "hourly");
  if (!hourly) {
    cJSON_Delete(json);
    return -1;
  }

  static const char *labels[SECTION_COUNT] = {"Morning", "Afternoon", "Evening",
                                              "Night"};

  for (int i = 0; i < SECTION_COUNT; i++) {
    init_section(&today->sections[i], labels[i]);
    init_section(&tomorrow->sections[i], labels[i]);
  }

  int r1 = parse_hourly_for_day(hourly, today, 0);
  today->valid = (r1 == 0);

  int r2 = parse_hourly_for_day(hourly, tomorrow, 1);
  tomorrow->valid = (r2 == 0);

  cJSON_Delete(json);
  return (r1 == 0) ? 0 : -1;
}

int weather_fetch(double latitude, double longitude, int num_days,
                  forecast_t *forecast, int force) {
  return fetch_forecast(WEATHER_API_URL, latitude, longitude, num_days,
                        forecast, force);
}

int weather_fetch_dwd(double latitude, double longitude, int num_days,
                      forecast_t *forecast, int force) {
  return fetch_forecast(DWD_ICON_API_URL, latitude, longitude, num_days,
                        forecast, force);
}

int weather_fetch_hourly(double latitude, double longitude,
                         today_detail_t *today, today_detail_t *tomorrow,
                         int force) {
  return fetch_and_parse_hourly(WEATHER_API_URL, latitude, longitude, today,
                                tomorrow, force);
}

int weather_fetch_hourly_dwd(double latitude, double longitude,
                             today_detail_t *today, today_detail_t *tomorrow,
                             int force) {
  return fetch_and_parse_hourly(DWD_ICON_API_URL, latitude, longitude, today,
                                tomorrow, force);
}

int weather_has_cache(double latitude, double longitude) {
  char url[WEATHER_URL_MAX];
  build_weather_url(url, sizeof(url), WEATHER_API_URL, latitude, longitude,
                    FORECAST_9DAY);
  return cache_age_seconds(url) >= 0 ? 1 : 0;
}

long weather_cache_mtime(double latitude, double longitude) {
  char url[WEATHER_URL_MAX];
  build_weather_url(url, sizeof(url), WEATHER_API_URL, latitude, longitude,
                    FORECAST_9DAY);
  return cache_mtime(url);
}

const char *weather_code_to_string(int code) {
  if (code == WMO_CLEAR_SKY)
    return "Clear sky";
  if (code == WMO_MAINLY_CLEAR)
    return "Mainly clear";
  if (code == WMO_PARTLY_CLOUDY)
    return "Partly cloudy";
  if (code == WMO_OVERCAST)
    return "Overcast";
  if (code >= WMO_FOG_MIN && code <= WMO_FOG_MAX)
    return "Fog";
  if (code >= WMO_DRIZZLE_MIN && code <= WMO_DRIZZLE_MAX)
    return "Drizzle";
  if (code >= WMO_RAIN_MIN && code <= WMO_RAIN_MAX)
    return "Rain";
  if (code >= WMO_SNOW_MIN && code <= WMO_SNOW_MAX)
    return "Snow";
  if (code >= WMO_SHOWERS_MIN && code <= WMO_SHOWERS_MAX)
    return "Showers";
  if (code >= WMO_SNOW_SHOWERS_MIN && code <= WMO_SNOW_SHOWERS_MAX)
    return "Snow showers";
  if (code == WMO_THUNDERSTORM)
    return "Thunderstorm";
  if (code >= WMO_THUNDERSTORM_HAIL_MIN && code <= WMO_THUNDERSTORM_HAIL_MAX)
    return "Thunderstorm with hail";
  return "Unknown";
}

const char *weather_code_to_icon(int code) {
  if (code == WMO_CLEAR_SKY)
    return "☀";
  if (code <= WMO_PARTLY_CLOUDY)
    return "⛅";
  if (code == WMO_OVERCAST)
    return "☁";
  if (code >= WMO_FOG_MIN && code <= WMO_FOG_MAX)
    return "🌫";
  if (code >= WMO_DRIZZLE_MIN && code <= WMO_DRIZZLE_MAX)
    return "🌦";
  if (code >= WMO_RAIN_MIN && code <= WMO_RAIN_MAX)
    return "🌧";
  if (code >= WMO_SNOW_MIN && code <= WMO_SNOW_MAX)
    return "❄";
  if (code >= WMO_SHOWERS_MIN && code <= WMO_SHOWERS_MAX)
    return "🌧";
  if (code >= WMO_SNOW_SHOWERS_MIN && code <= WMO_SNOW_SHOWERS_MAX)
    return "🌨";
  if (code >= WMO_THUNDERSTORM)
    return "⛈";
  return "?";
}
