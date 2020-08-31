/*
 * win32_io.c - A stdio-like disk I/O implementation for low-level disk access
 *		on Win32.  Can access an NTFS volume while it is mounted.
 *		Originated from the Linux-NTFS project.
 *
 * Copyright (c) 2003-2004 Lode Leroy
 * Copyright (c) 2003-2006 Anton Altaparmakov
 * Copyright (c) 2004-2005 Yuval Fledel
 * Copyright (c) 2012-2014 Jean-Pierre Andre
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the NTFS-3G
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#define st_blocks  st_rdev /* emulate st_blocks, missing in Windows */

#include "debug.h"
#include "types.h"
#include "device.h"
#include "misc.h"
#include "egl3interface.h"

uint8_t* ntfs_device_win32_get_sector(void* ctx, int64_t sector_addr);

void ntfs_device_win32_set_sector_FF(void* ctx, int64_t sector_addr);

int64_t* ntfs_device_win32_get_position(void* ctx);
uint64_t* ntfs_device_win32_get_written_bytes(void* ctx);

void* ntfs_device_win32_create_ctx();

/**
	* ntfs_device_win32_open - open a device
	* @dev:	a pointer to the NTFS_DEVICE to open
	* @flags:	unix open status flags
	*
	* @dev->d_name must hold the device name, the rest is ignored.
	* Supported flags are O_RDONLY, O_WRONLY and O_RDWR.
	*
	* If name is in format "(hd[0-9],[0-9])" then open a partition.
	* If name is in format "(hd[0-9])" then open a volume.
	* Otherwise open a file.
	*/
static int ntfs_device_win32_open(struct ntfs_device* dev, int flags)
{
	if (NDevOpen(dev)) {
		errno = EBUSY;
		return -1;
	}
	printf("OPENING %s\n", dev->d_name);
	dev->d_private = ntfs_device_win32_create_ctx();
	if ((flags & O_RDWR) != O_RDWR)
		NDevSetReadOnly(dev);
	NDevSetOpen(dev);
	NDevClearDirty(dev);
	return 0;
}

/**
	* ntfs_device_win32_close - close an open ntfs deivce
	* @dev:	ntfs device obtained via ->open
	*
	* Return 0 if o.k.
	*	 -1 if not, and errno set.  Note if error fd->vol_handle is trashed.
	*/
static int ntfs_device_win32_close(struct ntfs_device* dev)
{
	printf("CLOSING\nTOTAL WRITTEN: %lld\n", ntfs_device_win32_get_written_bytes(dev->d_private));
	ntfs_log_trace("Closing device %p.\n", dev);
	if (!NDevOpen(dev)) {
		errno = EBADF;
		return -1;
	}
	// not freeing up any data. it's up to ioctl AFOD request to get the data and free it themselves
	NDevClearOpen(dev);
	return 0;
}

/**
	* ntfs_device_win32_seek - change current logical file position
	* @dev:	ntfs device obtained via ->open
	* @offset:	required offset from the whence anchor
	* @whence:	whence anchor specifying what @offset is relative to
	*
	* Return the new position on the volume on success and -1 on error with errno
	* set to the error code.
	*
	* @whence may be one of the following:
	*	SEEK_SET - Offset is relative to file start.
	*	SEEK_CUR - Offset is relative to current position.
	*	SEEK_END - Offset is relative to end of file.
	*/
static s64 ntfs_device_win32_seek(struct ntfs_device* dev, s64 offset,
	int whence)
{
	//printf("SEEK TO %lld AT %d\n", offset, whence);
	int64_t* pos = ntfs_device_win32_get_position(dev->d_private);
	if (whence == SEEK_SET) {
		*pos = offset;
	}
	else {
		ntfs_log_error("BUG: seeking not using SEEK_SET (%d)\n", whence);
	}
	return *pos;
}

static s64 ntfs_device_win32_pread(struct ntfs_device* dev, void* b,
	s64 count, s64 offset)
{
	errno = EOPNOTSUPP;
	return -1;
}

/**
	* ntfs_device_win32_read - read bytes from an ntfs device
	* @dev:	ntfs device obtained via ->open
	* @b:		pointer to where to put the contents
	* @count:	how many bytes should be read
	*
	* On success returns the number of bytes actually read (can be < @count).
	* On error returns -1 with errno set.
	*/
