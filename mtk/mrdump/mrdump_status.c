#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <memory.h>
#include <stdint.h>
#include <log/log.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <mrdump_user.h>

#define MRDUMP_VERSION_FILE "/sys/module/mrdump/version"

static const char *expdb_devs[] = {
    "/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/expdb",
    "/dev/block/platform/mtk-msdc.0/11230000.MSDC0/by-name/expdb",
    NULL
};

#define MRDUMP_OFFSET 3145728

#define MRDUMP_SIG "MRDUMP04"
struct __attribute__((__packed__)) mrdump_cblock_result {
    char sig[9];
    char status[128];
    char log_buf[2048];
};

struct partinfo {
    int fd;
    uint64_t size;
    uint32_t blksize;
};

static int expdb_open(struct partinfo *partinfo)
{
    int fd, i;
    uint64_t part_size;
    uint32_t part_blksize;

    memset(partinfo, 0, sizeof(struct partinfo));

    for (i = 0; expdb_devs[i] != NULL; i++) {
	if ((fd = open(expdb_devs[i], O_RDWR)) < 0) {
	    ALOGI("%s: open %s failed(%d)", __func__, expdb_devs[i], errno);
	    continue;
	};
	if (ioctl(fd, BLKGETSIZE64, &part_size) < 0) {
	    ALOGE("%s, get expdb partition size fail(%d)", __func__, errno);
	    close(fd);
	    return -1;
	}

	if (ioctl(fd, BLKSSZGET, &part_blksize) < 0) {
	    ALOGE("%s, get sector size fail(%d)", __func__, errno);
	    close(fd);
	    return -1;
	}
	partinfo->fd = fd;
	partinfo->size = part_size;
	partinfo->blksize = part_blksize;
	return 0;
    }
    ALOGE("%s: No expdb partition found", __func__);
    return -1;
}

int mrdump_is_supported(void)
{
    FILE *fp = fopen(MRDUMP_VERSION_FILE, "r");
    if (fp != NULL) {
	char ver_str[16];
	if (fgets(ver_str, sizeof(ver_str), fp) != NULL) {
	    ALOGI("MT-RADMUMP support version %s", ver_str);
	    fclose(fp);
	    return 1;
	}
	fclose(fp);
	return 0;
    }
    else {
	return 0;
    }
}

bool mrdump_status_clear(void)
{
    struct partinfo partinfo;

    if (expdb_open(&partinfo) >= 0) {
	if (lseek64(partinfo.fd, partinfo.size - MRDUMP_OFFSET, SEEK_SET) < 0) {
	    ALOGE("%s: Can't seek part fd %d\n", __func__, partinfo.fd);
	    close(partinfo.fd);
	    return false;
	}

	struct mrdump_cblock_result cblock_result;
	if (read(partinfo.fd, &cblock_result, sizeof(struct mrdump_cblock_result)) != sizeof(struct mrdump_cblock_result)) {
	    ALOGE("%s: Can't read part fd %d\n", __func__, partinfo.fd);
	    close(partinfo.fd);
	    return false;
	}
	memset(cblock_result.status, 0, sizeof(cblock_result.status));
	strcpy(cblock_result.status, "CLEAR");

	if (lseek64(partinfo.fd, partinfo.size - MRDUMP_OFFSET, SEEK_SET) < 0) {
	    ALOGE("%s: Can't seek part fd %d\n", __func__, partinfo.fd);
	    close(partinfo.fd);
	    return false;
	}
	if (write(partinfo.fd, &cblock_result, sizeof(struct mrdump_cblock_result)) != sizeof(struct mrdump_cblock_result)) {
	    ALOGE("%s: Can't write part fd %d\n", __func__, partinfo.fd);
	    close(partinfo.fd);
	    return false;
	}
	close(partinfo.fd);
	return true;
    }
    return false;
}

bool mrdump_status_get(struct mrdump_status_result *result)
{
    memset(result, 0, sizeof(struct mrdump_status_result));
    result->struct_size = sizeof(struct mrdump_status_result);

    struct partinfo partinfo;
    if (expdb_open(&partinfo) >= 0) {
	if (lseek64(partinfo.fd, partinfo.size - MRDUMP_OFFSET, SEEK_SET) < 0) {
	    ALOGE("%s: Can't seek part fd %d\n", __func__, partinfo.fd);
	    close(partinfo.fd);
	    return false;
	}

	struct mrdump_cblock_result cblock_result;
	if (read(partinfo.fd, &cblock_result, sizeof(struct mrdump_cblock_result)) != sizeof(struct mrdump_cblock_result)) {
	    ALOGE("%s: Can't read part fd %d\n", __func__, partinfo.fd);
	    close(partinfo.fd);
	    return false;
	}
	close(partinfo.fd);

	if (strcmp(cblock_result.sig, MRDUMP_SIG) != 0) {
	    ALOGE("%s: Signature mismatched\n", __func__);
	    return false;
	}
	/* Copy/parsing status line */
	strlcpy(result->status_line, cblock_result.status, sizeof(result->status_line));

	char *saveptr;
	cblock_result.status[sizeof(cblock_result.status) - 1] = 0;
	char *strval = strtok_r(cblock_result.status, "\n", &saveptr);
	if (strval != NULL) {
	    if (strcmp(strval, "OK") == 0) {
		result->status = MRDUMP_STATUS_OK;
		result->output = MRDUMP_OUTPUT_NULL;

		do {
		    strval = strtok_r(NULL, "\n", &saveptr);
		    if (strval != NULL) {
                        if (strncmp(strval, "OUTPUT:", 7) == 0) {
			    if (strcmp(strval + 7, "EXT4_DATA") == 0) {
				result->output = MRDUMP_OUTPUT_EXT4_DATA;
			    }
			    else if (strcmp(strval + 7, "VFAT_INT_STORAGE") == 0) {
				result->output = MRDUMP_OUTPUT_VFAT_INT_STORAGE;
			    }
			    else {
				return false;
			    }
			}
			else if (strncmp(strval, "MODE:", 5) == 0) {
			    strlcpy(result->mode, strval + 5, sizeof(result->mode));
			}
		    }
		} while (strval != NULL);
	    }
	    else if (strcmp(strval, "NONE") == 0) {
		result->status = MRDUMP_STATUS_NONE;
	    }
	    else if (strcmp(strval, "CLEAR") == 0) {
		result->status = MRDUMP_STATUS_NONE;
	    }
	    else {
		result->status = MRDUMP_STATUS_FAILED;
	    }
	}
	else {
	    ALOGE("%s: status parsing error \"%s\"\n", __func__, cblock_result.status);
	    return false;
	}

	strlcpy(result->log_buf, cblock_result.log_buf, sizeof(result->log_buf));
	return true;
    }
    return false;
}
