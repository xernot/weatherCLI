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

#include "cache.h"
#include "secrets.h"
#include "ui.h"

#include <curl/curl.h>
#include <locale.h>
#include <ncurses.h>

int main(void) {
  setlocale(LC_ALL, "");
  secrets_load();
  curl_global_init(CURL_GLOBAL_DEFAULT);
  cache_refresh_start();

  AppState state = {0};
  ui_init(&state);

  while (state.running) {
    ui_check_async_gpt(&state);
    ui_render(&state);
    int ch = getch();
    ui_handle_input(&state, ch);
  }

  /* Cancel any in-flight async GPT thread before cleanup */
  if (state.gpt_thread_active) {
    pthread_detach(state.gpt_thread);
  }

  ui_cleanup();
  cache_refresh_stop();
  curl_global_cleanup();

  return 0;
}