static s64 ntfs_device_win32_read(struct ntfs_device* dev, void* b, s64 count)
{
	errno = EOPNOTSUPP;
	return -1;
}

static u8 ntfs_device_win32_is_sector_FF(const u8* b) {
	for (int n = 0; n < 512; ++n) {
		if (b[n] != 0xFF) {
			return 0;
		}
	}
	return 1;
}

static u8 ntfs_device_win32_is_sector_00(const u8* b) {
	for (int n = 0; n < 512; ++n) {
		if (b[n] != 0x00) {
			return 0;
		}
	}
	return 1;
}

static s64 ntfs_device_win32_pwrite(struct ntfs_device* dev, const u8* b,
	s64 count, s64 offset)
{
	s64 bytes_left = count;

	s64 sector_idx = offset >> 9; // divide by 512
	s64 sector_off = offset % 512;
	while (bytes_left) {
		int read_amt = min(512 - sector_off, bytes_left);
		if (read_amt != 512) {
			memcpy(ntfs_device_win32_get_sector(dev->d_private, sector_idx) + sector_off, b, read_amt);
		}
		else {
			if (ntfs_device_win32_is_sector_FF(b)) {
				ntfs_device_win32_set_sector_FF(dev->d_private, sector_idx);
			}
			else if (ntfs_device_win32_is_sector_00(b)) {
				// it's 0 by default
			}
			else {
				memcpy(ntfs_device_win32_get_sector(dev->d_private, sector_idx) + sector_off, b, read_amt);
			}
		}
		b += read_amt;
		sector_off = 0;
		bytes_left -= read_amt;
		sector_idx++;
	}

	*ntfs_device_win32_get_written_bytes(dev->d_private) += count;
	return count;
}

/**
 * ntfs_device_win32_write - write bytes to an ntfs device
 * @dev:	ntfs device obtained via ->open
 * @b:		pointer to the data to write
 * @count:	how many bytes should be written
 *
 * On success returns the number of bytes actually written.
 * On error returns -1 with errno set.
 */
static s64 ntfs_device_win32_write(struct ntfs_device* dev, const void* b,
	s64 count)
{
	s64 ret = ntfs_device_win32_pwrite(dev, b, count, *ntfs_device_win32_get_position(dev->d_private));
	if (ret != -1)
		*ntfs_device_win32_get_position(dev->d_private) += ret;
	return ret;
}

/**
 * ntfs_device_win32_sync - flush write buffers to disk
 * @dev:	ntfs device obtained via ->open
 *
 * Return 0 if o.k.
 *	 -1 if not, and errno set.
 *
 * Note: Volume syncing works differently in windows.
 *	 Disk cannot be synced in windows.
 */
static int ntfs_device_win32_sync(struct ntfs_device *dev)
{
	if (!NDevReadOnly(dev) && NDevDirty(dev)) {
		NDevClearDirty(dev);
	}
	return 0;
}

/**
 * ntfs_device_win32_stat - get a unix-like stat structure for an ntfs device
 * @dev:	ntfs device obtained via ->open
 * @buf:	pointer to the stat structure to fill
 *
 * Note: Only st_mode, st_size, and st_blocks are filled.
 *
 * Return 0 if o.k.
 *	 -1 if not and errno set. in this case handle is trashed.
 */
static int ntfs_device_win32_stat(struct ntfs_device *dev, struct stat *buf)
{
	errno = EOPNOTSUPP;
	return -1;
}

static int ntfs_device_win32_ioctl(struct ntfs_device *dev,
		unsigned long request, void *argp)
{
	if (request == 0x444F4641) { // AFOD: append file operation data
		*((void**)argp) = dev->d_private;
	}
	errno = EOPNOTSUPP;
	return -1;
}

struct ntfs_device_operations ntfs_device_win32_io_ops = {
	.open		= ntfs_device_win32_open,
	.close		= ntfs_device_win32_close,
	.seek		= ntfs_device_win32_seek,
	.read		= ntfs_device_win32_read,
	.write		= ntfs_device_win32_write,
	.pread		= ntfs_device_win32_pread,
	.pwrite		= ntfs_device_win32_pwrite,
	.sync		= ntfs_device_win32_sync,
	.stat		= ntfs_device_win32_stat,
	.ioctl		= ntfs_device_win32_ioctl
};