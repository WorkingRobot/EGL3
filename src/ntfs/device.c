/**
 * device.c - Low level device io functions. Originated from the Linux-NTFS project.
 *
 * Copyright (c) 2004-2013 Anton Altaparmakov
 * Copyright (c) 2004-2006 Szabolcs Szakacsits
 * Copyright (c) 2010      Jean-Pierre Andre
 * Copyright (c) 2008-2013 Tuxera Inc.
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

#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "types.h"
#include "mst.h"
#include "debug.h"
#include "device.h"
#include "support.h"
#include "logging.h"
#include "misc.h"

/**
 * ntfs_device_alloc - allocate an ntfs device structure and pre-initialize it
 * @name:	name of the device (must be present)
 * @state:	initial device state (usually zero)
 * @dops:	ntfs device operations to use with the device (must be present)
 * @priv_data:	pointer to private data (optional)
 *
 * Allocate an ntfs device structure and pre-initialize it with the user-
 * specified device operations @dops, device state @state, device name @name,
 * and optional private data @priv_data.
 *
 * Note, @name is copied and can hence be freed after this functions returns.
 *
 * On success return a pointer to the allocated ntfs device structure and on
 * error return NULL with errno set to the error code returned by ntfs_malloc().
 */
struct ntfs_device *ntfs_device_alloc(const char *name, const long state,
		struct ntfs_device_operations *dops, void *priv_data)
{
	struct ntfs_device *dev;

	if (!name) {
		errno = EINVAL;
		return NULL;
	}

	dev = ntfs_malloc(sizeof(struct ntfs_device));
	if (dev) {
		dev->d_name = _strdup(name);
		if (!(dev->d_name)) {
			int eo = errno;
			free(dev);
			errno = eo;
			return NULL;
		}
		dev->d_ops = dops;
		dev->d_state = state;
		dev->d_private = priv_data;
		dev->d_heads = -1;
		dev->d_sectors_per_track = -1;
	}
	return dev;
}

/**
 * ntfs_device_free - free an ntfs device structure
 * @dev:	ntfs device structure to free
 *
 * Free the ntfs device structure @dev.
 *
 * Return 0 on success or -1 on error with errno set to the error code. The
 * following error codes are defined:
 *	EINVAL		Invalid pointer @dev.
 *	EBUSY		Device is still open. Close it before freeing it!
 */
int ntfs_device_free(struct ntfs_device *dev)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	if (NDevOpen(dev)) {
		errno = EBUSY;
		return -1;
	}
	free(dev->d_name);
	free(dev);
	return 0;
}

/**
 * ntfs_pread - positioned read from disk
 * @dev:	device to read from
 * @pos:	position in device to read from
 * @count:	number of bytes to read
 * @b:		output data buffer
 *
 * This function will read @count bytes from device @dev at position @pos into
 * the data buffer @b.
 *
 * On success, return the number of successfully read bytes. If this number is
 * lower than @count this means that we have either reached end of file or
 * encountered an error during the read so that the read is partial. 0 means
 * end of file or nothing to read (@count is 0).
 *
 * On error and nothing has been read, return -1 with errno set appropriately
 * to the return code of either seek, read, or set to EINVAL in case of
 * invalid arguments.
 */
s64 ntfs_pread(struct ntfs_device *dev, const s64 pos, s64 count, void *b)
{
	s64 br, total;
	struct ntfs_device_operations *dops;

	ntfs_log_trace("pos %lld, count %lld\n",(long long)pos,(long long)count);
	
	if (!b || count < 0 || pos < 0) {
		errno = EINVAL;
		return -1;
	}
	if (!count)
		return 0;
	
	dops = dev->d_ops;

	for (total = 0; count; count -= br, total += br) {
		br = dops->pread(dev, (char*)b + total, count, pos + total);
		/* If everything ok, continue. */
		if (br > 0)
			continue;
		/* If EOF or error return number of bytes read. */
		if (!br || total)
			return total;
		/* Nothing read and error, return error status. */
		return br;
	}
	/* Finally, return the number of bytes read. */
	return total;
}

/**
 * ntfs_pwrite - positioned write to disk
 * @dev:	device to write to
 * @pos:	position in file descriptor to write to
 * @count:	number of bytes to write
 * @b:		data buffer to write to disk
 *
 * This function will write @count bytes from data buffer @b to the device @dev
 * at position @pos.
 *
 * On success, return the number of successfully written bytes. If this number
 * is lower than @count this means that the write has been interrupted in
 * flight or that an error was encountered during the write so that the write
 * is partial. 0 means nothing was written (also return 0 when @count is 0).
 *
 * On error and nothing has been written, return -1 with errno set
 * appropriately to the return code of either seek, write, or set
 * to EINVAL in case of invalid arguments.
 */
