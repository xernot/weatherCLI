#ifndef SECRETS_H
#define SECRETS_H

/* Load secrets from ~/.weathercli/secrets into memory.
   Call once at startup. */
void secrets_load(void);

/* Get a secret value by key name. Checks the secrets file first,
   falls back to environment variable. Returns NULL if not found. */
const char *secrets_get(const char *key);

#endif /* SECRETS_H */
