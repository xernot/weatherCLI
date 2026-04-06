#include "secrets.h"
#include "ui.h"

#include <curl/curl.h>
#include <locale.h>
#include <ncurses.h>

int main(void) {
  setlocale(LC_ALL, "");
  secrets_load();
  curl_global_init(CURL_GLOBAL_DEFAULT);

  AppState state = {0};
  ui_init(&state);

  while (state.running) {
    ui_render(&state);
    int ch = getch();
    ui_handle_input(&state, ch);
  }

  ui_cleanup();
  curl_global_cleanup();

  return 0;
}
