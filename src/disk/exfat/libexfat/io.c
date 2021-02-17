/*
	io.c (02.09.09)
	exFAT file system implementation library.

	Free exFAT implementation.
	Copyright (C) 2010-2018  Andrew Nayenko

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "exfat.h"
#include <inttypes.h>
#include "systypes.h"
#include <sys/stat.h>
#include "fcntl.h"
#include "unistd.h"
#include <string.h>
#include <errno.h>
#if defined(__APPLE__)
#include <sys/disk.h>
#elif defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#elif __linux__
#include <sys/mount.h>
#endif
#ifdef USE_UBLIO
#include <sys/uio.h>
#include <ublio.h>
#endif

#include "device.h"
#include "device_io.h"

struct exfat_dev
{
	void* io_fd;
	enum exfat_mode mode;
	off_t size; /* in bytes */
#ifdef USE_UBLIO
	off_t pos;
	ublio_filehandle_t ufh;
#endif
};

//static bool is_open(int fd)
//{
//	return fcntl(fd, F_GETFD) != -1;
//}

//static int open_ro(const char* spec)
//{
//	return open(spec, O_RDONLY);
//}
//
//static int open_rw(const char* spec)
//{
//	int fd = open(spec, O_RDWR);
//#ifdef __linux__
//	int ro = 0;
//
//	/*
//	   This ioctl is needed because after "blockdev --setro" kernel still
//	   allows to open the device in read-write mode but fails writes.
//	*/
//	if (fd != -1 && ioctl(fd, BLKROGET, &ro) == 0 && ro)
//	{
//		close(fd);
//		errno = EROFS;
//		return -1;
//	}
//#endif
//	return fd;
//}

struct exfat_dev* exfat_open(void* input, enum exfat_mode mode)
{
	struct exfat_dev* dev;
	struct stat stbuf;
#ifdef USE_UBLIO
	struct ublio_param up;
#endif

	/* The system allocates file descriptors sequentially. If we have been
	   started with stdin (0), stdout (1) or stderr (2) closed, the system
	   will give us descriptor 0, 1 or 2 later when we open block device,
	   FUSE communication pipe, etc. As a result, functions using stdin,
	   stdout or stderr will actually work with a different thing and can
	   corrupt it. Protect descriptors 0, 1 and 2 from such misuse. */
	// NOTE: Commented out because it's not used
	//while (!is_open(STDIN_FILENO)
	//	|| !is_open(STDOUT_FILENO)
	//	|| !is_open(STDERR_FILENO))
	//{
	//	/* we don't need those descriptors, let them leak */
	//	if (open("/dev/null", O_RDWR) == -1)
	//	{
	//		exfat_error("failed to open /dev/null");
	//		return NULL;
	//	}
	//}

	dev = malloc(sizeof(struct exfat_dev));
	if (dev == NULL)
	{
		exfat_error("failed to allocate memory for device structure");
		return NULL;
	}

	switch (mode)
	{
	case EXFAT_MODE_RO:
		dev->io_fd = default_io_ops.open_ro(input);
		if (dev->io_fd == NULL)
		{
			free(dev);
			exfat_error("failed to open '%p' in read-only mode: %s", input,
					strerror(errno));
			return NULL;
		}
		dev->mode = EXFAT_MODE_RO;
		break;
	case EXFAT_MODE_RW:
		dev->io_fd = default_io_ops.open_rw(input);
		if (dev->io_fd == NULL)
		{
			free(dev);
			exfat_error("failed to open '%p' in read-write mode: %s", input,
					strerror(errno));
			return NULL;
		}
		dev->mode = EXFAT_MODE_RW;
		break;
	case EXFAT_MODE_ANY:
		dev->io_fd = default_io_ops.open_rw(input);
		if (dev->io_fd != NULL)
		{
			dev->mode = EXFAT_MODE_RW;
			break;
		}
		dev->io_fd = default_io_ops.open_ro(input);
		if (dev->io_fd != NULL)
		{
			dev->mode = EXFAT_MODE_RO;
			exfat_warn("'%p' is write-protected, mounting read-only", input);
			break;
		}
		free(dev);
		exfat_error("failed to open '%p': %s", input, strerror(errno));
		return NULL;
	}

#if defined(__APPLE__)
	if (!S_ISREG(stbuf.st_mode))
	{
		uint32_t block_size = 0;
		uint64_t blocks = 0;

		if (ioctl(dev->fd, DKIOCGETBLOCKSIZE, &block_size) != 0)
		{
			close(dev->fd);
			free(dev);
			exfat_error("failed to get block size");
			return NULL;
		}
		if (ioctl(dev->fd, DKIOCGETBLOCKCOUNT, &blocks) != 0)
		{
			close(dev->fd);
			free(dev);
			exfat_error("failed to get blocks count");
			return NULL;
		}
		dev->size = blocks * block_size;
	}
	else
#elif defined(__OpenBSD__)
	if (!S_ISREG(stbuf.st_mode))
	{
		struct disklabel lab;
		struct partition* pp;
		char* partition;

		if (ioctl(dev->fd, DIOCGDINFO, &lab) == -1)
		{
			close(dev->fd);
			free(dev);
			exfat_error("failed to get disklabel");
			return NULL;
		}

		/* Don't need to check that partition letter is valid as we won't get
		   this far otherwise. */
		partition = strchr(spec, '\0') - 1;
		pp = &(lab.d_partitions[*partition - 'a']);
		dev->size = DL_GETPSIZE(pp) * lab.d_secsize;

		if (pp->p_fstype != FS_NTFS)
			exfat_warn("partition type is not 0x07 (NTFS/exFAT); "
					"you can fix this with fdisk(8)");
	}
	else
#endif
	{
		/* works for Linux, FreeBSD, Solaris */
		dev->size = exfat_seek(dev, 0, SEEK_END);
		if (dev->size <= 0)
		{
			default_io_ops.close(dev->io_fd);
			free(dev);
			exfat_error("failed to get size of '%p'", input);
			return NULL;
		}
		if (exfat_seek(dev, 0, SEEK_SET) == -1)
		{
			default_io_ops.close(dev->io_fd);
			free(dev);
			exfat_error("failed to seek to the beginning of '%p'", input);
			return NULL;
		}
	}

#ifdef USE_UBLIO
	memset(&up, 0, sizeof(struct ublio_param));
	up.up_blocksize = 256 * 1024;
	up.up_items = 64;
	up.up_grace = 32;
	up.up_priv = &dev->fd;

