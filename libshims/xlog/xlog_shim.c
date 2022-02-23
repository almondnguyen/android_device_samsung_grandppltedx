#define LOG_TAG "xlog"

#include <log/log.h>
#include <cutils/log.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define XLOG_DEVICE "/proc/xlog/setfil"

#define XLOG_NAME_MAX_LEN 64

#define XLOG_SET_LEVEL       12
#define XLOG_GET_LEVEL       13
#define XLOG_SET_TAG_LEVEL   14

#define XLOG_FILTER_DEFAULT_LEVEL 0x00223222

#ifdef __cplusplus
extern "C"
{
#endif

struct xlog_entry {
	char name[XLOG_NAME_MAX_LEN];
	uint32_t level;
};

int xlogf_set_level(uint32_t level)
{
    int ret = -1;
    int fd = open(XLOG_DEVICE, O_RDONLY);
    if (fd >= 0) {
        if (ioctl(fd, XLOG_SET_LEVEL, level) == 0) {
            ret = 0;
        }
        close(fd);
    }
    return ret;
}

uint32_t xlogf_get_level(void)
{
    uint32_t level = XLOG_FILTER_DEFAULT_LEVEL;
    int fd = open(XLOG_DEVICE, O_RDONLY);
    if (fd >= 0) {
        ioctl(fd,XLOG_GET_LEVEL, &level);
        close(fd);
    }

    return level;
}

int xlogf_tag_set_level(const char *name, uint32_t level)
{
    int ret = -1;
    int fd = open(XLOG_DEVICE, O_RDONLY);
    if (fd >= 0) {
        struct xlog_entry ent;

        memset(&ent, 0, sizeof(sizeof(struct xlog_entry)));
        strlcpy(ent.name, name, XLOG_NAME_MAX_LEN);
        ent.level = level;
        if (ioctl(fd, XLOG_SET_TAG_LEVEL, &ent) == 0) {
            ret = 0;
        }
        close(fd);
    }
    return ret;
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
{
#endif

struct xlog_record {
	const char *tag_str;
	const char *fmt_str;
	int prio;
};

static void init(void) __attribute__ ((constructor));

void init(void){}

int __xlog_buf_printf(int bufid, const struct xlog_record *rec, ...)
{
  va_list args;
  va_start(args, rec);
  LOG_PRI_VA(rec->prio, rec->tag_str, rec->fmt_str, args);
  va_end(args);

  return 0;
}

#ifdef __cplusplus
}
#endif

