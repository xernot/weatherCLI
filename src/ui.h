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

#include <pthread.h>
#include <time.h>

/* Which pane is currently focused */
typedef enum { PANE_LEFT, PANE_RIGHT } active_pane_t;

/* What the left pane is currently showing */
typedef enum { LEFT_MODE_SAVED, LEFT_MODE_SEARCH } left_mode_t;

/* Right pane tabs */
typedef enum { TAB_TODAY, TAB_TOMORROW, TAB_4DAY, TAB_9DAY } right_tab_t;

/* Full application UI state */
typedef struct {
  /* Layout */
  int term_rows;
  int term_cols;
  int left_width;

  /* Pane focus */
  active_pane_t active_pane;

  /* Left pane state */
  left_mode_t left_mode;
  char search_input[SEARCH_INPUT_MAX];
  int search_len;
  int cursor_index;  /* Selected item index in current list */
  int scroll_offset; /* Scroll position */

  /* Location data */
  location_list_t saved;          /* Saved locations */
  location_list_t search_results; /* API search results */
  location_list_t filtered;       /* Filtered saved locations */

  /* Right pane state — tabbed: Today, Tomorrow, 4-Day, 9-Day */
  right_tab_t right_tab;
  forecast_t forecast;             /* ECMWF forecast */
  forecast_t forecast_dwd;         /* DWD ICON forecast */
  today_detail_t today_detail;     /* ECMWF hourly sections for Today */
  today_detail_t tomorrow_detail;  /* ECMWF hourly sections for Tomorrow */
  today_detail_t today_detail_dwd; /* DWD ICON hourly sections for Today */
  today_detail_t
      tomorrow_detail_dwd; /* DWD ICON hourly sections for Tomorrow */
  char gpt_summaries[RIGHT_TAB_COUNT][2048]; /* Per-tab GPT summaries */
  int gpt_loaded[RIGHT_TAB_COUNT];           /* Whether summary is fetched */
  activity_list_t activities;                /* Loaded activity modes */
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

  /* Async GPT summary */
  pthread_t gpt_thread;           /* Background thread handle */
  int gpt_thread_active;          /* Whether a GPT thread is running */
  int gpt_thread_tab;             /* Which tab the thread is fetching for */
  char gpt_thread_result[2048];   /* Buffer for thread result */
  int gpt_thread_done;            /* Set by thread when complete */
  forecast_t gpt_thread_forecast; /* Snapshot of forecast for thread */
  char gpt_thread_location[MAX_LOCATION_NAME]; /* Location name for thread */
  char gpt_thread_prompt[512];                 /* Activity prompt for thread */

  /* Local day-of-year at last tick — used to detect midnight rollover */
  int last_yday;

  /* Running flag */
  int running;
} app_state_t;

/* Initialize ncurses and UI state */
void ui_init(app_state_t *state);

/* Clean up ncurses */
void ui_cleanup(void);

/* Main render function: draws both panes */
void ui_render(const app_state_t *state);

/* Handle a single keypress, updating state. Returns 0 to continue, 1 to quit.
 */
int ui_handle_input(app_state_t *state, int ch);

/* Check if async GPT thread has completed and collect result */
void ui_check_async_gpt(app_state_t *state);

/* Detect local-date rollover; on change, force-refresh the displayed forecast
 * so "Today" reflects the new calendar day. */
void ui_check_date_rollover(app_state_t *state);

/* Set a status bar message */
void ui_set_status(app_state_t *state, const char *fmt, ...);

/* Calculate left pane width from terminal width */
int ui_calc_left_width(int term_cols);

#endif /* UI_H */
