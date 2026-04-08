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

#ifndef WEATHER_H
#define WEATHER_H

#include "config.h"

/* Weather data for a single day */
typedef struct {
  char date[16];         /* YYYY-MM-DD */
  double temp_max;       /* Maximum temperature in Celsius */
  double temp_min;       /* Minimum temperature in Celsius */
  double precipitation;  /* Total precipitation in mm */
  int weather_code;      /* WMO weather code */
  double wind_speed_max; /* Maximum wind speed in km/h */
  int humidity;          /* Average relative humidity % */
  double uv_index;       /* Maximum UV index */
} day_weather_t;

/* forecast_t containing multiple days */
typedef struct {
  day_weather_t days[FORECAST_9DAY];
  int num_days;
  char timezone[64];
} forecast_t;

/* Aggregated weather for a time-of-day section (e.g. morning, afternoon) */
typedef struct {
  const char *label; /* "Morning", "Afternoon", "Evening", "Night" */
  double temp_max;
  double temp_min;
  double precipitation; /* Total precipitation in mm */
  double wind_speed_max;
  int weather_code; /* Dominant (worst) WMO code */
  int humidity;     /* Average humidity % */
  int valid;        /* Whether this section has data */
} hourly_section_t;

/* Today's detailed forecast broken into time-of-day sections */
typedef struct {
  hourly_section_t
      sections[SECTION_COUNT]; /* Morning, Afternoon, Evening, Night */
  char date[16];
  int valid;
} today_detail_t;

/* Fetch ECMWF forecast for given coordinates. Returns 0 on success, -1 on
 * error. */
int weather_fetch(double latitude, double longitude, int num_days,
                  forecast_t *forecast, int force);

/* Fetch DWD ICON forecast for given coordinates. Returns 0 on success, -1 on
 * error. */
int weather_fetch_dwd(double latitude, double longitude, int num_days,
                      forecast_t *forecast, int force);

/* Fetch ECMWF hourly data for today and tomorrow, aggregate into sections.
   Returns 0 on success, -1 on error. */
int weather_fetch_hourly(double latitude, double longitude, today_detail_t *today,
                         today_detail_t *tomorrow, int force);

/* Fetch DWD ICON hourly data for today and tomorrow, aggregate into sections.
   Returns 0 on success, -1 on error. */
int weather_fetch_hourly_dwd(double latitude, double longitude,
                             today_detail_t *today, today_detail_t *tomorrow,
                             int force);

/* Return 1 if a fresh-or-stale cached daily forecast exists for the given
 * coordinates, 0 otherwise. */
int weather_has_cache(double latitude, double longitude);

/* Return the mtime of the cached ECMWF daily forecast for the given
 * coordinates, or 0 if no cache entry exists. */
long weather_cache_mtime(double latitude, double longitude);

/* Get a human-readable weather condition string from a WMO code. */
const char *weather_code_to_string(int code);

/* Get a weather icon string from a WMO code. */
const char *weather_code_to_icon(int code);

#endif /* WEATHER_H */
