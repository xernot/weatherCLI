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

#include "ui.h"
#include "config.h"
#include "gpt.h"
#include "location.h"
#include "weather.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int ui_calc_left_width(int term_cols) {
  int w = (int)(term_cols * LEFT_PANE_RATIO);
  if (w < LEFT_PANE_MIN_WIDTH)
    w = LEFT_PANE_MIN_WIDTH;
  if (w > LEFT_PANE_MAX_WIDTH)
    w = LEFT_PANE_MAX_WIDTH;
  if (w >= term_cols)
    w = term_cols / 3;
  return w;
}

static void init_color_pairs(void) {
  if (!has_colors())
    return;
  start_color();
  use_default_colors();
  init_pair(COLOR_PAIR_TITLE, COLOR_YELLOW, -1);
  init_pair(COLOR_PAIR_SELECTED, COLOR_BLACK, COLOR_WHITE);
  init_pair(COLOR_PAIR_SEARCH, COLOR_CYAN, -1);
  init_pair(COLOR_PAIR_BORDER, COLOR_WHITE, -1);
  init_pair(COLOR_PAIR_TEMP_HIGH, COLOR_RED, -1);
  init_pair(COLOR_PAIR_TEMP_LOW, COLOR_BLUE, -1);
  init_pair(COLOR_PAIR_HEADER, COLOR_GREEN, -1);
  init_pair(COLOR_PAIR_STATUS, COLOR_BLACK, COLOR_YELLOW);
  init_pair(COLOR_PAIR_HELP, COLOR_WHITE, -1);
  init_pair(COLOR_PAIR_TAB_ACTIVE, COLOR_GREEN, -1);
  init_pair(COLOR_PAIR_TAB_INACTIVE, COLOR_WHITE, -1);
}

static void init_state_defaults(app_state_t *state) {
  state->active_pane = PANE_LEFT;
  state->left_mode = LEFT_MODE_SAVED;
  state->search_input[0] = '\0';
  state->search_len = 0;
  state->cursor_index = 0;
  state->scroll_offset = 0;
  state->right_tab = TAB_TODAY;
  state->activity_index = 0;
  state->chat_active = 0;
  state->chat_input[0] = '\0';
  state->chat_input_len = 0;
  state->chat_response[0] = '\0';
  state->chat_has_response = 0;
  state->has_forecast = 0;
  state->loading = 0;
  state->progress = 0;
  state->loading_location[0] = '\0';
  state->right_scroll = 0;
  state->status_msg[0] = '\0';
  for (int i = 0; i < RIGHT_TAB_COUNT; i++) {
    state->gpt_summaries[i][0] = '\0';
    state->gpt_loaded[i] = 0;
  }
  state->running = 1;
  state->search_results.count = 0;
  state->filtered.count = 0;
}

void ui_init(app_state_t *state) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(GETCH_TIMEOUT_MS);

  /* Set terminal window title (write directly to terminal, bypassing ncurses)
   */
  fprintf(stderr, "\033]0;%s\007", TERMINAL_TITLE);

  init_color_pairs();

  getmaxyx(stdscr, state->term_rows, state->term_cols);
  state->left_width = ui_calc_left_width(state->term_cols);
  init_state_defaults(state);

  location_load(&state->saved);
  activity_load(&state->activities);

  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  state->last_yday = lt ? lt->tm_yday : -1;
}

void ui_cleanup(void) { endwin(); }

void ui_set_status(app_state_t *state, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(state->status_msg, sizeof(state->status_msg), fmt, args);
  va_end(args);
}

static void draw_border(int left_width, int rows) {
  attron(COLOR_PAIR(COLOR_PAIR_BORDER));
  for (int y = 0; y < rows - FOOTER_HEIGHT; y++) {
    mvaddch(y, left_width, ACS_VLINE);
  }
  attroff(COLOR_PAIR(COLOR_PAIR_BORDER));
}

static void draw_header_lines(const app_state_t *state) {
  attron(COLOR_PAIR(COLOR_PAIR_BORDER));
  mvhline(HEADER_TOP_LINE, 0, ACS_HLINE, state->term_cols);
  attroff(COLOR_PAIR(COLOR_PAIR_BORDER));
}

static void draw_left_title(const app_state_t *state) {
  const char *title =
      (state->left_mode == LEFT_MODE_SEARCH) ? "Search Results" : "Locations";

  if (state->active_pane == PANE_LEFT) {
    attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvprintw(HEADER_TITLE_ROW, 1, "> %s", title);
    attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  } else {
    attron(A_DIM);
    mvprintw(HEADER_TITLE_ROW, 1, "  %s", title);
    attroff(A_DIM);
  }
}

static void draw_search_bar(const app_state_t *state) {
  attron(COLOR_PAIR(COLOR_PAIR_SEARCH));
  mvprintw(HEADER_SEARCH_ROW, 1, "/");
  printw("%-*s", state->left_width - 3, state->search_input);
  attroff(COLOR_PAIR(COLOR_PAIR_SEARCH));
  if (state->active_pane == PANE_LEFT) {
    attron(COLOR_PAIR(COLOR_PAIR_HEADER));
  }
  mvhline(HEADER_BOTTOM_LINE, 0, ACS_HLINE, state->left_width);
  if (state->active_pane == PANE_LEFT) {
    attroff(COLOR_PAIR(COLOR_PAIR_HEADER));
  }
}

static location_list_t *get_visible_list(app_state_t *state) {
  if (state->left_mode == LEFT_MODE_SEARCH) {
    return &state->search_results;
  }
  if (state->search_len > 0) {
    return &state->filtered;
  }
  return &state->saved;
}

static void draw_location_item(int y, int x, int width, const location_t *loc,
                               int selected) {
  if (selected) {
    attron(COLOR_PAIR(COLOR_PAIR_SELECTED));
  }

  const char *mark = weather_has_cache(loc->latitude, loc->longitude)
                         ? CACHED_LOCATION_MARK
                         : UNCACHED_LOCATION_MARK;

  char line[128];
  snprintf(line, sizeof(line), " %s %s %s", mark, loc->name, loc->country);

  mvprintw(y, x, "%-*.*s", width - 1, width - 1, line);

  if (selected) {
    attroff(COLOR_PAIR(COLOR_PAIR_SELECTED));
  }
}