	dev->pos = 0;
	dev->ufh = ublio_open(&up);
	if (dev->ufh == NULL)
	{
		close(dev->fd);
		free(dev);
		exfat_error("failed to initialize ublio");
		return NULL;
	}
#endif

	return dev;
}

int exfat_close(struct exfat_dev* dev)
{
	int rc = 0;

#ifdef USE_UBLIO
	if (ublio_close(dev->ufh) != 0)
	{
		exfat_error("failed to close ublio");
		rc = -EIO;
	}
#endif
	if (default_io_ops.close(dev->io_fd) != 0)
	{
		exfat_error("failed to close device: %s", strerror(errno));
		rc = -EIO;
	}
	free(dev);
	return rc;
}

void* exfat_get_io_fd(struct exfat_dev* dev) {
	return dev->io_fd;
}

int exfat_fsync(struct exfat_dev* dev)
{
	int rc = 0;

#ifdef USE_UBLIO
	if (ublio_fsync(dev->ufh) != 0)
	{
		exfat_error("ublio fsync failed");
		rc = -EIO;
	}
#endif
	// fsync does not exist on windows
	//if (fsync(dev->fd) != 0)
	//{
	//	exfat_error("fsync failed: %s", strerror(errno));
	//	rc = -EIO;
	//}
	return rc;
}

enum exfat_mode exfat_get_mode(const struct exfat_dev* dev)
{
	return dev->mode;
}

off_t exfat_get_size(const struct exfat_dev* dev)
{
	return dev->size;
}

off_t exfat_seek(struct exfat_dev* dev, off_t offset, int whence)
{
#ifdef USE_UBLIO
	/* XXX SEEK_CUR will be handled incorrectly */
	return dev->pos = lseek(dev->fd, offset, whence);
#else
	return default_io_ops.lseek(dev->io_fd, offset, whence);
#endif
}

ssize_t exfat_read(struct exfat_dev* dev, void* buffer, size_t size)
{
#ifdef USE_UBLIO
	ssize_t result = ublio_pread(dev->ufh, buffer, size, dev->pos);
	if (result >= 0)
		dev->pos += size;
	return result;
#else
	return default_io_ops.read(dev->io_fd, buffer, size);
#endif
}

ssize_t exfat_write(struct exfat_dev* dev, const void* buffer, size_t size)
{
#ifdef USE_UBLIO
	ssize_t result = ublio_pwrite(dev->ufh, buffer, size, dev->pos);
	if (result >= 0)
		dev->pos += size;
	return result;
#else
	return default_io_ops.write(dev->io_fd, buffer, size);
#endif
}

ssize_t exfat_pread(struct exfat_dev* dev, void* buffer, size_t size,
		off_t offset)
{
#ifdef USE_UBLIO
	return ublio_pread(dev->ufh, buffer, size, offset);
#else
	return default_io_ops.pread(dev->io_fd, buffer, size, offset);
#endif
}

ssize_t exfat_pwrite(struct exfat_dev* dev, const void* buffer, size_t size,
		off_t offset)
{
#ifdef USE_UBLIO
	return ublio_pwrite(dev->ufh, buffer, size, offset);
#else
	return default_io_ops.pwrite(dev->io_fd, buffer, size, offset);
#endif
}

