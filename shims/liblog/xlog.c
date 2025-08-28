#ifndef __unused
#define __unused  __attribute__((__unused__))
#endif

struct xlog_record {
	const char *tag_str;
	const char *fmt_str;
	int prio;
};

int __xlog_buf_printf(int bufid __unused, const struct xlog_record *xlog_record __unused, ...) {
	return 0;
}
