/**
 * volume.c - NTFS volume handling code. Originated from the Linux-NTFS project.
 *
 * Copyright (c) 2000-2006 Anton Altaparmakov
 * Copyright (c) 2002-2009 Szabolcs Szakacsits
 * Copyright (c) 2004-2005 Richard Russon
 * Copyright (c) 2010      Jean-Pierre Andre
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

#include <fcntl.h>
#include <sys/stat.h>
#include <locale.h>

#include "param.h"
#include "compat.h"
#include "volume.h"
#include "attrib.h"
#include "mft.h"
#include "bootsect.h"
#include "device.h"
#include "debug.h"
#include "inode.h"
#include "runlist.h"
#include "logfile.h"
#include "dir.h"
#include "logging.h"
#include "cache.h"
#include "realpath.h"
#include "misc.h"
#include "security.h"
#include "support.h"

/**
 * ntfs_volume_alloc - Create an NTFS volume object and initialise it
 *
 * Description...
 *
 * Returns:
 */
ntfs_volume *ntfs_volume_alloc(void)
{
	return ntfs_calloc(sizeof(ntfs_volume));
}

static void ntfs_attr_free(ntfs_attr **na)
{
	if (na && *na) {
		ntfs_attr_close(*na);
		*na = NULL;
	}
}

static int ntfs_inode_free(ntfs_inode **ni)
{
	int ret = -1;

	if (ni && *ni) {
		ret = ntfs_inode_close(*ni);
		*ni = NULL;
	} 
	
	return ret;
}

static void ntfs_error_set(int *err)
{
	if (!*err)
		*err = errno;
}

/**
 * __ntfs_volume_release - Destroy an NTFS volume object
 * @v:
 *
 * Description...
 *
 * Returns:
 */
static int __ntfs_volume_release(ntfs_volume *v)
{
	int err = 0;

	if (ntfs_close_secure(v))
		ntfs_error_set(&err);

	if (ntfs_inode_free(&v->vol_ni))
		ntfs_error_set(&err);
	/* 
	 * FIXME: Inodes must be synced before closing
	 * attributes, otherwise unmount could fail.
	 */
	if (v->lcnbmp_ni && NInoDirty(v->lcnbmp_ni))
		ntfs_inode_sync(v->lcnbmp_ni);
	ntfs_attr_free(&v->lcnbmp_na);
	if (ntfs_inode_free(&v->lcnbmp_ni))
		ntfs_error_set(&err);
	
	if (v->mft_ni && NInoDirty(v->mft_ni))
		ntfs_inode_sync(v->mft_ni);
	ntfs_attr_free(&v->mftbmp_na);
	ntfs_attr_free(&v->mft_na);
	if (ntfs_inode_free(&v->mft_ni))
		ntfs_error_set(&err);
	
	if (v->mftmirr_ni && NInoDirty(v->mftmirr_ni))
		ntfs_inode_sync(v->mftmirr_ni);
	ntfs_attr_free(&v->mftmirr_na);
	if (ntfs_inode_free(&v->mftmirr_ni))
		ntfs_error_set(&err);
	
	if (v->dev) {
		struct ntfs_device *dev = v->dev;

		if (dev->d_ops->sync(dev))
			ntfs_error_set(&err);
		if (dev->d_ops->close(dev))
			ntfs_error_set(&err);
	}

	ntfs_free_lru_caches(v);
	free(v->vol_name);
	free(v->upcase);
	if (v->locase) free(v->locase);
	free(v->attrdef);
	free(v);

	errno = err;
	return errno ? -1 : 0;
}

/**
 * ntfs_umount - close ntfs volume
 * @vol: address of ntfs_volume structure of volume to close
 * @force: if true force close the volume even if it is busy
 *
 * Deallocate all structures (including @vol itself) associated with the ntfs
 * volume @vol.
 *
 * Return 0 on success. On error return -1 with errno set appropriately
 * (most likely to one of EAGAIN, EBUSY or EINVAL). The EAGAIN error means that
 * an operation is in progress and if you try the close later the operation
 * might be completed and the close succeed.
 *
 * If @force is true (i.e. not zero) this function will close the volume even
 * if this means that data might be lost.
 *
 * @vol must have previously been returned by a call to ntfs_mount().
 *
 * @vol itself is deallocated and should no longer be dereferenced after this
 * function returns success. If it returns an error then nothing has been done
 * so it is safe to continue using @vol.
 */
int ntfs_umount(ntfs_volume *vol, const BOOL force)
{
	struct ntfs_device *dev;
	int ret;

	if (!vol) {
		errno = EINVAL;
		return -1;
	}
	dev = vol->dev;
	ret = __ntfs_volume_release(vol);
	ntfs_device_free(dev);
	return ret;
}