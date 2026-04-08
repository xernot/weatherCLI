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

#include "activity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void add_default_activities(activity_list_t *list) {
  list->count = 0;

  snprintf(list->items[0].label, MAX_ACTIVITY_LABEL, "%s",
           DEFAULT_ACTIVITY_LABEL_GENERAL);
  snprintf(list->items[0].prompt, MAX_ACTIVITY_PROMPT, "%s",
           DEFAULT_ACTIVITY_PROMPT_GENERAL);
  list->count++;

  snprintf(list->items[1].label, MAX_ACTIVITY_LABEL, "%s",
           DEFAULT_ACTIVITY_LABEL_BIKE);
  snprintf(list->items[1].prompt, MAX_ACTIVITY_PROMPT, "%s",
           DEFAULT_ACTIVITY_PROMPT_BIKE);
  list->count++;

  snprintf(list->items[2].label, MAX_ACTIVITY_LABEL, "%s",
           DEFAULT_ACTIVITY_LABEL_SKI);
  snprintf(list->items[2].prompt, MAX_ACTIVITY_PROMPT, "%s",
           DEFAULT_ACTIVITY_PROMPT_SKI);
  list->count++;

  snprintf(list->items[3].label, MAX_ACTIVITY_LABEL, "%s",
           DEFAULT_ACTIVITY_LABEL_SAIL);
  snprintf(list->items[3].prompt, MAX_ACTIVITY_PROMPT, "%s",
           DEFAULT_ACTIVITY_PROMPT_SAIL);
  list->count++;
}

static int get_filepath(char *buf, size_t bufsize) {
  const char *home = getenv("HOME");
  if (!home)
    return -1;
  snprintf(buf, bufsize, "%s/%s", home, ACTIVITIES_FILE);
  return 0;
}

static void trim_trailing(char *s) {
  int len = strlen(s);
  while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                     s[len - 1] == ' ' || s[len - 1] == '\t')) {
    s[--len] = '\0';
  }
}

static int parse_activities_file(FILE *f, activity_list_t *list) {
  char line[512];
  list->count = 0;

  while (fgets(line, sizeof(line), f) && list->count < MAX_ACTIVITY_COUNT) {
    trim_trailing(line);

    /* Skip empty lines and comments */
    if (line[0] == '\0' || line[0] == '#')
      continue;

    /* Find the separator between label and prompt */
    char *sep = strchr(line, '|');
    if (!sep)
      continue;

    *sep = '\0';
    char *label_src = line;
    char *prompt_src = sep + 1;

    trim_trailing(label_src);

    if (label_src[0] == '\0' || prompt_src[0] == '\0')
      continue;

    activity_t *act = &list->items[list->count];
    size_t llen = strlen(label_src);
    size_t plen = strlen(prompt_src);
    if (llen >= MAX_ACTIVITY_LABEL)
      llen = MAX_ACTIVITY_LABEL - 1;
    if (plen >= MAX_ACTIVITY_PROMPT)
      plen = MAX_ACTIVITY_PROMPT - 1;
    memcpy(act->label, label_src, llen);
    act->label[llen] = '\0';
    memcpy(act->prompt, prompt_src, plen);
    act->prompt[plen] = '\0';
    list->count++;
  }

  return (list->count > 0) ? 0 : -1;
}

int activity_load(activity_list_t *list) {
  char filepath[512];
  if (get_filepath(filepath, sizeof(filepath)) != 0) {
    add_default_activities(list);
    return 0;
  }

  FILE *f = fopen(filepath, "r");
  if (!f) {
    add_default_activities(list);
    return 0;
  }

  int result = parse_activities_file(f, list);
  fclose(f);

  if (result != 0) {
    add_default_activities(list);
  }

  return 0;
}
