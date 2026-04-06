#include "activity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void add_default_activities(ActivityList *list) {
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

static int parse_activities_file(FILE *f, ActivityList *list) {
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

    Activity *act = &list->items[list->count];
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

int activity_load(ActivityList *list) {
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