ssize_t exfat_generic_pread(const struct exfat* ef, struct exfat_node* node,
		void* buffer, size_t size, off_t offset)
{
	uint64_t newsize = offset;
	cluster_t cluster;
	char* bufp = buffer;
	off_t lsize, loffset, remainder;

	if (offset < 0)
		return -EINVAL;
	if (newsize >= node->size)
		return 0;
	if (size == 0)
		return 0;

	cluster = exfat_advance_cluster(ef, node, newsize / CLUSTER_SIZE(*ef->sb));
	if (CLUSTER_INVALID(*ef->sb, cluster))
	{
		exfat_error("invalid cluster 0x%x while reading", cluster);
		return -EIO;
	}

	loffset = newsize % CLUSTER_SIZE(*ef->sb);
	remainder = MIN(size, node->size - newsize);
	while (remainder > 0)
	{
		if (CLUSTER_INVALID(*ef->sb, cluster))
		{
			exfat_error("invalid cluster 0x%x while reading", cluster);
			return -EIO;
		}
		lsize = MIN(CLUSTER_SIZE(*ef->sb) - loffset, remainder);
		if (exfat_pread(ef->dev, bufp, lsize,
					exfat_c2o(ef, cluster) + loffset) < 0)
		{
			exfat_error("failed to read cluster %#x", cluster);
			return -EIO;
		}
		bufp += lsize;
		loffset = 0;
		remainder -= lsize;
		cluster = exfat_next_cluster(ef, node, cluster);
	}
	if (!(node->attrib & EXFAT_ATTRIB_DIR) && !ef->ro && !ef->noatime)
		exfat_update_atime(node);
	return MIN(size, node->size - newsize) - remainder;
}

ssize_t exfat_generic_pwrite(struct exfat* ef, struct exfat_node* node,
		const void* buffer, size_t size, off_t offset)
{
	uint64_t newsize = offset;
	int rc;
	cluster_t cluster;
	const char* bufp = buffer;
	off_t lsize, loffset, remainder;

	if (offset < 0)
		return -EINVAL;
	if (newsize > node->size)
	{
		rc = exfat_truncate(ef, node, newsize, true);
		if (rc != 0)
			return rc;
	}
	if (newsize + size > node->size)
	{
		rc = exfat_truncate(ef, node, newsize + size, false);
		if (rc != 0)
			return rc;
	}
	if (size == 0)
		return 0;

	cluster = exfat_advance_cluster(ef, node, newsize / CLUSTER_SIZE(*ef->sb));
	if (CLUSTER_INVALID(*ef->sb, cluster))
	{
		exfat_error("invalid cluster 0x%x while writing", cluster);
		return -EIO;
	}

	loffset = newsize % CLUSTER_SIZE(*ef->sb);
	remainder = size;
	while (remainder > 0)
	{
		if (CLUSTER_INVALID(*ef->sb, cluster))
		{
			exfat_error("invalid cluster 0x%x while writing", cluster);
			return -EIO;
		}
		lsize = MIN(CLUSTER_SIZE(*ef->sb) - loffset, remainder);
		if (exfat_pwrite(ef->dev, bufp, lsize,
				exfat_c2o(ef, cluster) + loffset) < 0)
		{
			exfat_error("failed to write cluster %#x", cluster);
			return -EIO;
		}
		bufp += lsize;
		loffset = 0;
		remainder -= lsize;
		cluster = exfat_next_cluster(ef, node, cluster);
	}
	if (!(node->attrib & EXFAT_ATTRIB_DIR))
		/* directory's mtime should be updated by the caller only when it
		   creates or removes something in this directory */
		exfat_update_mtime(node);
	return size - remainder;
}

static uint32_t c2i(const struct exfat* ef, cluster_t c) {
	return exfat_c2o(ef, c) / CLUSTER_SIZE(*ef->sb);
}

#define ALIGN(N, Value, Alignment) (N)(((uint64_t)Value + Alignment - 1) & ~(Alignment - 1))

int exfat_get_runlist(const struct exfat* ef, struct exfat_node* node, EGL3Run o_runs[16]) {
	cluster_t cluster;
	uint64_t cluster_count = ALIGN(uint64_t, node->size, CLUSTER_SIZE(*ef->sb)) / CLUSTER_SIZE(*ef->sb);

	if (node->size == 0) {
		o_runs[0].count = 0;
		return 0;
	}

	if (node->is_contiguous) {
		o_runs[0].idx = c2i(ef, node->start_cluster);
		o_runs[0].count = c2i(ef, node->start_cluster + cluster_count) - o_runs[0].idx;
		return 0;
	}

	cluster = node->start_cluster;
	int run_idx = 0;
	for(uint64_t i = 0; i < cluster_count; ++i, cluster = exfat_next_cluster(ef, node, cluster)) {
		if (CLUSTER_INVALID(*ef->sb, cluster))
		{
			exfat_error("invalid cluster 0x%x while reading", cluster);
			return -EIO;
		}

		if (i == 0) {
			o_runs[run_idx].idx = c2i(ef, node->start_cluster);
			o_runs[run_idx].count = 1;
		}
		else {
			if (o_runs[run_idx].idx + o_runs[run_idx].count + 1 == cluster) {
				++o_runs[run_idx].count;
			}
			else {
				++run_idx;
				o_runs[run_idx].idx = c2i(ef, cluster);
				o_runs[run_idx].count = 1;
			}
		}
	}
	return 0;
}