static void draw_left_list(app_state_t *state) {
  location_list_t *list = get_visible_list(state);
  int max_visible = state->term_rows - LEFT_PANE_HEADER_ROWS - FOOTER_HEIGHT;
  int start_y = LEFT_PANE_HEADER_ROWS - 1;

  if (list->count == 0) {
    attron(A_DIM);
    if (state->left_mode == LEFT_MODE_SEARCH) {
      mvprintw(start_y, 2, "No results");
    } else if (state->search_len > 0) {
      mvprintw(start_y, 2, "No matches");
    } else {
      mvprintw(start_y, 2, "No saved locations");
      mvprintw(start_y + 1, 2, "Type to search");
    }
    attroff(A_DIM);
    return;
  }

  for (int i = 0; i < max_visible && (i + state->scroll_offset) < list->count;
       i++) {
    int idx = i + state->scroll_offset;
    int selected =
        (idx == state->cursor_index && state->active_pane == PANE_LEFT);
    draw_location_item(start_y + i, 0, state->left_width, &list->items[idx],
                       selected);
  }
}

static void draw_progress_bar(int y, int x, int width, int percent) {
  int filled = (width * percent) / 100;

  attron(COLOR_PAIR(COLOR_PAIR_HEADER));
  for (int i = 0; i < filled; i++) {
    mvaddch(y, x + i, ACS_BLOCK);
  }
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER));

  attron(A_DIM);
  for (int i = filled; i < width; i++) {
    mvaddch(y, x + i, ACS_HLINE);
  }
  attroff(A_DIM);
}

static void draw_footer_content(const app_state_t *state, int mid) {
  if (state->chat_active) {
    attron(COLOR_PAIR(COLOR_PAIR_SEARCH));
    mvprintw(mid, 1, "Ask: %s_", state->chat_input);
    attroff(COLOR_PAIR(COLOR_PAIR_SEARCH));
    return;
  }
  if (state->progress > 0) {
    mvprintw(mid, 1, "%s ", state->status_msg);
    int text_len = strlen(state->status_msg) + 2;
    int bar_width = state->term_cols - text_len - 8;
    if (bar_width > 10) {
      draw_progress_bar(mid, text_len, bar_width, state->progress);
      mvprintw(mid, text_len + bar_width + 1, "%3d%%", state->progress);
    }
    return;
  }
  if (state->status_msg[0]) {
    mvprintw(mid, 1, "%s", state->status_msg);
    return;
  }
  attron(A_DIM);
  mvprintw(mid, 1, FOOTER_HINT_TEXT);
  attroff(A_DIM);
}

static void draw_footer(const app_state_t *state) {
  int top = state->term_rows - FOOTER_HEIGHT;
  int mid = top + 1;
  int bot = top + 2;

  attron(COLOR_PAIR(COLOR_PAIR_BORDER));
  mvhline(top, 0, ACS_HLINE, state->term_cols);
  attroff(COLOR_PAIR(COLOR_PAIR_BORDER));

  mvhline(mid, 0, ' ', state->term_cols);
  draw_footer_content(state, mid);

  attron(A_DIM);
  mvprintw(mid, state->term_cols - (int)strlen(APP_VERSION) - 1, "%s",
           APP_VERSION);
  attroff(A_DIM);

  attron(COLOR_PAIR(COLOR_PAIR_BORDER));
  mvhline(bot, 0, ACS_HLINE, state->term_cols);
  attroff(COLOR_PAIR(COLOR_PAIR_BORDER));
}

static void format_refresh_time(time_t ts, char *out, size_t out_size) {
  struct tm *lt = localtime(&ts);
  if (lt) {
    strftime(out, out_size, REFRESH_TIME_FORMAT, lt);
  } else {
    out[0] = '\0';
  }
}

static void draw_location_header(const app_state_t *state, int x) {
  const char *name = NULL;
  char buf[256];

  if (state->loading_location[0]) {
    name = state->loading_location;
  } else {
    location_list_t *list = get_visible_list((app_state_t *)state);
    if (state->cursor_index >= 0 && state->cursor_index < list->count) {
      const location_t *loc = &list->items[state->cursor_index];
      snprintf(buf, sizeof(buf), "%s, %s", loc->name, loc->country);
      name = buf;
    }
  }

  if (!name)
    return;

  int col = 0;
  if (state->active_pane == PANE_RIGHT) {
    attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvprintw(HEADER_TITLE_ROW, x + 2, "> %s", name);
    col = x + 2 + 2 + (int)strlen(name);
    attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  } else {
    attron(A_DIM);
    mvprintw(HEADER_TITLE_ROW, x + 2, "  %s", name);
    col = x + 2 + 2 + (int)strlen(name);
    attroff(A_DIM);
  }

  if (state->last_refresh > 0) {
    char time_buf[TIME_BUF_MAX];
    format_refresh_time(state->last_refresh, time_buf, sizeof(time_buf));
    attron(A_DIM);
    mvprintw(HEADER_TITLE_ROW, col + 1, " %s", time_buf);
    attroff(A_DIM);
  }
}

static void draw_tab_bar(const app_state_t *state, int x, int width) {
  static const char *tab_labels[RIGHT_TAB_COUNT] = {" Today ", " Tomorrow ",
                                                    " 4-Day ", " 9-Day "};
  int col = x + 1;

  for (int i = 0; i < RIGHT_TAB_COUNT; i++) {
    int active = (i == (int)state->right_tab);
    if (active) {
      attron(COLOR_PAIR(COLOR_PAIR_TAB_ACTIVE) | A_BOLD);
    } else {
      attron(COLOR_PAIR(COLOR_PAIR_TAB_INACTIVE));
    }
    mvprintw(HEADER_SEARCH_ROW, col, "%s", tab_labels[i]);
    if (active) {
      attroff(COLOR_PAIR(COLOR_PAIR_TAB_ACTIVE) | A_BOLD);
    } else {
      attroff(COLOR_PAIR(COLOR_PAIR_TAB_INACTIVE));
    }
    col += strlen(tab_labels[i]) + 1;
  }

  /* Show current activity mode on the right side of the tab bar */
  const char *act_label = state->activities.items[state->activity_index].label;
  int act_len = strlen(act_label) + 4;
  int x_end = x + width;
  if (col + act_len < x_end) {
    attron(A_DIM);
    mvprintw(HEADER_SEARCH_ROW, x_end - act_len - 1, "[%s]", act_label);
    attroff(A_DIM);
  }

  if (state->active_pane == PANE_RIGHT) {
    attron(COLOR_PAIR(COLOR_PAIR_HEADER));
  }
  mvhline(HEADER_BOTTOM_LINE, x + 1, ACS_HLINE, width - 1);
  if (state->active_pane == PANE_RIGHT) {
    attroff(COLOR_PAIR(COLOR_PAIR_HEADER));
  }
}

