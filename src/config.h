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

#ifndef CONFIG_H
#define CONFIG_H

/* --- API Endpoints --- */

/* Open-Meteo geocoding API (free, no key required) */
#define GEOCODING_API_URL "https://geocoding-api.open-meteo.com/v1/search"

/* Open-Meteo ECMWF model API (free, no key required) */
#define WEATHER_API_URL "https://api.open-meteo.com/v1/ecmwf"

/* Open-Meteo DWD ICON model API (free, no key required) */
#define DWD_ICON_API_URL "https://api.open-meteo.com/v1/dwd-icon"

/* Nominatim (OpenStreetMap) geocoding API — fallback for fuzzy search */
#define NOMINATIM_API_URL "https://nominatim.openstreetmap.org/search"

/* OpenAI chat completions endpoint for GPT summaries */
#define OPENAI_API_URL "https://api.openai.com/v1/chat/completions"

/* --- API Parameters --- */

/* Maximum number of geocoding results returned per search */
#define GEOCODING_MAX_RESULTS 10

/* OpenAI model used for weather summarization */
#define GPT_MODEL "gpt-5.4"

/* Maximum tokens for GPT response */
#define GPT_MAX_TOKENS 512

/* GPT sampling temperature (0.0 = deterministic, 1.0 = creative) */
#define GPT_TEMPERATURE 0.7

/* Environment variable name for OpenAI API key */
#define OPENAI_API_KEY_ENV "OPENAI_API_KEY"

/* --- Terminal --- */

/* Application version string */
#define APP_VERSION "weathercli v0.1"

/* Footer hint bar text */
#define FOOTER_HINT_TEXT                                                       \
  "S-Tab:pane  a:activity  c:chat  r:refresh  s:save  x:del  ?:info  q:quit"

/* Terminal window title */
#define TERMINAL_TITLE "WeatherCLI"

/* getch() timeout in milliseconds for the main event loop */
#define GETCH_TIMEOUT_MS 100

/* How long temporary status messages (e.g. "Refreshed") stay visible */
#define STATUS_DISPLAY_SECONDS 3

/* Weather source labels shown in the UI */
#define SOURCE_LABEL_ECMWF "ECMWF"
#define SOURCE_LABEL_DWD "DWD ICON"

/* strftime format for the last-refresh timestamp shown in the header */
#define REFRESH_TIME_FORMAT "[%H:%M:%S]"

/* Marker shown next to a location that has cached weather data on disk */
#define CACHED_LOCATION_MARK "*"
#define UNCACHED_LOCATION_MARK "-"

/* --- Forecast Modes --- */

/* Number of days for each forecast mode */
#define FORECAST_DAILY 1
#define FORECAST_4DAY 4
#define FORECAST_9DAY 9

/* Number of days to fetch for hourly detail (today + tomorrow) */
#define HOURLY_FORECAST_DAYS 2

/* Time-of-day sections for the Today tab */
#define SECTION_COUNT 4
#define SECTION_MORNING_START 6
#define SECTION_MORNING_END 12
#define SECTION_AFTERNOON_START 12
#define SECTION_AFTERNOON_END 18
#define SECTION_EVENING_START 18
#define SECTION_EVENING_END 24
#define SECTION_NIGHT_START 0
#define SECTION_NIGHT_END 6

/* --- Activity Modes --- */
/* Activities are loaded from ~/.weathercli/activities.conf at startup.
   File format: one activity per line, label and prompt separated by |
   Example: Cycling|Summarize for a bike ride. Focus on wind and rain.
   Falls back to built-in defaults if file is missing. */

/* Maximum number of activity modes */
#define MAX_ACTIVITY_COUNT 10

/* Maximum length of an activity label */
#define MAX_ACTIVITY_LABEL 32

/* Maximum length of an activity GPT prompt */
#define MAX_ACTIVITY_PROMPT 256

/* Path to activities config file (relative to $HOME) */
#define ACTIVITIES_FILE ".weathercli/activities.conf"

/* Built-in default activity labels */
#define DEFAULT_ACTIVITY_LABEL_GENERAL "General"
#define DEFAULT_ACTIVITY_LABEL_BIKE "Motorbike"
#define DEFAULT_ACTIVITY_LABEL_SKI "Ski"
#define DEFAULT_ACTIVITY_LABEL_SAIL "Sailing"

/* Built-in default activity prompts */
#define DEFAULT_ACTIVITY_PROMPT_GENERAL "Give a general weather summary."

#define DEFAULT_ACTIVITY_PROMPT_BIKE                                           \
  "Summarize for a motorbike tour. Focus on rain risk, road "                  \
  "grip conditions, wind gusts, visibility, and comfortable "                  \
  "riding temperature ranges."

#define DEFAULT_ACTIVITY_PROMPT_SKI                                            \
  "Summarize for ski touring or skiing. Focus on snowfall, "                   \
  "freezing level, avalanche-relevant wind/temperature swings, "               \
  "visibility, and snow quality expectations."

