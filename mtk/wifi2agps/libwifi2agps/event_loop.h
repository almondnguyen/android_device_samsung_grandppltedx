#include "common.h"

struct time_value {
	u32 second;
	u32 usecond;
};

struct socket_data {
	struct socket_data *next;
	s32 sock_fd;
	s32 (*cb)(void* loop_context, void *user_data);
	void *loop_context;
	void *user_data;
	struct time_value timeout;
};
s32 event_loop_add_event_timeout (s32 sock, void *loop_context, void* user_data, 
					s32 (*cb)(void* loop_context, void *user_data), u32 sec, u32 usec);
s32 event_loop_add_event (s32 sock, void *loop_context, void* user_data, 
					s32 (*cb)(void* loop_context, void *user_data));
s32 event_loop_remove_event(s32 sock);
void event_loop_terminate();
void event_loop_run();
