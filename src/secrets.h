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

#ifndef SECRETS_H
#define SECRETS_H

/* Load secrets from ~/.weathercli/secrets into memory.
   Call once at startup. */
void secrets_load(void);

/* Get a secret value by key name. Checks the secrets file first,
   falls back to environment variable. Returns NULL if not found. */
const char *secrets_get(const char *key);

#endif /* SECRETS_H */
