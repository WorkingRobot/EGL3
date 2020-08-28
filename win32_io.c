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

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#define st_blocks  st_rdev /* emulate st_blocks, missing in Windows */

#include "debug.h"
#include "types.h"
#include "device.h"
#include "misc.h"

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
static int ntfs_device_win32_open(struct ntfs_device *dev, int flags)
{

	if (NDevOpen(dev)) {
		errno = EBUSY;
		return -1;
	}
	const char* mode = NULL;
	if (flags == O_RDONLY) {
		mode = "rb";
	}
	else if (flags == O_WRONLY) {

	}
	FILE* fd = fopen(dev->d_name, flags == 0 ? "rb" : "rb+");
	fseek(fd, 0, SEEK_SET);
	/* Setup our read-only flag. */
	if ((flags & O_RDWR) != O_RDWR)
		NDevSetReadOnly(dev);
	dev->d_private = fd;
	NDevSetOpen(dev);
	NDevClearDirty(dev);
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
static s64 ntfs_device_win32_seek(struct ntfs_device *dev, s64 offset,
		int whence)
{
	return fseek(dev->d_private, offset, whence);
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
static s64 ntfs_device_win32_read(struct ntfs_device *dev, void *b, s64 count)
{
	return fread(b, 1, count, dev->d_private);
}

/**
 * ntfs_device_win32_close - close an open ntfs deivce
 * @dev:	ntfs device obtained via ->open
 *
 * Return 0 if o.k.
 *	 -1 if not, and errno set.  Note if error fd->vol_handle is trashed.
 */
static int ntfs_device_win32_close(struct ntfs_device *dev)
{
	BOOL rvl;

	ntfs_log_trace("Closing device %p.\n", dev);
	if (!NDevOpen(dev)) {
		errno = EBADF;
		return -1;
	}
	fclose(dev->d_private);
	NDevClearOpen(dev);
	return 0;
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
		fflush(dev->d_private);
		NDevClearDirty(dev);
	}
	return 0;
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
static s64 ntfs_device_win32_write(struct ntfs_device *dev, const void *b,
		s64 count)
{
	return fwrite(b, 1, count, dev->d_private);
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
	memset(buf, 0, sizeof(struct stat));
	buf->st_mode = S_IFCHR;
	s64 curOff = ftell(dev->d_private);
	fseek(dev->d_private, 0, SEEK_END);
	buf->st_size = ftell(dev->d_private);
	fseek(dev->d_private, curOff, SEEK_SET);
	if (buf->st_size != -1)
		buf->st_blocks = buf->st_size >> 9;
	else
		buf->st_size = 0;
	return 0;
}

static int ntfs_device_win32_ioctl(struct ntfs_device *dev,
		unsigned long request, void *argp)
{
	ntfs_log_trace("win32_ioctl(0x%lx) called.\n", request);
	switch (request) {
#if defined(BLKGETSIZE)
	case BLKGETSIZE:
	{
		ntfs_log_debug("BLKGETSIZE detected.\n");
		s64 curPos = ftell(dev->d_private);
		fseek(dev->d_private, 0, SEEK_END);
		*(int*)argp = (int)(ftell(dev->d_private) / 512);
		fseek(dev->d_private, curPos, SEEK_SET);
		return 0;
	}
#endif
#if defined(BLKGETSIZE64)
	case BLKGETSIZE64:
	{
		ntfs_log_debug("BLKGETSIZE64 detected.\n");
		s64 curPos = ftell(dev->d_private);
		fseek(dev->d_private, 0, SEEK_END);
		*(s64*)argp = (s64)ftell(dev->d_private);
		fseek(dev->d_private, curPos, SEEK_SET);
		return 0;
	}
#endif
#ifdef HDIO_GETGEO
	case HDIO_GETGEO:
		ntfs_log_debug("HDIO_GETGEO detected.\n");
		errno = EOPNOTSUPP;
		return -1;
#endif
#ifdef BLKSSZGET
	case BLKSSZGET:
		ntfs_log_debug("BLKSSZGET detected.\n");
		*(int*)argp = 512;
		return 0;
#endif
#ifdef BLKBSZSET
	case BLKBSZSET:
		ntfs_log_debug("BLKBSZSET detected.\n");
		/* Nothing to do on Windows. */
		return 0;
#endif
	default:
		ntfs_log_debug("unimplemented ioctl 0x%lx.\n", request);
		errno = EOPNOTSUPP;
		return -1;
	}
}

static s64 ntfs_device_win32_pread(struct ntfs_device *dev, void *b,
		s64 count, s64 offset)
{
	s64 preOff = ftell(dev->d_private);
	fseek(dev->d_private, offset, SEEK_SET);
	s64 ret = fread(b, 1, count, dev->d_private);
	fseek(dev->d_private, preOff, SEEK_SET);
	return ret;
}

static s64 ntfs_device_win32_pwrite(struct ntfs_device *dev, const void *b,
		s64 count, s64 offset)
{
	s64 preOff = ftell(dev->d_private);
	fseek(dev->d_private, offset, SEEK_SET);
	s64 ret = fwrite(b, 1, count, dev->d_private);
	fseek(dev->d_private, preOff, SEEK_SET);
	return ret;
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
