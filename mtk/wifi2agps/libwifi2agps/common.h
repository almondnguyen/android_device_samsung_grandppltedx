#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__
#include <stdlib.h>
#include <errno.h>

#include <android/log.h>
typedef int s32;
typedef unsigned int u32;
typedef short s16;
typedef unsigned short u16;
typedef char s8;
typedef unsigned char u8;
typedef long time_t;
enum {MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR};
static int tranlate_to_android_level(u32 level)
{
	if (level == MSG_ERROR)
		return ANDROID_LOG_ERROR;
	if (level == MSG_WARNING)
		return ANDROID_LOG_WARN;
	if (level == MSG_INFO)
		return ANDROID_LOG_INFO;
	return ANDROID_LOG_DEBUG;
}

static void wifi2agps_log(u32 level, s8* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	__android_log_vprint(tranlate_to_android_level(level),
				     "wifi2agps", fmt, ap);
}
#endif
