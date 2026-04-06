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
