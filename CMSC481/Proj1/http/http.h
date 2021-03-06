#ifndef HTTP_H
#define HTTP_H

#include "http_url.h"
#include "log.h"

#define SUCCESS 0
#define FAIL -1
#define RETRY -2

typedef enum { DISCONNECTED, CONNECTED, REQUEST_SENT, HEADER_RECEIVED, HEADER_PARSED } http_status;
typedef enum { GET, HEAD, POST } http_method;

typedef struct {
	http_status status;
	http_url_t *url;
	http_method method;
	void *post_data;
	size_t post_data_size;
	int num_retry;
	log_t *log;
	int sockfd;
	char *header;
	int content_length;
	int received;
} http_t;

void http_init(http_t *http, http_url_t *url, http_method method, void *post_data, size_t post_data_size, int num_retry, log_t *log);
void http_run(http_t *http);
int http_connect(http_t *http);
int http_send_request(http_t *http);
int http_recv_header(http_t *http);
int http_parse_header(http_t *http);
int http_save_file(http_t *http, int fd);
int http_disconnect(http_t *http);

#endif
