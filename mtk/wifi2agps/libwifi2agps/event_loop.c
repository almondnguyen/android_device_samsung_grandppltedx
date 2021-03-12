#include "event_loop.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

static struct socket_data *wait_context;
static struct socket_data *wait_context_no_timeout;
static s32 max_fd = 0;
static s8 gexit = 0;
s32 time_before(struct time_value *a, struct time_value *b) {
	if (a->second > b->second || 
		((a->second = b->second) && (a->usecond > b->usecond)))
		return 0;
	return 1;
}
s32 event_loop_add_event(s32 sock,void * loop_context,void * user_data,s32(* cb)(void * loop_context,void * user_data)) {
	return event_loop_add_event_timeout(sock, loop_context, user_data, cb, 0, 0);
}
s32 event_loop_add_event_timeout (s32 sock, void *loop_context, void* user_data, 
					s32 (*cb)(void* loop_context, void *user_data), u32 sec, u32 usec) {
	struct socket_data *new_data;
	struct socket_data *tail;
	struct timeval tv;

	new_data = (struct socket_data *)malloc(sizeof(struct socket_data));
	memset(new_data, 0, sizeof(struct socket_data));
	if (sock > max_fd)
		max_fd = sock;
	
	new_data->loop_context = loop_context;
	new_data->cb = cb;
	new_data->sock_fd = sock;
	new_data->user_data = user_data;
	new_data->next = NULL;
	if (sec ==0 && usec == 0) {
		if (wait_context_no_timeout == NULL) {
			wait_context_no_timeout = new_data;
		} else {
			tail = wait_context_no_timeout;
			while (tail->next) tail = tail->next;
			tail->next = new_data;
		}
		wifi2agps_log(MSG_DEBUG, "add no timeout event, sock=%d", sock);
		return 0;
	}
	wifi2agps_log(MSG_DEBUG, "add timeout event, sock=%d, sec=%u, usec=%u", sock, sec, usec);
	gettimeofday(&tv, NULL);
	new_data->timeout.second = sec + tv.tv_sec;
	new_data->timeout.usecond = usec + tv.tv_usec;
	if (wait_context == NULL) {
		wait_context = new_data;
		return 0;
	}
	if (time_before(&new_data->timeout, &wait_context->timeout)) {
		new_data->next = wait_context;
		wait_context = new_data;
		return 0;
	}
	tail = &wait_context;
	while (tail) {
		if (time_before(&new_data->timeout, &tail->next->timeout)) {
			new_data->next = tail->next;
			tail->next = new_data;
			return 0;
		}
	}
	tail->next = new_data;
	return 0;
}

s32 event_loop_remove_event(s32 sock) {
	struct socket_data *tail = wait_context;
	if (tail->sock_fd == sock)
		goto found;
	while (tail->next) {
		if (tail->next->sock_fd == sock) {
			struct socket_data *tmp = tail->next;
			tail->next = tmp->next;
			tail = tmp;
			goto found;
		}
		tail = tail->next;
	}
	return 0;
found:
	free(tail);
	return 1;
}

void event_loop_terminate() {
	gexit = 1;
}
void event_loop_run() {
	struct socket_data *tail;
	fd_set read_fds;
	struct timeval tv;
	struct time_value current;
	s8 timeout = 0;
	s32 res;
	/*wifi2agps_log(MSG_DEBUG, "wait_context=%p, wait_context_no_timeout=%p",
					wait_context, wait_context_no_timeout);*/
	while (!gexit && (wait_context || wait_context_no_timeout)) {
		tail = wait_context_no_timeout;
		FD_ZERO(&read_fds);
		while (tail) {
			/*wifi2agps_log(MSG_DEBUG, "tail->sock_fd=%d",
					tail->sock_fd);*/
			FD_SET(tail->sock_fd, &read_fds);
			tail = tail->next;
		}
		if (wait_context) {
			gettimeofday(&tv, NULL);
			current.second = tv.tv_sec;
			current.usecond = tv.tv_usec;
			//wifi2agps_log(MSG_DEBUG, "sec1=%u, sec2=%u", wait_context->timeout.second, current.second);
			if (time_before(&wait_context->timeout, &current))
				timeout = 0;
			else {
				if (wait_context->timeout.usecond < current.usecond) {
					tv.tv_usec = wait_context->timeout.usecond-current.usecond+1000000;
					tv.tv_sec = wait_context->timeout.second-current.second-1;
				}
				else {
					tv.tv_usec = wait_context->timeout.usecond-current.usecond;
					tv.tv_sec = wait_context->timeout.second-current.second;
				}
				timeout = 1;
			}
			//wifi2agps_log(MSG_DEBUG, "timeout=%d, sec=%u", timeout, tv.tv_sec);
			FD_SET(wait_context->sock_fd, &read_fds);
			
		}
		res = select(max_fd + 1, &read_fds, NULL, NULL, timeout ? &tv:NULL);
		//wifi2agps_log(MSG_DEBUG, "res=%d, errno=%s", res, strerror(errno));
		if (res <= 0 && wait_context) {
			gettimeofday(&tv, NULL);
			current.second = tv.tv_sec;
			current.usecond = tv.tv_usec;
			if (time_before(&wait_context->timeout, &current)) {
				wait_context->cb(wait_context->loop_context, wait_context->user_data);
				tail = wait_context;
				wait_context = wait_context->next;
				free(tail);
			}
			continue;
		}
		tail = wait_context_no_timeout;
		while (tail) {
			if (FD_ISSET(tail->sock_fd, &read_fds))
				tail->cb(tail->loop_context, tail->user_data);
			tail = tail->next;
		}
	}
	/*wifi2agps_log(MSG_INFO, "wifi2agps terminated, exit=%d, wait_context=%p,"
		"wait_context_no_timeout=%p", gexit, wait_context, wait_context_no_timeout);*/
	tail = wait_context;
	while (wait_context) {
		tail = wait_context->next;
		free(wait_context);
		wait_context = tail;
	}
	tail = wait_context_no_timeout;
	while (wait_context_no_timeout) {
		tail = wait_context_no_timeout->next;
		free(wait_context_no_timeout);
		wait_context_no_timeout = tail;
	}
}
