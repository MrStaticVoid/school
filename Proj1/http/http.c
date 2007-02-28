#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "http.h"

#define RETRY_SLEEP 5

static char *http_method_strings[] = {"GET", "HEAD", "POST"};

void http_init(http_t *http, http_url_t *url, log_t *log) {
	http->status = DISCONNECTED;
	http->url = url;
	http->log = log;
	http->sockfd = -1;
}

int http_go(http_t *http, http_method method, void *post_data, size_t post_data_size, int num_retry) {
	int ret, connect_try, request_try, disconnect_try;

	connect_try = 0;
	request_try = 0;

CONNECT:
	connect_try++;
	ret = http_connect(http);

	if (ret == FAIL || connect_try >= num_retry)
		return FAIL;
	else if (ret == RETRY) {
		log_printf(http->log, INFO, "http_go", "Retrying http_connect %d of %d times in %d seconds...", connect_try, num_retry, RETRY_SLEEP);
		sleep(5);
		goto CONNECT;
	}

SEND_REQUEST:
	request_try++;
	ret = http_send_request(http, method, post_data, post_data_size);

	if (ret == FAIL || request_try >= num_retry)
		return FAIL;
	else if (ret == RETRY) {
		http_disconnect(http);
		log_printf(http->log, INFO, "http_go", "Retrying http_send_request %d of %d times in %d seconds...", request_try, num_retry, RETRY_SLEEP);
		sleep(5);
		goto CONNECT;
	}

	return http_disconnect(http);
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

int http_send_request(http_t *http, http_method method, void *post_data, size_t post_data_size) {
	struct timeval starttime, endtime;
	char *request_line;
	int sent, pos;

	if (http->status == DISCONNECTED) {
		log_printf(http->log, INFO, "http_send_request", "Disconnected");
		return FAIL;
	} else if (http->status > CONNECTED) {
		log_printf(http->log, INFO, "http_send_request", "Request already sent");
		return FAIL;
	}
	
	gettimeofday(&starttime, NULL);

	log_printf(http->log, DETAILS, "http_send_request", "Sending %s request...", http_method_strings[method]);

	request_line = (char*) malloc(strlen(http->url->abs_path) + strlen(http->url->query) + 17);
	sprintf(request_line, "%s %s?%s HTTP/1.0\r\n", http_method_strings[method], http->url->abs_path, http->url->query);

	for (pos = 0; pos < strlen(request_line); pos += sent) {
		log_printf(http->log, DETAILS, "http_send_request", "Calling send");
		if ((sent = send(http->sockfd, request_line + pos, strlen(request_line + pos), 0)) == -1) {
			log_perror(http->log, "send");
			return RETRY;
		}
	}

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