static void draw_section_col(int y, int col_x, const hourly_section_t *sec) {
  if (!sec->valid)
    return;

  mvprintw(y, col_x, "%s ", weather_code_to_icon(sec->weather_code));
  attron(COLOR_PAIR(COLOR_PAIR_TEMP_HIGH));
  printw("%.0f", sec->temp_max);
  attroff(COLOR_PAIR(COLOR_PAIR_TEMP_HIGH));
  printw("/");
  attron(COLOR_PAIR(COLOR_PAIR_TEMP_LOW));
  printw("%.0fC", sec->temp_min);
  attroff(COLOR_PAIR(COLOR_PAIR_TEMP_LOW));
  printw(" %.0fkm/h %.1fmm %d%%", sec->wind_speed_max, sec->precipitation,
         sec->humidity);
}

static int draw_hourly_section_dual(int y, int x, int width,
                                    const hourly_section_t *ecmwf,
                                    const hourly_section_t *dwd) {
  if (!ecmwf->valid && !dwd->valid)
    return y;

  int half = (width - 2) / 2;
  int col_ecmwf = x + 14;
  int col_dwd = x + 14 + half;

  /* Section label + column headers */
  attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  mvprintw(y, x + 2, "%s", ecmwf->valid ? ecmwf->label : dwd->label);
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);

  attron(A_DIM);
  mvprintw(y, col_ecmwf, SOURCE_LABEL_ECMWF);
  mvprintw(y, col_dwd, SOURCE_LABEL_DWD);
  attroff(A_DIM);
  y++;

  /* Data: single row side by side */
  draw_section_col(y, col_ecmwf, ecmwf);
  draw_section_col(y, col_dwd, dwd);
  y++;

  attron(A_DIM);
  mvhline(y, x + 2, ACS_HLINE, width - 4);
  attroff(A_DIM);

  return y + 1;
}

/* Fallback: daily detail when hourly data is unavailable */
static void draw_day_detailed(int y, int x, const day_weather_t *day) {
  mvprintw(y, x + 2, "%s  %s %s", day->date,
           weather_code_to_icon(day->weather_code),
           weather_code_to_string(day->weather_code));

  attron(COLOR_PAIR(COLOR_PAIR_TEMP_HIGH));
  mvprintw(y + 1, x + 4, "High: %.1f C", day->temp_max);
  attroff(COLOR_PAIR(COLOR_PAIR_TEMP_HIGH));

  attron(COLOR_PAIR(COLOR_PAIR_TEMP_LOW));
  printw("  Low: %.1f C", day->temp_min);
  attroff(COLOR_PAIR(COLOR_PAIR_TEMP_LOW));

  mvprintw(y + 2, x + 4, "Wind: %.0f km/h  Precip: %.1f mm",
           day->wind_speed_max, day->precipitation);
  mvprintw(y + 3, x + 4, "Humidity: %d%%  UV: %.0f", day->humidity,
           day->uv_index);
}

static void draw_day_col(int y, int col_x, const day_weather_t *day) {
  mvprintw(y, col_x, "%s ", weather_code_to_icon(day->weather_code));
  attron(COLOR_PAIR(COLOR_PAIR_TEMP_HIGH));
  printw("%.0f", day->temp_max);
  attroff(COLOR_PAIR(COLOR_PAIR_TEMP_HIGH));
  printw("/");
  attron(COLOR_PAIR(COLOR_PAIR_TEMP_LOW));
  printw("%.0fC", day->temp_min);
  attroff(COLOR_PAIR(COLOR_PAIR_TEMP_LOW));
  printw(" %.0fkm/h %.1fmm %d%%", day->wind_speed_max, day->precipitation,
         day->humidity);
}

