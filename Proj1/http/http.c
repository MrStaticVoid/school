#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "http.h"

#define HOST_HEADER "Host: "
#define CONTENT_LENGTH_HEADER "Content-Length: "

static char *http_method_strings[] = {"GET", "HEAD", "POST"};

void http_init(http_t *http, http_url_t *url, http_method method, void *post_data, size_t post_data_size, int num_retry, log_t *log) {
	http->status = DISCONNECTED;
	http->url = url;
	http->method = method;
	http->post_data = post_data;
	http->post_data_size = post_data_size;
	http->num_retry = num_retry;
	http->log = log;
	http->sockfd = -1;
}

int http_connect(http_t *http) {
	struct timeval starttime, endtime;
	struct hostent *he;
	struct sockaddr_in addr;
	char *ip;

	if (http->status >= CONNECTED) {
		log_printf(http->log, INFO, "http_connect", "Already connected");
		return FAIL;
	}

	gettimeofday(&starttime, NULL);

	log_printf(http->log, DETAILS, "http_connect", "Connecting to %s:%d...", http->url->host, http->url->port);

	log_printf(http->log, DETAILS, "http_connect", "Calling gethostbyname");
	if (!(he = gethostbyname(http->url->host))) {
		log_herror(http->log, "gethostbyname");
		if (h_errno == TRY_AGAIN)
			return RETRY;
		return FAIL;
	}

	ip = inet_ntoa(*((struct in_addr*) he->h_addr));
	if (strcmp(ip, http->url->host))
		log_printf(http->log, DETAILS, "http_connect", "%s is %s", http->url->host, ip);

	log_printf(http->log, DETAILS, "http_connect", "Calling socket");
	if ((http->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_perror(http->log, "socket");
		return FAIL;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(http->url->port);
	addr.sin_addr = *((struct in_addr*) he->h_addr);
	memset(&addr.sin_zero, '\0', 8);

	log_printf(http->log, DETAILS, "http_connect", "Calling connect");
	if (connect(http->sockfd, (struct sockaddr*) &addr, sizeof(struct sockaddr)) == -1) {
		log_perror(http->log, "connect");
		return RETRY;
	}

	gettimeofday(&endtime, NULL);
	log_printf(http->log, DETAILS, "http_connect", "Connected in %d ms.", endtime.tv_usec / 1000 - starttime.tv_usec / 1000);

	http->status = CONNECTED;

	return SUCCESS;
}

int http_send(http_t *http, void *data, size_t size) {
	int sent, pos;

	if (http->status == DISCONNECTED) {
		log_printf(http->log, INFO, "http_send", "Disconnected");
		return FAIL;
	}

	for (pos = 0; pos < size; pos += sent) {
		log_printf(http->log, DETAILS, "http_send", "Calling send");
		if ((sent = send(http->sockfd, data + pos, size - pos, 0)) == -1) {
			log_perror(http->log, "send");
			return RETRY;
		}
	}

	return SUCCESS;
}

int http_send_request(http_t *http) {
	struct timeval starttime, endtime;
	char *request_line;
	int ret;

	if (http->status == DISCONNECTED) {
		log_printf(http->log, INFO, "http_send_request", "Disconnected");
		return FAIL;
	} else if (http->status > CONNECTED) {
		log_printf(http->log, INFO, "http_send_request", "Request already sent");
		return FAIL;
	}
	
	gettimeofday(&starttime, NULL);

	log_printf(http->log, DETAILS, "http_send_request", "Sending %s request...", http_method_strings[http->method]);

	request_line = (char*) malloc(strlen(http->url->abs_path) + strlen(http->url->query) + 18);
	sprintf(request_line, "%s %s?%s HTTP/1.0\r\n", http_method_strings[http->method], http->url->abs_path, http->url->query);
	if (ret = http_send(http, request_line, strlen(request_line))) return ret;
	free(request_line);

	request_line = (char*) malloc(strlen(HOST_HEADER) + strlen(http->url->host) + 3);
	sprintf(request_line, "%s%s\r\n", HOST_HEADER, http->url->host);
	if (ret = http_send(http, request_line, strlen(request_line))) return ret;
	free(request_line);

	if (http->method == POST) {
		request_line = (char*) malloc(strlen(CONTENT_LENGTH_HEADER) + 15);
		sprintf(request_line, "%s%d\r\n\r\n", CONTENT_LENGTH_HEADER, http->post_data_size);
		if (ret = http_send(http, request_line, strlen(request_line))) return ret;
		free(request_line);

		if (ret = http_send(http, http->post_data, http->post_data_size)) return ret;
	}
	
	http_send(http, "\r\n", 2);

	gettimeofday(&endtime, NULL);
	log_printf(http->log, DETAILS, "http_send_request", "Request sent in %d ms.", endtime.tv_usec / 1000 - starttime.tv_usec / 1000);

	http->status = REQUEST_SENT;

	return SUCCESS;
}

int http_disconnect(http_t *http) {
	struct timeval starttime, endtime;

	if (http->status == DISCONNECTED) {
		log_printf(http->log, INFO, "http_disconnect", "Already disconnected");
		return FAIL;
	}
	
	gettimeofday(&starttime, NULL);

	log_printf(http->log, DETAILS, "http_disconnect", "Disconnecting...");

	log_printf(http->log, DETAILS, "http_disconnect", "Calling close");
	if (close(http->sockfd)) {
		log_perror(http->log, "close");
		return FAIL;
	}
	http->sockfd = -1;

	gettimeofday(&endtime, NULL);
	log_printf(http->log, DETAILS, "http_disconnect", "Disconnected in %d ms.", endtime.tv_usec / 1000 - starttime.tv_usec / 1000);

	http->status = DISCONNECTED;

	return SUCCESS;
}
