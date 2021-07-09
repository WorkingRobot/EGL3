#include "../interface.h"

#include "mkfs/mkexfat.h"
#include "mkfs/vbr.h"
#include "mkfs/fat.h"
#include "mkfs/cbm.h"
#include "mkfs/uct.h"
#include "mkfs/rootdir.h"

#include <inttypes.h>

// linux stuff vvv

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
typedef struct timeval {
	long tv_sec;
	long tv_usec;
} timeval;

int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}

// linux stuff ^^^

const struct fs_object* objects[] =
{
	&vbr,
	&vbr,
	&fat,
	/* clusters heap */
	&cbm,
	&uct,
	&rootdir,
	NULL,
};

static struct
{
	int sector_bits;
	int spc_bits;
	off_t volume_size;
	le16_t volume_label[EXFAT_ENAME_MAX + 1];
	uint32_t volume_serial;
	uint64_t first_sector;
}
param;

int get_sector_bits(void)
{
	return param.sector_bits;
}

int get_spc_bits(void)
{
	return param.spc_bits;
}

off_t get_volume_size(void)
{
	return param.volume_size;
}

const le16_t* get_volume_label(void)
{
	return param.volume_label;
}

uint32_t get_volume_serial(void)
{
	return param.volume_serial;
}

uint64_t get_first_sector(void)
{
	return param.first_sector;
}

int get_sector_size(void)
{
	return 1 << get_sector_bits();
}

int get_cluster_size(void)
{
	return get_sector_size() << get_spc_bits();
}

static int setup_spc_bits(int sector_bits, int user_defined, off_t volume_size)
{
	int i;

	if (user_defined != -1)
	{
		off_t cluster_size = 1 << sector_bits << user_defined;
		if (volume_size / cluster_size > EXFAT_LAST_DATA_CLUSTER)
		{
			struct exfat_human_bytes chb, vhb;

			exfat_humanize_bytes(cluster_size, &chb);
			exfat_humanize_bytes(volume_size, &vhb);
			exfat_error("cluster size %"PRIu64" %s is too small for "
					"%"PRIu64" %s volume, try -s %d",
					chb.value, chb.unit,
					vhb.value, vhb.unit,
					1 << setup_spc_bits(sector_bits, -1, volume_size));
			return -1;
		}
		return user_defined;
	}

	return MAX(0, 12 - sector_bits); // 4 kb (1<<12), that's what winspd likes working with


	if (volume_size < 256LL * 1024 * 1024)
		return MAX(0, 12 - sector_bits);	/* 4 KB */
	if (volume_size < 32LL * 1024 * 1024 * 1024)
		return MAX(0, 15 - sector_bits);	/* 32 KB */

	for (i = 17; ; i++)						/* 128 KB or more */
		if (DIV_ROUND_UP(volume_size, 1 << i) <= EXFAT_LAST_DATA_CLUSTER)
			return MAX(0, i - sector_bits);
}

static int setup_volume_label(le16_t label[EXFAT_ENAME_MAX + 1], const char* s)
{
	memset(label, 0, (EXFAT_ENAME_MAX + 1) * sizeof(le16_t));
	if (s == NULL)
		return 0;
	return exfat_utf8_to_utf16(label, s, EXFAT_ENAME_MAX + 1, strlen(s));
}

static uint32_t setup_volume_serial(uint32_t user_defined)
{
	struct timeval now;

	if (user_defined != 0)
		return user_defined;

	if (gettimeofday(&now, NULL) != 0)
	{
		exfat_error("failed to form volume id");
		return 0;
	}
	return (now.tv_sec << 20) | now.tv_usec;
}

static int setup(struct exfat_dev* dev, int sector_bits, int spc_bits,
		const char* volume_label, uint32_t volume_serial,
		uint64_t first_sector)
{
	param.sector_bits = sector_bits;
	param.first_sector = first_sector;
	param.volume_size = exfat_get_size(dev);

	param.spc_bits = setup_spc_bits(sector_bits, spc_bits, param.volume_size);
	if (param.spc_bits == -1)
		return 1;

	if (setup_volume_label(param.volume_label, volume_label) != 0)
		return 1;

	param.volume_serial = setup_volume_serial(volume_serial);
	if (param.volume_serial == 0)
		return 1;

	return mkfs(dev, param.volume_size);
}

bool EGL3CreateDisk(const char* label, EGL3File files[], uint32_t file_count, void** o_data) {
    struct exfat_dev* dev; 
    dev = exfat_open(NULL, EXFAT_MODE_RW);

    if (dev == NULL) {
        return false;
    }

	int spc_bits = -1;
	const char* volume_label = NULL;
	uint32_t volume_serial = 0;
	uint64_t first_sector = 1;

	if (setup(dev, 12, spc_bits, label, volume_serial, first_sector) != 0) {
		exfat_close(dev);
		return false;
	}

	*o_data = exfat_get_io_fd(dev);

	if (exfat_close(dev) != 0) {
		return false;
	}
	
	struct exfat ef;
	if (exfat_mount(&ef, *o_data, "rw") != 0) {
		return false;
	}

	for (uint32_t i = 0; i < file_count; ++i) {
		EGL3File* file = files + i;
		exfat_mknod_reserve(&ef, file->path, file->size, &file->run_idx, &file->run_size);
	}

	exfat_unmount(&ef);

    return true;
}