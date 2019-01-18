#include <cutils/log.h>

struct xlog_record {
	const char *tag_str;
	const char *fmt_str;
	int prio;
};

int __xlog_buf_printf(int bufid, const struct xlog_record *rec, ...) {
  va_list args;
  va_start(args, rec);
  LOG_PRI_VA(rec->prio, rec->tag_str, rec->fmt_str, args);
  va_end(args);

  return 0;
}
