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
#include "http.h"

#include <stdio.h>
#include <string.h>

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

static int parse_day(cJSON *daily, int index, DayWeather *day) {
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

static int fetch_forecast(const char *base_url, double latitude,
                          double longitude, int num_days, Forecast *forecast) {
  char url[1024];
  build_weather_url(url, sizeof(url), base_url, latitude, longitude, num_days);

  HttpBuffer response;
  http_buffer_init(&response);

  if (http_get(url, &response) != 0) {
    http_buffer_free(&response);
    return -1;
  }

  cJSON *json = cJSON_Parse(response.data);
  http_buffer_free(&response);
  if (!json) {
    return -1;
  }

  cJSON *daily = cJSON_GetObjectItem(json, "daily");
  cJSON *tz = cJSON_GetObjectItem(json, "timezone");

  if (!daily) {
    cJSON_Delete(json);
    return -1;
  }

  if (tz && tz->valuestring) {
    snprintf(forecast->timezone, sizeof(forecast->timezone), "%s",
             tz->valuestring);
  }

  cJSON *dates = cJSON_GetObjectItem(daily, "time");
  int n = cJSON_GetArraySize(dates);
  if (n > FORECAST_9DAY) {
    n = FORECAST_9DAY;
  }

  forecast->num_days = n;
  for (int i = 0; i < n; i++) {
    parse_day(daily, i, &forecast->days[i]);
  }

  cJSON_Delete(json);
  return 0;
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

static void init_section(HourlySection *s, const char *label) {
  s->label = label;
  s->temp_max = -999;
  s->temp_min = 999;
  s->precipitation = 0;
  s->wind_speed_max = 0;
  s->weather_code = 0;
  s->humidity = 0;
  s->valid = 0;
}

static void aggregate_hour(HourlySection *s, double temp, int wcode,
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

static void finalize_section(HourlySection *s) {
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

static int parse_hourly_for_day(cJSON *hourly, TodayDetail *detail,
                                int day_index) {
  cJSON *times = cJSON_GetObjectItem(hourly, "time");
  cJSON *temps = cJSON_GetObjectItem(hourly, "temperature_2m");
  cJSON *codes = cJSON_GetObjectItem(hourly, "weathercode");
  cJSON *winds = cJSON_GetObjectItem(hourly, "windspeed_10m");
  cJSON *precs = cJSON_GetObjectItem(hourly, "precipitation");
  cJSON *humids = cJSON_GetObjectItem(hourly, "relativehumidity_2m");

  if (!times || !temps)
    return -1;

  /* Find the date string for the target day */
  char target_date[16] = {0};
  int n = cJSON_GetArraySize(times);
  int dates_seen = -1;
  char prev_date[16] = {0};

  for (int i = 0; i < n; i++) {
    cJSON *t = cJSON_GetArrayItem(times, i);
    if (!t || !t->valuestring)
      continue;
    char cur_date[16];
    snprintf(cur_date, sizeof(cur_date), "%.10s", t->valuestring);
    if (strcmp(cur_date, prev_date) != 0) {
      dates_seen++;
      snprintf(prev_date, sizeof(prev_date), "%s", cur_date);
      if (dates_seen == day_index) {
        snprintf(target_date, sizeof(target_date), "%s", cur_date);
        break;
      }
    }
  }

  if (target_date[0] == '\0')
    return -1;

  snprintf(detail->date, sizeof(detail->date), "%s", target_date);

  for (int i = 0; i < n; i++) {
    cJSON *t = cJSON_GetArrayItem(times, i);
    if (!t || !t->valuestring)
      continue;

    const char *time_str = t->valuestring;

    /* Skip entries not matching target date */
    if (strncmp(time_str, target_date, 10) != 0)
      continue;

    int hour = 0;
    const char *th = strchr(time_str, 'T');
    if (th)
      hour = (th[1] - '0') * 10 + (th[2] - '0');

    int sec = hour_to_section(hour);
    cJSON *ci;

    double temp = 0;
    ci = cJSON_GetArrayItem(temps, i);
    if (ci)
      temp = ci->valuedouble;

    int wcode = 0;
    ci = cJSON_GetArrayItem(codes, i);
    if (ci)
      wcode = ci->valueint;

    double wind = 0;
    ci = cJSON_GetArrayItem(winds, i);
    if (ci)
      wind = ci->valuedouble;

    double precip = 0;
    ci = cJSON_GetArrayItem(precs, i);
    if (ci)
      precip = ci->valuedouble;

    int humid = 0;
    ci = cJSON_GetArrayItem(humids, i);
    if (ci)
      humid = (int)ci->valuedouble;

    aggregate_hour(&detail->sections[sec], temp, wcode, wind, precip, humid);
  }

  for (int i = 0; i < SECTION_COUNT; i++) {
    finalize_section(&detail->sections[i]);
  }
  return 0;
}

static int fetch_and_parse_hourly(const char *base_url, double latitude,
                                  double longitude, TodayDetail *today,
                                  TodayDetail *tomorrow) {
  char url[1024];
  build_hourly_url(url, sizeof(url), base_url, latitude, longitude,
                   HOURLY_FORECAST_DAYS);

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
                  Forecast *forecast) {
  return fetch_forecast(WEATHER_API_URL, latitude, longitude, num_days,
                        forecast);
}

int weather_fetch_dwd(double latitude, double longitude, int num_days,
                      Forecast *forecast) {
  return fetch_forecast(DWD_ICON_API_URL, latitude, longitude, num_days,
                        forecast);
}

int weather_fetch_hourly(double latitude, double longitude, TodayDetail *today,
                         TodayDetail *tomorrow) {
  return fetch_and_parse_hourly(WEATHER_API_URL, latitude, longitude, today,
                                tomorrow);
}

int weather_fetch_hourly_dwd(double latitude, double longitude,
                             TodayDetail *today, TodayDetail *tomorrow) {
  return fetch_and_parse_hourly(DWD_ICON_API_URL, latitude, longitude, today,
                                tomorrow);
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
