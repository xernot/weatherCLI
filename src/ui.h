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

#ifndef UI_H
#define UI_H

#include "activity.h"
#include "config.h"
#include "location.h"
#include "weather.h"

#include <time.h>

/* Which pane is currently focused */
typedef enum { PANE_LEFT, PANE_RIGHT } ActivePane;

/* What the left pane is currently showing */
typedef enum { LEFT_MODE_SAVED, LEFT_MODE_SEARCH } LeftMode;

/* Right pane tabs */
typedef enum { TAB_TODAY, TAB_TOMORROW, TAB_4DAY, TAB_9DAY } RightTab;

/* Full application UI state */
typedef struct {
  /* Layout */
  int term_rows;
  int term_cols;
  int left_width;

  /* Pane focus */
  ActivePane active_pane;

  /* Left pane state */
  LeftMode left_mode;
  char search_input[SEARCH_INPUT_MAX];
  int search_len;
  int cursor_index;  /* Selected item index in current list */
  int scroll_offset; /* Scroll position */

  /* Location data */
  LocationList saved;          /* Saved locations */
  LocationList search_results; /* API search results */
  LocationList filtered;       /* Filtered saved locations */

  /* Right pane state — tabbed: Today, Tomorrow, 4-Day, 9-Day */
  RightTab right_tab;
  Forecast forecast;               /* ECMWF forecast */
  Forecast forecast_dwd;           /* DWD ICON forecast */
  TodayDetail today_detail;        /* ECMWF hourly sections for Today */
  TodayDetail tomorrow_detail;     /* ECMWF hourly sections for Tomorrow */
  TodayDetail today_detail_dwd;    /* DWD ICON hourly sections for Today */
  TodayDetail tomorrow_detail_dwd; /* DWD ICON hourly sections for Tomorrow */
  char gpt_summaries[RIGHT_TAB_COUNT][2048]; /* Per-tab GPT summaries */
  int gpt_loaded[RIGHT_TAB_COUNT];           /* Whether summary is fetched */
  ActivityList activities;                   /* Loaded activity modes */
  int activity_index;                        /* Current activity mode index */

  /* Chat mode */
  int chat_active;                 /* Whether chat input is open */
  char chat_input[CHAT_INPUT_MAX]; /* Current chat question */
  int chat_input_len;
  char chat_response[CHAT_RESPONSE_MAX]; /* Last chat response */
  int chat_has_response;                 /* Whether a response is displayed */

  int has_forecast;    /* Whether forecast data is loaded */
  time_t last_refresh; /* Timestamp of last weather data fetch */
  int loading;         /* Whether a request is in progress */
  int right_scroll;    /* Scroll position for right pane */

  /* Loading progress (0 = not loading, 1-100 = percent) */
  int progress;
  char loading_location[MAX_LOCATION_NAME]; /* Location being loaded */

  /* Status bar message */
  char status_msg[256];
  time_t status_expire; /* When nonzero, clear status_msg after this time */

  /* Running flag */
  int running;
} AppState;

/* Initialize ncurses and UI state */
void ui_init(AppState *state);

/* Clean up ncurses */
void ui_cleanup(void);

/* Main render function: draws both panes */
void ui_render(const AppState *state);

/* Handle a single keypress, updating state. Returns 0 to continue, 1 to quit.
 */
int ui_handle_input(AppState *state, int ch);

/* Set a status bar message */
void ui_set_status(AppState *state, const char *fmt, ...);

/* Calculate left pane width from terminal width */
int ui_calc_left_width(int term_cols);

#endif /* UI_H */
