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

#include "secrets.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum number of secrets stored */
#define MAX_SECRETS 16

/* Maximum length of a single line in the secrets file */
#define MAX_SECRET_LINE 512

typedef struct {
  char key[64];
  char value[256];
} SecretEntry;

static SecretEntry entries[MAX_SECRETS];
static int entry_count = 0;

static int get_secrets_path(char *buf, size_t bufsize) {
  const char *home = getenv("HOME");
  if (!home)
    return -1;
  snprintf(buf, bufsize, "%s/%s", home, SECRETS_FILE);
  return 0;
}

static void parse_secret_line(const char *line) {
  if (entry_count >= MAX_SECRETS)
    return;
  if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
    return;

  const char *eq = strchr(line, '=');
  if (!eq)
    return;

  int key_len = eq - line;
  if (key_len <= 0 || key_len >= 64)
    return;

  SecretEntry *e = &entries[entry_count];
  snprintf(e->key, sizeof(e->key), "%.*s", key_len, line);

  const char *val_start = eq + 1;
  snprintf(e->value, sizeof(e->value), "%s", val_start);

  /* Strip trailing newline */
  int vlen = strlen(e->value);
  while (vlen > 0 &&
         (e->value[vlen - 1] == '\n' || e->value[vlen - 1] == '\r')) {
    e->value[vlen - 1] = '\0';
    vlen--;
  }

  entry_count++;
}

void secrets_load(void) {
  char path[512];
  if (get_secrets_path(path, sizeof(path)) != 0)
    return;

  FILE *f = fopen(path, "r");
  if (!f)
    return;

  char line[MAX_SECRET_LINE];
  while (fgets(line, sizeof(line), f)) {
    parse_secret_line(line);
  }
  fclose(f);
}

const char *secrets_get(const char *key) {
  for (int i = 0; i < entry_count; i++) {
    if (strcmp(entries[i].key, key) == 0) {
      return entries[i].value;
    }
  }
  return getenv(key);
}