#define DEFAULT_ACTIVITY_PROMPT_SAIL                                           \
  "Summarize for sailing. Focus on wind speed and direction, "                 \
  "gusts, wave-relevant precipitation, visibility, and storm risk."

/* --- Chat --- */

/* Maximum length of a chat question */
#define CHAT_INPUT_MAX 256

/* Maximum length of a chat response */
#define CHAT_RESPONSE_MAX 4096

/* --- Storage --- */

/* Path to saved locations file (relative to $HOME) */
#define LOCATIONS_FILE ".weathercli/locations.json"

/* Path to secrets file (relative to $HOME, one KEY=value per line) */
#define SECRETS_FILE ".weathercli/secrets"

/* Maximum number of saved locations */
#define MAX_SAVED_LOCATIONS 50

/* Directory for cached HTTP responses (relative to $HOME) */
#define CACHE_DIR ".weathercli/cache"

/* How often the background thread refreshes cached forecasts (seconds) */
#define CACHE_REFRESH_INTERVAL_SECONDS (6 * 3600)

/* Maximum age of a cached response before it is considered stale and a
 * fresh network fetch is required. */
#define CACHE_MAX_AGE_SECONDS (6 * 3600)

/* Maximum length of a location name */
#define MAX_LOCATION_NAME 128

/* Maximum length of a country code */
#define MAX_COUNTRY_CODE 8

/* --- UI Layout --- */

/* Left pane width as fraction of terminal width (0.0 - 1.0) */
#define LEFT_PANE_RATIO 0.30

/* Minimum left pane width in columns */
#define LEFT_PANE_MIN_WIDTH 25

/* Maximum left pane width in columns */
#define LEFT_PANE_MAX_WIDTH 40

/* Maximum length of the search input field */
#define SEARCH_INPUT_MAX 64

/* Number of header rows in left pane (top line + title + search + bottom line)
 */
#define LEFT_PANE_HEADER_ROWS 5

/* Row offsets within the header */
#define HEADER_TOP_LINE 0
#define HEADER_TITLE_ROW 1
#define HEADER_SEARCH_ROW 2
#define HEADER_BOTTOM_LINE 3

/* Footer height (top line + content + bottom line) */
#define FOOTER_HEIGHT 3

/* Scroll padding: lines to keep visible above/below cursor */
#define SCROLL_PADDING 2

/* --- HTTP --- */

/* HTTP request timeout in seconds */
#define HTTP_TIMEOUT_SECONDS 30

/* Maximum HTTP response buffer size in bytes (1 MB) */
#define HTTP_MAX_RESPONSE_SIZE (1024 * 1024)

/* --- Key Bindings --- */

#define KEY_ENTER_CODE 10
#define KEY_ESCAPE_CODE 27
#define KEY_BACKSPACE_CODE 127
#define KEY_QUIT 'q'
#define KEY_DELETE_LOCATION 'x'
#define KEY_SAVE_LOCATION 's'
#define KEY_RELOAD 'r'
#define KEY_ACTIVITY 'a'
#define KEY_INFO '?'

/* Number of tabs in right pane */
#define RIGHT_TAB_COUNT 4

/* --- Colors --- */

/* ncurses color pair IDs */
#define COLOR_PAIR_TITLE 1
#define COLOR_PAIR_SELECTED 2
#define COLOR_PAIR_SEARCH 3
#define COLOR_PAIR_BORDER 4
#define COLOR_PAIR_TEMP_HIGH 5
#define COLOR_PAIR_TEMP_LOW 6
#define COLOR_PAIR_HEADER 7
#define COLOR_PAIR_STATUS 8
#define COLOR_PAIR_HELP 9
#define COLOR_PAIR_TAB_ACTIVE 10
#define COLOR_PAIR_TAB_INACTIVE 11

/* --- Weather Display --- */

/* Weather condition code to icon mapping thresholds (WMO codes) */
#define WMO_CLEAR_SKY 0
#define WMO_MAINLY_CLEAR 1
#define WMO_PARTLY_CLOUDY 2
#define WMO_OVERCAST 3
#define WMO_FOG_MIN 45
#define WMO_FOG_MAX 48
#define WMO_DRIZZLE_MIN 51
#define WMO_DRIZZLE_MAX 57
#define WMO_RAIN_MIN 61
#define WMO_RAIN_MAX 67
#define WMO_SNOW_MIN 71
#define WMO_SNOW_MAX 77
#define WMO_SHOWERS_MIN 80
#define WMO_SHOWERS_MAX 82
#define WMO_SNOW_SHOWERS_MIN 85
#define WMO_SNOW_SHOWERS_MAX 86
#define WMO_THUNDERSTORM 95
#define WMO_THUNDERSTORM_HAIL_MIN 96
#define WMO_THUNDERSTORM_HAIL_MAX 99

#endif /* CONFIG_H */
