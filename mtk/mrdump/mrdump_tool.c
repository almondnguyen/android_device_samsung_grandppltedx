#include <stdio.h>
#include <string.h>

#include "mrdump_user.h"

static void usage(const char *prog)
{
    printf("Usage\n"
           "\t%s is-supported\n"
	   "\t%s status-get\n"
	   "\t%s status-log\n"
	   "\t%s status-clear\n",
	   prog, prog, prog, prog);
}

static void dump_status_ok(const struct mrdump_status_result *result)
{
    printf("Ok\n");
    printf("\tMode: %s\n\tOutput: ", result->mode);

    switch (result->output) {
    case MRDUMP_OUTPUT_NULL:
        printf("null\n");
        break;
    case MRDUMP_OUTPUT_USB:
        printf("usb\n");
        break;
    case MRDUMP_OUTPUT_EXT4_DATA:
        printf("ext4/data partition\n");
        break;
    case MRDUMP_OUTPUT_VFAT_INT_STORAGE:
        printf("vfat/internal partition\n");
        break;
    }
}

int main(int argc, char *argv[])
{
    if (argc == 2) {
        if (strcmp(argv[1], "is-supported") == 0) {
            if (mrdump_is_supported()) {
                printf("MT-RAMDUMP support ok\n");
            }
            else {
                printf("MT-RAMDUMP not support\n");
            }
        }
	else if (strcmp(argv[1], "status-get") == 0) {
	    struct mrdump_status_result result;
	    if (mrdump_status_get(&result)) {
		printf("MT-RAMDUMP\n\tStatus:");
		switch (result.status) {
		case MRDUMP_STATUS_NONE:
		    printf("None\n");
		    break;
		case MRDUMP_STATUS_FAILED:
		    printf("Failed\n");
		    break;
		case MRDUMP_STATUS_OK:
                    dump_status_ok(&result);
		    break;
		}

	    }
	    else {
		printf("MT-RAMDUMP get status failed\n");
	    }
	}
	else if (strcmp(argv[1], "status-log") == 0) {
	    struct mrdump_status_result result;
	    if (mrdump_status_get(&result)) {
		printf("status line:\n%s\nlog:\n%s\n", result.status_line, result.log_buf);
	    }
	    else {
		printf("MT-RAMDUMP get status failed\n");
	    }
	}
	else if (strcmp(argv[1], "status-clear") == 0) {
	    if (!mrdump_status_clear()) {
		printf("MT-RAMDUMP Status clear failed\n");
	    }
	}
	else {
	    usage(argv[0]);
	}
    }
    else {
	usage(argv[0]);
    }
    return 0;
}
