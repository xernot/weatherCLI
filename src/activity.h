#ifndef ACTIVITY_H
#define ACTIVITY_H

#include "config.h"

/* Describes an activity mode for GPT summaries */
typedef struct {
  char label[MAX_ACTIVITY_LABEL];   /* Human-readable label for UI */
  char prompt[MAX_ACTIVITY_PROMPT]; /* GPT prompt context for this activity */
} Activity;

/* Runtime activity list, loaded from config or defaults */
typedef struct {
  Activity items[MAX_ACTIVITY_COUNT];
  int count;
} ActivityList;

/* Load activities from ~/.weathercli/activities.conf.
   Falls back to built-in defaults if file is missing.
   Returns 0 on success. */
int activity_load(ActivityList *list);

#endif /* ACTIVITY_H */
