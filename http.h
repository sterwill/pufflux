#ifndef __HTTP_H
#define __HTTP_H

#include <WiFi101.h>

#define HTTP_STATUS_CONNECT_ERR                 -1
#define HTTP_STATUS_MALFROMED_RESPONSE_LINE     -2
#define HTTP_STATUS_MALFROMED_RESPONSE_HEADER   -3

typedef enum {
  HTTP_METHOD_NEW,
  HTTP_METHOD_READING_RESPONSE_HEADERS,
  HTTP_METHOD_READING_RESPONSE_BODY,
  HTTP_METHOD_DONE,
  HTTP_METHOD_CLOSED,
} http_method_state;


// Predefined for self-reference in callbacks
typedef struct http_request http_request;

struct http_request {
  // Caller fills these fields
  const char *host;
  uint16_t port;
  bool ssl;
  const char *path_and_query;
  struct http_key_value **headers;

  void (*header_cb)(http_request *req, const char *header, const char *value);

  void (*body_cb)(http_request *req);

  void *caller_ctx;

  // HTTP methods fill these fields
  int id;
  int status;

  // Valid during callback execution
  WiFiClient *client;
};

typedef struct {
  char scheme[6];
  char host[64];
  uint16_t port;
  char path_and_query[256];
} url_parts;

void http_request_init(http_request *req);
void http_get(http_request *req);

#endif /* __HTTP_H */