static int draw_day_compact_dual(int y, int x, int width,
                                 const day_weather_t *ecmwf,
                                 const day_weather_t *dwd) {
  int half = (width - 2) / 2;
  int col_ecmwf = x + 14;
  int col_dwd = x + 14 + half;

  /* Date + column headers */
  attron(COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
  mvprintw(y, x + 2, "%s", ecmwf->date);
  attroff(COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);

  attron(A_DIM);
  mvprintw(y, col_ecmwf, SOURCE_LABEL_ECMWF);
  mvprintw(y, col_dwd, SOURCE_LABEL_DWD);
  attroff(A_DIM);
  y++;

  /* Data side by side */
  draw_day_col(y, col_ecmwf, ecmwf);
  if (dwd) {
    draw_day_col(y, col_dwd, dwd);
  }
  y++;

  attron(A_DIM);
  mvhline(y, x + 2, ACS_HLINE, width - 4);
  attroff(A_DIM);

  return y + 1;
}

static int draw_wrapped_text(const char *text, int x, int y, int max_w,
                             int max_y) {
  const char *p = text;
  while (*p && y < max_y) {
    /* Handle explicit newlines */
    const char *nl = strchr(p, '\n');
    int chunk_len = nl ? (int)(nl - p) : (int)strlen(p);

    while (chunk_len > 0 && y < max_y) {
      int line_len = (chunk_len > max_w) ? max_w : chunk_len;

      /* Word wrap: find last space within max_w */
      if (chunk_len > max_w) {
        int last_space = max_w;
        while (last_space > 0 && p[last_space] != ' ')
          last_space--;
        if (last_space > 0)
          line_len = last_space;
      }

      move(y, x);
      for (int i = 0; i < line_len; i++)
        addch((unsigned char)p[i]);
      y++;

      p += line_len;
      chunk_len -= line_len;
      if (*p == ' ') {
        p++;
        chunk_len--;
      }
    }

    /* Skip the newline character */
    if (*p == '\n')
      p++;
  }
  return y;
}

static int draw_gpt_summary_text(const app_state_t *state, int x, int width,
                                 int start_y) {
  int tab = (int)state->right_tab;
  int y = start_y;
  if (state->gpt_summaries[tab][0] == '\0')
    return y;
  if (y >= state->term_rows - FOOTER_HEIGHT)
    return y;

  mvhline(y, x + 1, ACS_HLINE, width - 2);
  y++;

  attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  mvprintw(y, x + 2, "AI Summary (%s):",
           state->activities.items[state->activity_index].label);
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  y++;

  return draw_wrapped_text(state->gpt_summaries[tab], x + 2, y, width - 4,
                           state->term_rows - FOOTER_HEIGHT);
}

static void draw_tab_detail(const app_state_t *state, int x, int width,
                            const today_detail_t *ecmwf,
                            const today_detail_t *dwd, int fallback_day) {
  int content_start = LEFT_PANE_HEADER_ROWS - 1;
  int y = content_start - state->right_scroll;
  int bottom = state->term_rows - FOOTER_HEIGHT;

  if (ecmwf->valid || dwd->valid) {
    const today_detail_t *primary = ecmwf->valid ? ecmwf : dwd;
    if (y >= content_start && y < bottom) {
      attron(COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
      mvprintw(y, x + 2, "%s", primary->date);
      attroff(COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
    }
    y++;

    for (int i = 0; i < SECTION_COUNT; i++) {
      if (y >= bottom)
        break;
      if (y >= content_start) {
        y = draw_hourly_section_dual(y, x, width, &ecmwf->sections[i],
                                     &dwd->sections[i]);
      } else {
        y += 4;
      }
    }
  } else if (fallback_day < state->forecast.num_days) {
    if (y >= content_start && y < bottom) {
      draw_day_detailed(y, x, &state->forecast.days[fallback_day]);
    }
    y += 5;
  }

  draw_gpt_summary_text(state, x, width, y);
}

static int draw_tab_multiday_content(const app_state_t *state, int x, int width,
                                     int start, int end) {
  int content_start = LEFT_PANE_HEADER_ROWS - 1;
  int y = content_start - state->right_scroll;
  for (int i = start; i < end && i < state->forecast.num_days; i++) {
    if (y >= state->term_rows - FOOTER_HEIGHT)
      break;
    if (y >= content_start) {
      const day_weather_t *dwd = (i < state->forecast_dwd.num_days)
                                     ? &state->forecast_dwd.days[i]
                                     : NULL;
      y = draw_day_compact_dual(y, x, width, &state->forecast.days[i], dwd);
    } else {
      y += 4;
    }
  }
  return y;
}

static void draw_tab_multiday(const app_state_t *state, int x, int width,
                              int start, int end) {
  int y = draw_tab_multiday_content(state, x, width, start, end);
  draw_gpt_summary_text(state, x, width, y);
}

static void draw_chat_response_pane(const app_state_t *state, int x,
                                    int width) {
  int content_start = LEFT_PANE_HEADER_ROWS - 1;
  int y = content_start - state->right_scroll;

  attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  if (y >= content_start)
    mvprintw(y, x + 2, "Chat Response:");
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  y++;

  if (y >= content_start) {
    draw_wrapped_text(state->chat_response, x + 2, y, width - 4,
                      state->term_rows - FOOTER_HEIGHT);
  }
}

static void draw_active_tab(const app_state_t *state, int x, int width) {
  switch (state->right_tab) {
  case TAB_TODAY:
    draw_tab_detail(state, x, width, &state->today_detail,
                    &state->today_detail_dwd, 0);
    break;
  case TAB_TOMORROW:
    draw_tab_detail(state, x, width, &state->tomorrow_detail,
                    &state->tomorrow_detail_dwd, 1);
    break;
  case TAB_4DAY:
    draw_tab_multiday(state, x, width, 0, FORECAST_4DAY);
    break;
  case TAB_9DAY:
    draw_tab_multiday(state, x, width, 0, FORECAST_9DAY);
    break;
  }
}

static void draw_right_pane(const app_state_t *state) {
  int x = state->left_width + 1;
  int width = state->term_cols - state->left_width - 1;

  if (!state->has_forecast) {
    attron(A_DIM);
    mvprintw(state->term_rows / 2, x + width / 2 - 10, "Select a location");
    attroff(A_DIM);
    return;
  }

  if (state->loading) {
    attron(A_DIM);
    mvprintw(state->term_rows / 2, x + width / 2 - 5, "Loading...");
    attroff(A_DIM);
    return;
  }

  draw_location_header(state, x);
  draw_tab_bar(state, x, width);

  if (state->chat_has_response) {
    draw_chat_response_pane(state, x, width);
  } else {
    draw_active_tab(state, x, width);
  }
}

static void expire_status(app_state_t *state) {
  if (state->status_expire > 0 && time(NULL) >= state->status_expire) {
    state->status_msg[0] = '\0';
    state->status_expire = 0;
  }
}

void ui_render(const app_state_t *state) {
  expire_status((app_state_t *)state);
  erase();
  draw_header_lines(state);
  draw_border(state->left_width, state->term_rows);
  draw_left_title(state);
  draw_search_bar(state);
  draw_left_list((app_state_t *)state);
  draw_right_pane(state);
  draw_footer(state);
  refresh();
}

static void clamp_cursor(app_state_t *state) {
  location_list_t *list = get_visible_list(state);
  if (state->cursor_index >= list->count) {
    state->cursor_index = list->count - 1;
  }
  if (state->cursor_index < 0) {
    state->cursor_index = 0;
  }

  /* Adjust scroll to keep cursor visible */
  int max_visible = state->term_rows - LEFT_PANE_HEADER_ROWS - FOOTER_HEIGHT;
  if (state->cursor_index < state->scroll_offset + SCROLL_PADDING) {
    state->scroll_offset = state->cursor_index - SCROLL_PADDING;
  }
  if (state->cursor_index >=
      state->scroll_offset + max_visible - SCROLL_PADDING) {
    state->scroll_offset =
        state->cursor_index - max_visible + SCROLL_PADDING + 1;
  }
  if (state->scroll_offset < 0) {
    state->scroll_offset = 0;
  }
}

static void invalidate_gpt_summaries(app_state_t *state) {
  for (int i = 0; i < RIGHT_TAB_COUNT; i++) {
    state->gpt_summaries[i][0] = '\0';
    state->gpt_loaded[i] = 0;
  }
}

static void get_tab_day_range(right_tab_t tab, int *start, int *end) {
  switch (tab) {
  case TAB_TODAY:
    *start = 0;
    *end = FORECAST_DAILY;
    break;
  case TAB_TOMORROW:
    *start = 1;
    *end = FORECAST_DAILY + 1;
    break;
  case TAB_4DAY:
    *start = 0;
    *end = FORECAST_4DAY;
    break;
  case TAB_9DAY:
    *start = 0;
    *end = FORECAST_9DAY;
    break;
  }
}

static void set_progress(app_state_t *state, int percent, const char *msg) {
  state->progress = percent;
  ui_set_status(state, "%s", msg);
  ui_render(state);
}

/* Thread argument for async GPT fetch */
typedef struct {
  forecast_t forecast;
  char location_name[MAX_LOCATION_NAME];
  char activity_prompt[512];
  int day_start;
  int day_end;
  char result[2048];
  int *done_flag;
} gpt_thread_arg_t;

static void *gpt_thread_worker(void *arg) {
  gpt_thread_arg_t *a = (gpt_thread_arg_t *)arg;
  gpt_summarize(&a->forecast, a->location_name, a->activity_prompt,
                a->day_start, a->day_end, a->result, sizeof(a->result));
  __atomic_store_n(a->done_flag, 1, __ATOMIC_RELEASE);
  return arg;
}

static void cancel_async_gpt(app_state_t *state) {
  if (!state->gpt_thread_active)
    return;
  pthread_detach(state->gpt_thread);
  state->gpt_thread_active = 0;
  state->gpt_thread_done = 0;
}

static gpt_thread_arg_t *build_gpt_thread_arg(app_state_t *state,
                                              const location_t *loc) {
  gpt_thread_arg_t *arg = malloc(sizeof(*arg));
  if (!arg)
    return NULL;
  int day_start = 0;
  int day_end = 0;
  get_tab_day_range(state->right_tab, &day_start, &day_end);

  arg->forecast = state->forecast;
  snprintf(arg->location_name, sizeof(arg->location_name), "%s", loc->name);
  snprintf(arg->activity_prompt, sizeof(arg->activity_prompt), "%s",
           state->activities.items[state->activity_index].prompt);
  arg->day_start = day_start;
  arg->day_end = day_end;
  arg->result[0] = '\0';
  arg->done_flag = &state->gpt_thread_done;
  return arg;
}

static void launch_async_gpt(app_state_t *state) {
  int tab = (int)state->right_tab;
  if (state->gpt_loaded[tab] || !gpt_is_available() || !state->has_forecast)
    return;

  location_list_t *list = get_visible_list(state);
  if (state->cursor_index < 0 || state->cursor_index >= list->count)
    return;

  cancel_async_gpt(state);

  const location_t *loc = &list->items[state->cursor_index];
  state->gpt_thread_tab = tab;
  state->gpt_thread_done = 0;

  gpt_thread_arg_t *arg = build_gpt_thread_arg(state, loc);
  if (!arg)
    return;

  state->gpt_thread_active = 1;
  ui_set_status(state, "Loading AI summary...");

  if (pthread_create(&state->gpt_thread, NULL, gpt_thread_worker, arg) != 0) {
    free(arg);
    state->gpt_thread_active = 0;
    ui_set_status(state, "");
    return;
  }
}

void ui_check_async_gpt(app_state_t *state) {
  if (!state->gpt_thread_active)
    return;
  if (!__atomic_load_n(&state->gpt_thread_done, __ATOMIC_ACQUIRE))
    return;

  /* Thread finished — join and collect result */
  gpt_thread_arg_t *arg = NULL;
  pthread_join(state->gpt_thread, (void **)&arg);
  state->gpt_thread_active = 0;
  state->gpt_thread_done = 0;

  if (arg) {
    int tab = state->gpt_thread_tab;
    if (tab >= 0 && tab < RIGHT_TAB_COUNT && !state->gpt_loaded[tab]) {
      snprintf(state->gpt_summaries[tab], sizeof(state->gpt_summaries[tab]),
               "%s", arg->result);
      state->gpt_loaded[tab] = 1;
    }
    free(arg);
  }

  ui_set_status(state, "");
}

static int fetch_all_forecast_data(app_state_t *state, const location_t *loc,
                                   int force) {
  set_progress(state, 5, "Fetching ECMWF data...");
  int result = weather_fetch(loc->latitude, loc->longitude, FORECAST_9DAY,
                             &state->forecast, force);
  if (result != 0)
    return -1;

  set_progress(state, 20, "Fetching ECMWF hourly...");
  weather_fetch_hourly(loc->latitude, loc->longitude, &state->today_detail,
                       &state->tomorrow_detail, force);

  set_progress(state, 35, "Fetching DWD ICON data...");
  weather_fetch_dwd(loc->latitude, loc->longitude, FORECAST_9DAY,
                    &state->forecast_dwd, force);

  set_progress(state, 50, "Fetching DWD ICON hourly...");
  weather_fetch_hourly_dwd(loc->latitude, loc->longitude,
                           &state->today_detail_dwd,
                           &state->tomorrow_detail_dwd, force);

  set_progress(state, 100, "Done");
  return 0;
}

static void finalize_forecast_load(app_state_t *state) {
  state->loading = 0;
  state->progress = 0;
  state->loading_location[0] = '\0';

  if (!state->has_forecast)
    return;

  state->active_pane = PANE_RIGHT;
  char time_buf[TIME_BUF_MAX];
  format_refresh_time(state->last_refresh, time_buf, sizeof(time_buf));
  ui_set_status(state, "Refreshed %s", time_buf);
  state->status_expire = time(NULL) + STATUS_DISPLAY_SECONDS;

  /* Launch GPT summary in background — forecast displays immediately */
  launch_async_gpt(state);
}

static void fetch_forecast_for_selected(app_state_t *state, int force) {
  location_list_t *list = get_visible_list(state);
  if (state->cursor_index < 0 || state->cursor_index >= list->count)
    return;

  const location_t *loc = &list->items[state->cursor_index];

  snprintf(state->loading_location, sizeof(state->loading_location), "%s, %s",
           loc->name, loc->country);
  state->loading = 1;
  invalidate_gpt_summaries(state);

  if (fetch_all_forecast_data(state, loc, force) == 0) {
    state->has_forecast = 1;
    long mtime = weather_cache_mtime(loc->latitude, loc->longitude);
    state->last_refresh = mtime > 0 ? (time_t)mtime : time(NULL);
    state->right_scroll = 0;
  } else {
    state->has_forecast = 0;
    ui_set_status(state, "Weather service unavailable — try again later");
  }

  finalize_forecast_load(state);
}

void ui_check_date_rollover(app_state_t *state) {
  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  if (!lt)
    return;
  if (lt->tm_yday == state->last_yday)
    return;
  state->last_yday = lt->tm_yday;
  if (state->has_forecast)
    fetch_forecast_for_selected(state, 1);
}

static void handle_search_input(app_state_t *state, int ch) {
  if (ch == KEY_BACKSPACE_CODE || ch == KEY_BACKSPACE) {
    if (state->search_len > 0) {
      state->search_len--;
      state->search_input[state->search_len] = '\0';
    }
    if (state->search_len == 0) {
      state->left_mode = LEFT_MODE_SAVED;
      state->search_results.count = 0;
    } else {
      location_filter(&state->saved, state->search_input, &state->filtered);
    }
    state->cursor_index = 0;
    state->scroll_offset = 0;
    return;
  }

  if (isprint(ch) && state->search_len < SEARCH_INPUT_MAX - 1) {
    state->search_input[state->search_len] = ch;
    state->search_len++;
    state->search_input[state->search_len] = '\0';

    /* Filter saved locations as user types */
    location_filter(&state->saved, state->search_input, &state->filtered);
    state->cursor_index = 0;
    state->scroll_offset = 0;
  }
}

static void handle_enter_key(app_state_t *state) {
  if (state->search_len > 0 && state->left_mode == LEFT_MODE_SAVED) {
    /* Trigger API search */
    state->loading = 1;
    ui_set_status(state, "Searching...");
    ui_render(state);

    int count = location_search(state->search_input, &state->search_results);
    state->loading = 0;

    if (count > 0) {
      state->left_mode = LEFT_MODE_SEARCH;
      state->cursor_index = 0;
      state->scroll_offset = 0;
      ui_set_status(state, "Found %d locations", count);
    } else {
      ui_set_status(state, "No locations found");
    }
    return;
  }

  fetch_forecast_for_selected(state, 0);
}

static void handle_save_location(app_state_t *state) {
  if (state->left_mode != LEFT_MODE_SEARCH)
    return;

  location_list_t *list = &state->search_results;
  if (state->cursor_index < 0 || state->cursor_index >= list->count)
    return;

  const location_t *loc = &list->items[state->cursor_index];
  if (location_add(&state->saved, loc) == 0) {
    location_save(&state->saved);
    ui_set_status(state, "Saved %s", loc->name);
  } else {
    ui_set_status(state, "location_t list full");
  }
}

static void handle_delete_location(app_state_t *state) {
  if (state->left_mode != LEFT_MODE_SAVED)
    return;

  location_list_t *list = get_visible_list(state);
  if (state->cursor_index < 0 || state->cursor_index >= list->count)
    return;

  /* Find actual index in saved list if filtering */
  if (state->search_len > 0) {
    const location_t *loc = &state->filtered.items[state->cursor_index];
    for (int i = 0; i < state->saved.count; i++) {
      if (state->saved.items[i].latitude == loc->latitude &&
          state->saved.items[i].longitude == loc->longitude) {
        location_remove(&state->saved, i);
        break;
      }
    }
  } else {
    location_remove(&state->saved, state->cursor_index);
  }

  location_save(&state->saved);
  if (state->search_len > 0) {
    location_filter(&state->saved, state->search_input, &state->filtered);
  }
  clamp_cursor(state);
  ui_set_status(state, "location_t removed");
}

static void handle_escape(app_state_t *state) {
  if (state->left_mode == LEFT_MODE_SEARCH) {
    state->left_mode = LEFT_MODE_SAVED;
    state->search_results.count = 0;
    state->cursor_index = 0;
    state->scroll_offset = 0;
  }
  state->search_input[0] = '\0';
  state->search_len = 0;
  state->filtered.count = 0;
}

static void draw_activity_list(const app_state_t *state, int x, int y,
                               int selected) {
  for (int i = 0; i < state->activities.count; i++) {
    int is_selected = (i == selected);
    int is_current = (i == state->activity_index);

    if (is_selected) {
      attron(COLOR_PAIR(COLOR_PAIR_SELECTED));
    } else if (is_current) {
      attron(A_BOLD);
    }

    mvprintw(y + i, x + 1, " %s%s", state->activities.items[i].label,
             is_current ? " *" : "");

    if (is_selected) {
      attroff(COLOR_PAIR(COLOR_PAIR_SELECTED));
    } else if (is_current) {
      attroff(A_BOLD);
    }
  }
}

static void render_activity_picker(const app_state_t *state, int x, int width,
                                   int top_y, int selected) {
  for (int row = top_y; row < state->term_rows - FOOTER_HEIGHT; row++) {
    mvhline(row, state->left_width + 1, ' ', width + 1);
  }

  attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  mvprintw(top_y, x, "Select activity");
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);

  draw_activity_list(state, x, top_y + 2, selected);

  int hint_y = top_y + 2 + state->activities.count + 1;
  attron(A_DIM);
  mvprintw(hint_y, x + 1, "Up/Down: select  Enter: confirm  Esc: cancel");
  attroff(A_DIM);
  refresh();
}

/* Run the picker loop. Returns selected index, or -1 on cancel. */
static int run_activity_picker_loop(app_state_t *state) {
  int x = state->left_width + 2;
  int width = state->term_cols - state->left_width - 3;
  int top_y = LEFT_PANE_HEADER_ROWS;
  int selected = state->activity_index;

  timeout(-1);
  for (;;) {
    render_activity_picker(state, x, width, top_y, selected);
    int ch = getch();
    if (ch == KEY_UP && selected > 0) {
      selected--;
    } else if (ch == KEY_DOWN && selected < state->activities.count - 1) {
      selected++;
    } else if (ch == KEY_ENTER_CODE || ch == KEY_ENTER) {
      timeout(GETCH_TIMEOUT_MS);
      return selected;
    } else if (ch == KEY_ESCAPE_CODE) {
      timeout(GETCH_TIMEOUT_MS);
      return -1;
    }
  }
}

static void show_activity_picker(app_state_t *state) {
  int selected = run_activity_picker_loop(state);
  if (selected < 0 || selected == state->activity_index)
    return;

  state->activity_index = selected;
  invalidate_gpt_summaries(state);
  ui_set_status(state, "Activity: %s",
                state->activities.items[state->activity_index].label);
  launch_async_gpt(state);
}

static void open_chat(app_state_t *state) {
  state->chat_active = 1;
  state->chat_input[0] = '\0';
  state->chat_input_len = 0;
}

static void close_chat(app_state_t *state) {
  state->chat_active = 0;
  state->chat_input[0] = '\0';
  state->chat_input_len = 0;
}

/* Map a 2-byte UTF-8 sequence starting with 0xC3 to ASCII transliteration.
 * Writes 0-2 chars into dst[*j] and advances *j. */
static void translit_c3(unsigned char c2, char *dst, size_t *j) {
  if (c2 == 0x84 || c2 == 0xA4) { /* Ä ä -> ae */
    dst[(*j)++] = 'a';
    dst[(*j)++] = 'e';
  } else if (c2 == 0x96 || c2 == 0xB6) { /* Ö ö -> oe */
    dst[(*j)++] = 'o';
    dst[(*j)++] = 'e';
  } else if (c2 == 0x9C || c2 == 0xBC) { /* Ü ü -> ue */
    dst[(*j)++] = 'u';
    dst[(*j)++] = 'e';
  } else if (c2 == 0x9F) { /* ß -> ss */
    dst[(*j)++] = 's';
    dst[(*j)++] = 's';
  }
}

/* Skip past a multi-byte UTF-8 continuation; returns bytes consumed beyond
 * the lead byte. */
static int utf8_skip_continuation(const char *src, int extra) {
  int n = 0;
  for (int k = 0; k < extra && src[k + 1]; k++)
    n++;
  return n;
}

static size_t normalize_for_match(const char *src, char *dst, size_t dst_size) {
  size_t j = 0;
  for (size_t i = 0; src[i] && j < dst_size - 2; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c < 0x80) {
      dst[j++] = tolower(c);
      continue;
    }
    if (c >= 0xC2 && c <= 0xDF) {
      unsigned char c2 = (unsigned char)src[i + 1];
      if (!c2)
        break;
      i++;
      if (c == 0xC3)
        translit_c3(c2, dst, &j);
      continue;
    }
    if (c >= 0xE0 && c <= 0xEF) {
      i += utf8_skip_continuation(src + i, 2);
      continue;
    }
    if (c >= 0xF0) {
      i += utf8_skip_continuation(src + i, 3);
    }
  }
  dst[j] = '\0';
  return j;
}

static int match_location_in_query(const char *query, const location_t *loc) {
  char norm_query[CHAT_INPUT_MAX * 2];
  char norm_name[MAX_LOCATION_NAME * 2];

  normalize_for_match(query, norm_query, sizeof(norm_query));
  normalize_for_match(loc->name, norm_name, sizeof(norm_name));

  return strstr(norm_query, norm_name) != NULL;
}

static int build_chat_context(app_state_t *state, char *context,
                              size_t context_size) {
  int offset = 0;
  int found = 0;

  set_progress(state, 20, "Finding locations...");

  /* Search saved locations mentioned in the question */
  for (int i = 0; i < state->saved.count && found < CHAT_MAX_MATCHED_LOCATIONS;
       i++) {
    if (!match_location_in_query(state->chat_input, &state->saved.items[i]))
      continue;

    forecast_t f = {0};
    char msg[LOADING_MSG_MAX];
    snprintf(msg, sizeof(msg), "Fetching weather for %s...",
             state->saved.items[i].name);
    set_progress(state, 20 + found * 15, msg);

    if (weather_fetch(state->saved.items[i].latitude,
                      state->saved.items[i].longitude, FORECAST_9DAY, &f,
                      0) == 0) {
      offset = gpt_format_forecast(&f, state->saved.items[i].name, context,
                                   context_size, offset);
      found++;
    }
  }

  /* If no locations matched, use the currently selected one */
  if (found == 0 && state->has_forecast) {
    location_list_t *list = get_visible_list(state);
    if (state->cursor_index >= 0 && state->cursor_index < list->count) {
      const location_t *loc = &list->items[state->cursor_index];
      offset = gpt_format_forecast(&state->forecast, loc->name, context,
                                   context_size, offset);
      found = 1;
    }
  }

  return found;
}

static void submit_chat(app_state_t *state) {
  if (state->chat_input_len == 0)
    return;

  state->chat_active = 0;

  char context[CHAT_CONTEXT_MAX] = {0};
  int found = build_chat_context(state, context, sizeof(context));

  if (found == 0) {
    snprintf(state->chat_response, sizeof(state->chat_response),
             "[No weather data available]");
    state->chat_has_response = 1;
    state->progress = 0;
    ui_set_status(state, "");
    return;
  }

  set_progress(state, 70, "Asking AI...");

  gpt_chat(context, state->chat_input, state->chat_response,
           sizeof(state->chat_response));
  state->chat_has_response = 1;
  state->chat_input[0] = '\0';
  state->chat_input_len = 0;
  state->progress = 0;
  ui_set_status(state, "");
}

static void switch_right_tab(app_state_t *state, int direction) {
  int tab = (int)state->right_tab + direction;
  if (tab < 0)
    tab = RIGHT_TAB_COUNT - 1;
  if (tab >= RIGHT_TAB_COUNT)
    tab = 0;
  state->right_tab = (right_tab_t)tab;
  state->right_scroll = 0;
  launch_async_gpt(state);
}

static int confirm_quit(app_state_t *state) {
  int mid = state->term_rows - FOOTER_HEIGHT + 1;

  mvhline(mid, 0, ' ', state->term_cols);
  attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  mvprintw(mid, 1, "Quit? (y/n)");
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  refresh();

  timeout(-1);
  int answer = getch();
  timeout(GETCH_TIMEOUT_MS);

  if (answer == 'y' || answer == 'Y') {
    state->running = 0;
    return 1;
  }
  return 0;
}

static void draw_info_keyboard_column(int x, int y) {
  attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  mvprintw(y, x, "Keyboard");
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  int ky = y + 1;
  mvprintw(ky++, x + 1, "S-Tab  Switch pane");
  mvprintw(ky++, x + 1, "Arrows Navigate/tabs/scroll");
  mvprintw(ky++, x + 1, "Enter  Search/load forecast");
  mvprintw(ky++, x + 1, "a      Activity mode");
  mvprintw(ky++, x + 1, "c      Chat");
  mvprintw(ky++, x + 1, "r      Refresh");
  mvprintw(ky++, x + 1, "s      Save location");
  mvprintw(ky++, x + 1, "x      Delete location");
  mvprintw(ky++, x + 1, "?      This screen");
  mvprintw(ky++, x + 1, "q      Quit");
}

static void draw_info_sources_column(int col2, int y) {
  attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  mvprintw(y, col2, "Data Sources");
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  int dy = y + 1;
  mvprintw(dy++, col2 + 1, "ECMWF   via Open-Meteo");
  mvprintw(dy++, col2 + 1, "DWD ICON via Open-Meteo");
  mvprintw(dy++, col2 + 1, "Search  Open-Meteo/Nominatim");
  mvprintw(dy++, col2 + 1, "AI      OpenAI GPT (%s)", GPT_MODEL);
  dy++;

  attron(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  mvprintw(dy, col2, "Reading the forecast");
  attroff(COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
  dy++;
  mvprintw(dy++, col2 + 1, "Icon Max/MinC Wind Precip Hum");
  mvprintw(dy++, col2 + 1, "%s 18/12C 15km/h 0.2mm 65%%",
           weather_code_to_icon(WMO_OVERCAST));
}

static void show_info(app_state_t *state) {
  int x = state->left_width + 2;
  int width = state->term_cols - state->left_width - 3;
  int col2 = x + width / 2;
  int y = LEFT_PANE_HEADER_ROWS;

  for (int row = y; row < state->term_rows - FOOTER_HEIGHT; row++) {
    mvhline(row, state->left_width + 1, ' ', width + 1);
  }

  attron(COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
  mvprintw(y, x, "%s", APP_VERSION);
  attroff(COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
  y += 2;

  draw_info_keyboard_column(x, y);
  draw_info_sources_column(col2, y);

  int mid = state->term_rows - FOOTER_HEIGHT + 1;
  mvhline(mid, 0, ' ', state->term_cols);
  attron(A_DIM);
  mvprintw(mid, 1, "Press any key to close");
  attroff(A_DIM);
  refresh();

  timeout(-1);
  getch();
  timeout(GETCH_TIMEOUT_MS);
}

static void handle_chat_input(app_state_t *state, int ch) {
  if (ch == KEY_ESCAPE_CODE) {
    close_chat(state);
    return;
  }
  if (ch == KEY_ENTER_CODE || ch == KEY_ENTER) {
    submit_chat(state);
    return;
  }
  if (ch == KEY_BACKSPACE_CODE || ch == KEY_BACKSPACE) {
    if (state->chat_input_len > 0) {
      state->chat_input_len--;
      state->chat_input[state->chat_input_len] = '\0';
    }
    return;
  }
  if (isprint(ch) && state->chat_input_len < CHAT_INPUT_MAX - 1) {
    state->chat_input[state->chat_input_len] = ch;
    state->chat_input_len++;
    state->chat_input[state->chat_input_len] = '\0';
  }
}

static void handle_left_pane_input(app_state_t *state, int ch) {
  switch (ch) {
  case KEY_UP:
    state->cursor_index--;
    clamp_cursor(state);
    return;
  case KEY_DOWN:
    state->cursor_index++;
    clamp_cursor(state);
    return;
  case KEY_ENTER_CODE:
  case KEY_ENTER:
    handle_enter_key(state);
    return;
  case KEY_ESCAPE_CODE:
    handle_escape(state);
    return;
  case KEY_SAVE_LOCATION:
    if (state->left_mode == LEFT_MODE_SEARCH) {
      handle_save_location(state);
      return;
    }
    handle_search_input(state, ch);
    return;
  case KEY_DELETE_LOCATION:
    if (state->left_mode == LEFT_MODE_SAVED && state->search_len == 0) {
      handle_delete_location(state);
      return;
    }
    handle_search_input(state, ch);
    return;
  default:
    handle_search_input(state, ch);
    return;
  }
}

static int handle_right_pane_input(app_state_t *state, int ch) {
  switch (ch) {
  case KEY_LEFT:
    switch_right_tab(state, -1);
    return 0;
  case KEY_RIGHT:
    switch_right_tab(state, 1);
    return 0;
  case KEY_UP:
    if (state->right_scroll > 0)
      state->right_scroll--;
    return 0;
  case KEY_DOWN:
    state->right_scroll++;
    return 0;
  case 'c':
    if (state->has_forecast) {
      open_chat(state);
    }
    return 0;
  case KEY_ACTIVITY:
    show_activity_picker(state);
    return 0;
  case KEY_ESCAPE_CODE:
    if (state->chat_has_response) {
      state->chat_has_response = 0;
    }
    return 0;
  case KEY_QUIT:
    if (state->chat_has_response) {
      state->chat_has_response = 0;
      return 0;
    }
    return confirm_quit(state);
  }
  return 0;
}

int ui_handle_input(app_state_t *state, int ch) {
  if (ch == ERR)
    return 0;

  if (ch == KEY_RESIZE) {
    getmaxyx(stdscr, state->term_rows, state->term_cols);
    state->left_width = ui_calc_left_width(state->term_cols);
    return 0;
  }

  if (state->chat_active) {
    handle_chat_input(state, ch);
    return 0;
  }

  if (ch == KEY_INFO) {
    show_info(state);
    return 0;
  }

  if (ch == KEY_RELOAD && state->has_forecast && !state->loading) {
    state->chat_has_response = 0;
    fetch_forecast_for_selected(state, 1);
    return 0;
  }

  if (ch == KEY_BTAB) {
    state->active_pane =
        (state->active_pane == PANE_LEFT) ? PANE_RIGHT : PANE_LEFT;
    return 0;
  }

  if (state->active_pane == PANE_LEFT) {
    handle_left_pane_input(state, ch);
    return 0;
  }
  return handle_right_pane_input(state, ch);
}
