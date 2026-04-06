#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

/* Dynamic buffer for HTTP response data */
typedef struct {
  char *data;
  size_t size;
  size_t capacity;
} HttpBuffer;

/* Initialize an HTTP buffer */
void http_buffer_init(HttpBuffer *buf);

/* Free an HTTP buffer */
void http_buffer_free(HttpBuffer *buf);

/* Perform an HTTP GET request. Returns 0 on success, -1 on failure. */
int http_get(const char *url, HttpBuffer *response);

/* Perform an HTTP POST request with JSON body. Returns 0 on success, -1 on
 * failure. */
int http_post_json(const char *url, const char *json_body,
                   const char *auth_header, HttpBuffer *response);

#endif /* HTTP_H */
