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
} DayWeather;

/* Forecast containing multiple days */
typedef struct {
  DayWeather days[FORECAST_9DAY];
  int num_days;
  char timezone[64];
} Forecast;

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
} HourlySection;

/* Today's detailed forecast broken into time-of-day sections */
typedef struct {
  HourlySection
      sections[SECTION_COUNT]; /* Morning, Afternoon, Evening, Night */
  char date[16];
  int valid;
} TodayDetail;

/* Fetch ECMWF forecast for given coordinates. Returns 0 on success, -1 on
 * error. */
int weather_fetch(double latitude, double longitude, int num_days,
                  Forecast *forecast);

/* Fetch DWD ICON forecast for given coordinates. Returns 0 on success, -1 on
 * error. */
int weather_fetch_dwd(double latitude, double longitude, int num_days,
                      Forecast *forecast);

/* Fetch ECMWF hourly data for today and tomorrow, aggregate into sections.
   Returns 0 on success, -1 on error. */
int weather_fetch_hourly(double latitude, double longitude, TodayDetail *today,
                         TodayDetail *tomorrow);

/* Fetch DWD ICON hourly data for today and tomorrow, aggregate into sections.
   Returns 0 on success, -1 on error. */
int weather_fetch_hourly_dwd(double latitude, double longitude,
                             TodayDetail *today, TodayDetail *tomorrow);

/* Get a human-readable weather condition string from a WMO code. */
const char *weather_code_to_string(int code);

/* Get a weather icon string from a WMO code. */
const char *weather_code_to_icon(int code);

#endif /* WEATHER_H */