s64 ntfs_pwrite(struct ntfs_device *dev, const s64 pos, s64 count,
		const void *b)
{
	s64 written, total, ret = -1;
	struct ntfs_device_operations *dops;

	ntfs_log_trace("pos %lld, count %lld\n",(long long)pos,(long long)count);

	if (!b || count < 0 || pos < 0) {
		errno = EINVAL;
		goto out;
	}
	if (!count)
		return 0;
	if (NDevReadOnly(dev)) {
		errno = EROFS;
		goto out;
	}
	
	dops = dev->d_ops;

	NDevSetDirty(dev);
	for (total = 0; count; count -= written, total += written) {
		written = dops->pwrite(dev, (const char*)b + total, count,
				       pos + total);
		/* If everything ok, continue. */
		if (written > 0)
			continue;
		/*
		 * If nothing written or error return number of bytes written.
		 */
		if (!written || total)
			break;
		/* Nothing written and error, return error status. */
		total = written;
		break;
	}
	if (NDevSync(dev) && total && dops->sync(dev)) {
		total--; /* on sync error, return partially written */
	}
	ret = total;
out:	
	return ret;
}


/**
 * ntfs_mst_pwrite - multi sector transfer (mst) positioned write
 * @dev:	device to write to
 * @pos:	position in file descriptor to write to
 * @count:	number of blocks to write
 * @bksize:	size of each block that needs mst protecting
 * @b:		data buffer to write to disk
 *
 * Multi sector transfer (mst) positioned write. This function will write
 * @count blocks of size @bksize bytes each from data buffer @b to the device
 * @dev at position @pos.
 *
 * On success, return the number of successfully written blocks. If this number
 * is lower than @count this means that the write has been interrupted or that
 * an error was encountered during the write so that the write is partial. 0
 * means nothing was written (also return 0 when @count or @bksize are 0).
 *
 * On error and nothing has been written, return -1 with errno set
 * appropriately to the return code of either seek, write, or set
 * to EINVAL in case of invalid arguments.
 *
 * NOTE: We mst protect the data, write it, then mst deprotect it using a quick
 * deprotect algorithm (no checking). This saves us from making a copy before
 * the write and at the same time causes the usn to be incremented in the
 * buffer. This conceptually fits in better with the idea that cached data is
 * always deprotected and protection is performed when the data is actually
 * going to hit the disk and the cache is immediately deprotected again
 * simulating an mst read on the written data. This way cache coherency is
 * achieved.
 */
s64 ntfs_mst_pwrite(struct ntfs_device *dev, const s64 pos, s64 count,
		const u32 bksize, void *b)
{
	s64 written, i;

	if (count < 0 || bksize % NTFS_BLOCK_SIZE) {
		errno = EINVAL;
		return -1;
	}
	if (!count)
		return 0;
	/* Prepare data for writing. */
	for (i = 0; i < count; ++i) {
		int err;

		err = ntfs_mst_pre_write_fixup((NTFS_RECORD*)
				((u8*)b + i * bksize), bksize);
		if (err < 0) {
			/* Abort write at this position. */
			if (!i)
				return err;
			count = i;
			break;
		}
	}
	/* Write the prepared data. */
	written = ntfs_pwrite(dev, pos, count * bksize, b);
	/* Quickly deprotect the data again. */
	for (i = 0; i < count; ++i)
		ntfs_mst_post_write_fixup((NTFS_RECORD*)((u8*)b + i * bksize));
	if (written <= 0)
		return written;
	/* Finally, return the number of complete blocks written. */
	return written / bksize;
}

/**
 * ntfs_cluster_write - write ntfs clusters
 * @vol:	volume to write to
 * @lcn:	starting logical cluster number
 * @count:	number of clusters to write
 * @b:		data buffer to write to disk
 *
 * Write @count ntfs clusters starting at logical cluster number @lcn from
 * buffer @b to volume @vol. Return the number of clusters written or -1 on
 * error, with errno set to the error code.
 */
s64 ntfs_cluster_write(const ntfs_volume *vol, const s64 lcn,
		const s64 count, const void *b)
{
	s64 bw;

	if (!vol || lcn < 0 || count < 0) {
		errno = EINVAL;
		return -1;
	}
	if (vol->nr_clusters < lcn + count) {
		errno = ESPIPE;
		ntfs_log_perror("Trying to write outside of volume "
				"(%lld < %lld)", (long long)vol->nr_clusters,
			        (long long)lcn + count);
		return -1;
	}
	if (!NVolReadOnly(vol))
		bw = ntfs_pwrite(vol->dev, lcn << vol->cluster_size_bits,
				count << vol->cluster_size_bits, b);
	else
		bw = count << vol->cluster_size_bits;
	if (bw < 0) {
		ntfs_log_perror("Error writing cluster(s)");
		return bw;
	}
	return bw >> vol->cluster_size_bits;
}
