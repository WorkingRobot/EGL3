/**
 * mkntfs - Part of the Linux-NTFS project.
 *
 * Copyright (c) 2000-2011 Anton Altaparmakov
 * Copyright (c) 2001-2005 Richard Russon
 * Copyright (c) 2002-2006 Szabolcs Szakacsits
 * Copyright (c) 2005      Erik Sornes
 * Copyright (c) 2007      Yura Pakhuchiy
 * Copyright (c) 2010-2018 Jean-Pierre Andre
 *
 * This utility will create an NTFS 1.2 or 3.1 volume on a user
 * specified (block) device.
 *
 * Some things (option handling and determination of mount status) have been
 * adapted from e2fsprogs-1.19 and lib/ext2fs/ismounted.c and misc/mke2fs.c in
 * particular.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS source
 * in the file COPYING); if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "types.h"
#include "egl3interface.h"
#include "runlist.h"
#include "device.h"
#include "dir.h"
#include "device_io.h"
#include "logfile.h"
#include "boot.h"
#include "attrdef.h"
#include "misc.h"

typedef enum { WRITE_STANDARD, WRITE_BITMAP, WRITE_LOGFILE } WRITE_TYPE;

#ifdef NO_NTFS_DEVICE_DEFAULT_IO_OPS
#error "No default device io operations!  Cannot build mkntfs.  \
You need to run ./configure without the --disable-default-device-io-ops \
switch if you want to be able to build the NTFS utilities."
#endif

/* Page size on ia32. Can change to 8192 on Alpha. */
#define NTFS_PAGE_SIZE	4096

#define OPT_NO_ACTION 0
#define OPT_SECTOR_SIZE 512
#define OPT_CLUSTER_SIZE 4096

struct BITMAP_ALLOCATION {
	struct BITMAP_ALLOCATION* next;
	LCN	lcn;		/* first allocated cluster */
	s64	length;		/* count of consecutive clusters */
};

/* Upcase $Info, used since Windows 8 */
struct UPCASEINFO {
	le32	len;
	le32	filler;
	le64	crc;
	le32	osmajor;
	le32	osminor;
	le32	build;
	le16	packmajor;
	le16	packminor;
};

/**
 * struct mkntfs_options
 */
struct mkntfs_options {
	long long num_sectors;		/* size of device in sectors (required) */
	char* label;			/* -L, volume label */
	long mbr_hidden_sectors;
	EGL3File* egl3_files;
	u32 egl3_file_count;

	void* o_data; // AppendingFile*

	// these were global variables
	u8* g_buf;
	int	g_mft_bitmap_byte_size;
	u8* g_mft_bitmap;
	int	g_lcn_bitmap_byte_size;
	int	g_dynamic_buf_size;
	u8* g_dynamic_buf;
	struct UPCASEINFO* g_upcaseinfo;
	runlist* g_rl_mft;
	runlist* g_rl_mft_bmp;
	runlist* g_rl_mftmirr;
	runlist* g_rl_logfile;
	runlist* g_rl_boot;
	runlist* g_rl_bad;
	INDEX_ALLOCATION* g_index_block;
	ntfs_volume* g_vol;
	u8* g_logfile_buf;
	int		   g_mft_size;
	long long	   g_mft_lcn;		/* lcn of $MFT, $DATA attribute */
	long long	   g_mftmirr_lcn;		/* lcn of $MFTMirr, $DATA */
	long long	   g_logfile_lcn;		/* lcn of $LogFile, $DATA */
	int		   g_logfile_size;		/* in bytes, determined from volume_size */
	long long	   g_mft_zone_end;		/* Determined from volume_size and mft_zone_multiplier, in clusters */
	long long	   g_num_bad_blocks;		/* Number of bad clusters */
	long long* g_bad_blocks;	/* Array of bad clusters */

	struct BITMAP_ALLOCATION* g_allocation;	/* Head of cluster allocations */
};

typedef struct mkntfs_options OPTS;

/*
 *  crc64, adapted from http://rpm5.org/docs/api/digest_8c-source.html
 * ECMA-182 polynomial, see
 *     http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-182.pdf
 */
 /* make sure the needed types are defined */
#undef byte
#undef uint32_t
#undef uint64_t
#define byte u8
#define uint32_t u32
#define uint64_t u64
static uint64_t crc64(uint64_t crc, const byte* data, size_t size)
/*@*/
{
	static uint64_t polynomial = 0x9a6c9329ac4bc9b5ULL;
	static uint64_t xorout = 0xffffffffffffffffULL;
	static uint64_t table[256];

	crc ^= xorout;

	if (data == NULL) {
		/* generate the table of CRC remainders for all possible bytes */
		uint64_t c;
		uint32_t i, j;
		for (i = 0; i < 256; i++) {
			c = i;
			for (j = 0; j < 8; j++) {
				if (c & 1)
					c = polynomial ^ (c >> 1);
				else
					c = (c >> 1);
			}
			table[i] = c;
		}
	}
	else
		while (size) {
			crc = table[(crc ^ *data) & 0xff] ^ (crc >> 8);
			size--;
			data++;
		}

	crc ^= xorout;

	return crc;
}

/*
 *		Mark a run of clusters as allocated
 *
 *	Returns FALSE if unsuccessful
 */

static BOOL bitmap_allocate(OPTS* opts, LCN lcn, s64 length)
{
	BOOL done;
	struct BITMAP_ALLOCATION* p;
	struct BITMAP_ALLOCATION* q;
	struct BITMAP_ALLOCATION* newall;

	done = TRUE;
	if (length) {
		p = opts->g_allocation;
		q = (struct BITMAP_ALLOCATION*)NULL;
		/* locate the first run which starts beyond the requested lcn */
		while (p && (p->lcn <= lcn)) {
			q = p;
			p = p->next;
		}
		/* make sure the requested lcns were not allocated */
		if ((q && ((q->lcn + q->length) > lcn))
			|| (p && ((lcn + length) > p->lcn))) {
			ntfs_log_error("Bitmap allocation error\n");
			done = FALSE;
		}
		if (q && ((q->lcn + q->length) == lcn)) {
			/* extend current run, no overlapping possible */
			q->length += length;
		}
		else {
			newall = (struct BITMAP_ALLOCATION*)
				ntfs_malloc(sizeof(struct BITMAP_ALLOCATION));
			if (newall) {
				newall->lcn = lcn;
				newall->length = length;
				newall->next = p;
				if (q) q->next = newall;
				else opts->g_allocation = newall;
			}
			else {
				done = FALSE;
				ntfs_log_perror("Not enough memory");
			}
		}
	}
	return (done);
}

/*
 *		Mark a run of cluster as not allocated
 *
 *	Returns FALSE if unsuccessful
 *		(freeing free clusters is not considered as an error)
 */

static BOOL bitmap_deallocate(OPTS* opts, LCN lcn, s64 length)
{
	BOOL done;
	struct BITMAP_ALLOCATION* p;
	struct BITMAP_ALLOCATION* q;
	LCN first, last;
	s64 begin_length, end_length;

	done = TRUE;
	if (length) {
		p = opts->g_allocation;
		q = (struct BITMAP_ALLOCATION*)NULL;
		/* locate a run which has a common portion */
		while (p) {
			first = (p->lcn > lcn ? p->lcn : lcn);
			last = ((p->lcn + p->length) < (lcn + length)
				? p->lcn + p->length : lcn + length);
			if (first < last) {
				/* get the parts which must be kept */
				begin_length = first - p->lcn;
				end_length = p->lcn + p->length - last;
				/* delete the entry */
				if (q)
					q->next = p->next;
				else
					opts->g_allocation = p->next;
				free(p);
				/* reallocate the beginning and the end */
				if (begin_length
					&& !bitmap_allocate(opts, first - begin_length,
						begin_length))
					done = FALSE;
				if (end_length
					&& !bitmap_allocate(opts, last, end_length))
					done = FALSE;
				/* restart a full search */
				p = opts->g_allocation;
				q = (struct BITMAP_ALLOCATION*)NULL;
			}
			else {
				q = p;
				p = p->next;
			}
		}
	}
	return (done);
}

/*
 *		Get the allocation status of a single cluster
 *	and mark as allocated
 *
 *	Returns 1 if the cluster was previously allocated
 */

static int bitmap_get_and_set(OPTS* opts, LCN lcn, unsigned long length)
{
	struct BITMAP_ALLOCATION* p;
	struct BITMAP_ALLOCATION* q;
	int bit;

	if (length == 1) {
		p = opts->g_allocation;
		q = (struct BITMAP_ALLOCATION*)NULL;
		/* locate the first run which starts beyond the requested lcn */
		while (p && (p->lcn <= lcn)) {
			q = p;
			p = p->next;
		}
		if (q && (q->lcn <= lcn) && ((q->lcn + q->length) > lcn))
			bit = 1; /* was allocated */
		else {
			bitmap_allocate(opts, lcn, length);
			bit = 0;
		}
	}
	else {
		ntfs_log_error("Can only allocate a single cluster at a time\n");
		bit = 0;
	}
	return (bit);
}

/*
 *		Build a section of the bitmap according to allocation
 */

static void bitmap_build(OPTS* opts, u8* buf, LCN lcn, s64 length)
{
	struct BITMAP_ALLOCATION* p;
	LCN first, last;
	int j; /* byte number */
	int bn; /* bit number */

	for (j = 0; (8 * j) < length; j++)
		buf[j] = 0;
	for (p = opts->g_allocation; p; p = p->next) {
		first = (p->lcn > lcn ? p->lcn : lcn);
		last = ((p->lcn + p->length) < (lcn + length)
			? p->lcn + p->length : lcn + length);
		if (first < last) {
			bn = first - lcn;
			/* initial partial byte, if any */
			while ((bn < (last - lcn)) && (bn & 7)) {
				buf[bn >> 3] |= 1 << (bn & 7);
				bn++;
			}
			/* full bytes */
			while (bn < (last - lcn - 7)) {
				buf[bn >> 3] = 255;
				bn += 8;
			}
			/* final partial byte, if any */
			while (bn < (last - lcn)) {
				buf[bn >> 3] |= 1 << (bn & 7);
				bn++;
			}
		}
	}
}

/**
 * mkntfs_time
 */
static ntfs_time mkntfs_time(void)
{
	struct timespec ts;

	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	ts.tv_sec = time(NULL);
	return timespec2ntfs(ts);
}

/**
 * mkntfs_write
 */
static long long mkntfs_write(OPTS* opts, struct ntfs_device* dev,
	const void* b, long long count)
{
	long long bytes_written, total;
	int retry;

	if (OPT_NO_ACTION)
		return count;
	total = 0LL;
	retry = 0;
	do {
		bytes_written = dev->d_ops->write(dev, b, count);
		if (bytes_written == -1LL) {
			retry = errno;
			ntfs_log_perror("Error writing to %s", dev->d_name);
			errno = retry;
			return bytes_written;
		}
		else if (!bytes_written) {
			retry++;
		}
		else {
			count -= bytes_written;
			total += bytes_written;
		}
	} while (count && retry < 3);
	if (count)
		ntfs_log_error("Failed to complete writing to %s after three retries."
			"\n", dev->d_name);
	return total;
}

/**
 *		Build and write a part of the global bitmap
 *	without overflowing from the allocated buffer
 *
 * mkntfs_bitmap_write
 */
static s64 mkntfs_bitmap_write(OPTS* opts, struct ntfs_device* dev,
	s64 offset, s64 length)
{
	s64 partial_length;
	s64 written;

	partial_length = length;
	if (partial_length > opts->g_dynamic_buf_size)
		partial_length = opts->g_dynamic_buf_size;
	/* create a partial bitmap section, and write it */
	bitmap_build(opts, opts->g_dynamic_buf, offset << 3, partial_length << 3);
	written = dev->d_ops->write(dev, opts->g_dynamic_buf, partial_length);
	return (written);
}

/**
 *		Build and write a part of the log file
 *	without overflowing from the allocated buffer
 *
 * mkntfs_logfile_write
 */
static s64 mkntfs_logfile_write(OPTS* opts, struct ntfs_device* dev,
	s64 offset, s64 length)
{
	s64 partial_length;
	s64 written;

	partial_length = length;
	if (partial_length > opts->g_dynamic_buf_size)
		partial_length = opts->g_dynamic_buf_size;
	/* create a partial bad cluster section, and write it */
	memset(opts->g_dynamic_buf, -1, partial_length);
	written = dev->d_ops->write(dev, opts->g_dynamic_buf, partial_length);
	return (written);
}

/**
 * ntfs_rlwrite - Write to disk the clusters contained in the runlist @rl
 * taking the data from @val.  Take @val_len bytes from @val and pad the
 * rest with zeroes.
 *
 * If the @rl specifies a completely sparse file, @val is allowed to be NULL.
 *
 * @inited_size if not NULL points to an output variable which will contain
 * the actual number of bytes written to disk. I.e. this will not include
 * sparse bytes for example.
 *
 * Return the number of bytes written (minus padding) or -1 on error. Errno
 * will be set to the error code.
 */
static s64 ntfs_rlwrite(OPTS* opts, struct ntfs_device* dev, const runlist* rl,
	const u8* val, const s64 val_len, s64* inited_size,
	WRITE_TYPE write_type)
{
	s64 bytes_written, total, length, delta;
	int retry, i;

	if (inited_size)
		*inited_size = 0LL;
	if (OPT_NO_ACTION)
		return val_len;
	total = 0LL;
	delta = 0LL;
	for (i = 0; rl[i].length; i++) {
		length = rl[i].length * opts->g_vol->cluster_size;
		/* Don't write sparse runs. */
		if (rl[i].lcn == -1) {
			total += length;
			if (!val)
				continue;
			/* TODO: Check that *val is really zero at pos and len. */
			continue;
		}
		/*
		 * Break up the write into the real data write and then a write
		 * of zeroes between the end of the real data and the end of
		 * the (last) run.
		 */
		if (total + length > val_len) {
			delta = length;
			length = val_len - total;
			delta -= length;
		}
		if (dev->d_ops->seek(dev, rl[i].lcn * opts->g_vol->cluster_size,
			SEEK_SET) == (off_t)-1)
			return -1LL;
		retry = 0;
		do {
			/* use specific functions if buffer is not prefilled */
			switch (write_type) {
			case WRITE_BITMAP:
				bytes_written = mkntfs_bitmap_write(opts, dev,
					total, length);
				break;
			case WRITE_LOGFILE:
				bytes_written = mkntfs_logfile_write(opts, dev,
					total, length);
				break;
			default:
				bytes_written = dev->d_ops->write(dev,
					val + total, length);
				break;
			}
			if (bytes_written == -1LL) {
				retry = errno;
				ntfs_log_perror("Error writing to %s",
					dev->d_name);
				errno = retry;
				return bytes_written;
			}
			if (bytes_written) {
				length -= bytes_written;
				total += bytes_written;
				if (inited_size)
					*inited_size += bytes_written;
			}
			else {
				retry++;
			}
		} while (length && retry < 3);
		if (length) {
			ntfs_log_error("Failed to complete writing to %s after three "
				"retries.\n", dev->d_name);
			return total;
		}
	}
	if (delta) {
		int eo;
		char* b = ntfs_calloc(delta);
		if (!b)
			return -1;
		bytes_written = mkntfs_write(opts, dev, b, delta);
		eo = errno;
		free(b);
		errno = eo;
		if (bytes_written == -1LL)
			return bytes_written;
	}
	return total;
}

/**
 * make_room_for_attribute - make room for an attribute inside an mft record
 * @m:		mft record
 * @pos:	position at which to make space
 * @size:	byte size to make available at this position
 *
 * @pos points to the attribute in front of which we want to make space.
 *
 * Return 0 on success or -errno on error. Possible error codes are:
 *
 *	-ENOSPC		There is not enough space available to complete
 *			operation. The caller has to make space before calling
 *			this.
 *	-EINVAL		Can only occur if mkntfs was compiled with -DDEBUG. Means
 *			the input parameters were faulty.
 */
static int make_room_for_attribute(MFT_RECORD* m, char* pos, const u32 size)
{
	u32 biu;

	if (!size)
		return 0;
	biu = le32_to_cpu(m->bytes_in_use);
	/* Do we have enough space? */
	if (biu + size > le32_to_cpu(m->bytes_allocated))
		return -ENOSPC;
	/* Move everything after pos to pos + size. */
	memmove(pos + size, pos, biu - (pos - (char*)m));
	/* Update mft record. */
	m->bytes_in_use = cpu_to_le32(biu + size);
	return 0;
}

/**
 * allocate_scattered_clusters
 * @clusters: Amount of clusters to allocate.
 *
 * Allocate @clusters and create a runlist of the allocated clusters.
 *
 * Return the allocated runlist. Caller has to free the runlist when finished
 * with it.
 *
 * On error return NULL and errno is set to the error code.
 *
 * TODO: We should be returning the size as well, but for mkntfs this is not
 * necessary.
 */
static runlist* allocate_scattered_clusters(OPTS* opts, s64 clusters)
{
	runlist* rl = NULL, * rlt;
	VCN vcn = 0LL;
	LCN lcn, end, prev_lcn = 0LL;
	int rlpos = 0;
	int rlsize = 0;
	s64 prev_run_len = 0LL;
	char bit;

	end = opts->g_vol->nr_clusters;
	/* Loop until all clusters are allocated. */
	while (clusters) {
		/* Loop in current zone until we run out of free clusters. */
		for (lcn = opts->g_mft_zone_end; lcn < end; lcn++) {
			bit = bitmap_get_and_set(opts, lcn, 1);
			if (bit)
				continue;
			/*
			 * Reallocate memory if necessary. Make sure we have
			 * enough for the terminator entry as well.
			 */
			if ((rlpos + 2) * (int)sizeof(runlist) >= rlsize) {
				rlsize += 4096; /* PAGE_SIZE */
				rlt = realloc(rl, rlsize);
				if (!rlt)
					goto err_end;
				rl = rlt;
			}
			/* Coalesce with previous run if adjacent LCNs. */
			if (prev_lcn == lcn - prev_run_len) {
				rl[rlpos - 1].length = ++prev_run_len;
				vcn++;
			}
			else {
				rl[rlpos].vcn = vcn++;
				rl[rlpos].lcn = lcn;
				prev_lcn = lcn;
				rl[rlpos].length = 1LL;
				prev_run_len = 1LL;
				rlpos++;
			}
			/* Done? */
			if (!--clusters) {
				/* Add terminator element and return. */
				rl[rlpos].vcn = vcn;
				rl[rlpos].lcn = 0LL;
				rl[rlpos].length = 0LL;
				return rl;
			}

		}
		/* Switch to next zone, decreasing mft zone by factor 2. */
		end = opts->g_mft_zone_end;
		opts->g_mft_zone_end >>= 1;
		/* Have we run out of space on the volume? */
		if (opts->g_mft_zone_end <= 0)
			goto err_end;
	}
	return rl;
err_end:
	if (rl) {
		/* Add terminator element. */
		rl[rlpos].vcn = vcn;
		rl[rlpos].lcn = -1LL;
		rl[rlpos].length = 0LL;
		/* Free the runlist. */
		free(rl);
	}
	return NULL;
}

/**
 * ntfs_attr_find - find (next) attribute in mft record
 * @type:	attribute type to find
 * @name:	attribute name to find (optional, i.e. NULL means don't care)
 * @name_len:	attribute name length (only needed if @name present)
 * @ic:		IGNORE_CASE or CASE_SENSITIVE (ignored if @name not present)
 * @val:	attribute value to find (optional, resident attributes only)
 * @val_len:	attribute value length
 * @ctx:	search context with mft record and attribute to search from
 *
 * You shouldn't need to call this function directly. Use lookup_attr() instead.
 *
 * ntfs_attr_find() takes a search context @ctx as parameter and searches the
 * mft record specified by @ctx->mrec, beginning at @ctx->attr, for an
 * attribute of @type, optionally @name and @val. If found, ntfs_attr_find()
 * returns 0 and @ctx->attr will point to the found attribute.
 *
 * If not found, ntfs_attr_find() returns -1, with errno set to ENOENT and
 * @ctx->attr will point to the attribute before which the attribute being
 * searched for would need to be inserted if such an action were to be desired.
 *
 * On actual error, ntfs_attr_find() returns -1 with errno set to the error
 * code but not to ENOENT.  In this case @ctx->attr is undefined and in
 * particular do not rely on it not changing.
 *
 * If @ctx->is_first is TRUE, the search begins with @ctx->attr itself. If it
 * is FALSE, the search begins after @ctx->attr.
 *
 * If @type is AT_UNUSED, return the first found attribute, i.e. one can
 * enumerate all attributes by setting @type to AT_UNUSED and then calling
 * ntfs_attr_find() repeatedly until it returns -1 with errno set to ENOENT to
 * indicate that there are no more entries. During the enumeration, each
 * successful call of ntfs_attr_find() will return the next attribute in the
 * mft record @ctx->mrec.
 *
 * If @type is AT_END, seek to the end and return -1 with errno set to ENOENT.
 * AT_END is not a valid attribute, its length is zero for example, thus it is
 * safer to return error instead of success in this case. This also allows us
 * to interoperate cleanly with ntfs_external_attr_find().
 *
 * If @name is AT_UNNAMED search for an unnamed attribute. If @name is present
 * but not AT_UNNAMED search for a named attribute matching @name. Otherwise,
 * match both named and unnamed attributes.
 *
 * If @ic is IGNORE_CASE, the @name comparison is not case sensitive and
 * @ctx->ntfs_ino must be set to the ntfs inode to which the mft record
 * @ctx->mrec belongs. This is so we can get at the ntfs volume and hence at
 * the upcase table. If @ic is CASE_SENSITIVE, the comparison is case
 * sensitive. When @name is present, @name_len is the @name length in Unicode
 * characters.
 *
 * If @name is not present (NULL), we assume that the unnamed attribute is
 * being searched for.
 *
 * Finally, the resident attribute value @val is looked for, if present.
 * If @val is not present (NULL), @val_len is ignored.
 *
 * ntfs_attr_find() only searches the specified mft record and it ignores the
 * presence of an attribute list attribute (unless it is the one being searched
 * for, obviously). If you need to take attribute lists into consideration, use
 * ntfs_attr_lookup() instead (see below). This also means that you cannot use
 * ntfs_attr_find() to search for extent records of non-resident attributes, as
 * extents with lowest_vcn != 0 are usually described by the attribute list
 * attribute only. - Note that it is possible that the first extent is only in
 * the attribute list while the last extent is in the base mft record, so don't
 * rely on being able to find the first extent in the base mft record.
 *
 * Warning: Never use @val when looking for attribute types which can be
 *	    non-resident as this most likely will result in a crash!
 */
static int mkntfs_attr_find(OPTS* opts, const ATTR_TYPES type, const ntfschar* name,
	const u32 name_len, const IGNORE_CASE_BOOL ic,
	const u8* val, const u32 val_len, ntfs_attr_search_ctx* ctx)
{
	ATTR_RECORD* a;
	ntfschar* upcase = opts->g_vol->upcase;
	u32 upcase_len = opts->g_vol->upcase_len;

	/*
	 * Iterate over attributes in mft record starting at @ctx->attr, or the
	 * attribute following that, if @ctx->is_first is TRUE.
	 */
	if (ctx->is_first) {
		a = ctx->attr;
		ctx->is_first = FALSE;
	}
	else {
		a = (ATTR_RECORD*)((char*)ctx->attr +
			le32_to_cpu(ctx->attr->length));
	}
	for (;; a = (ATTR_RECORD*)((char*)a + le32_to_cpu(a->length))) {
		if (p2n(a) < p2n(ctx->mrec) || (char*)a > (char*)ctx->mrec +
			le32_to_cpu(ctx->mrec->bytes_allocated))
			break;
		ctx->attr = a;
		if (((type != AT_UNUSED) && (le32_to_cpu(a->type) >
			le32_to_cpu(type))) ||
			(a->type == AT_END)) {
			errno = ENOENT;
			return -1;
		}
		if (!a->length)
			break;
		/* If this is an enumeration return this attribute. */
		if (type == AT_UNUSED)
			return 0;
		if (a->type != type)
			continue;
		/*
		 * If @name is AT_UNNAMED we want an unnamed attribute.
		 * If @name is present, compare the two names.
		 * Otherwise, match any attribute.
		 */
		if (name == AT_UNNAMED) {
			/* The search failed if the found attribute is named. */
			if (a->name_length) {
				errno = ENOENT;
				return -1;
			}
		}
		else if (name && !ntfs_names_are_equal(name, name_len,
			(ntfschar*)((char*)a + le16_to_cpu(a->name_offset)),
			a->name_length, ic, upcase, upcase_len)) {
			int rc;

			rc = ntfs_names_full_collate(name, name_len,
				(ntfschar*)((char*)a +
					le16_to_cpu(a->name_offset)),
				a->name_length, IGNORE_CASE,
				upcase, upcase_len);
			/*
			 * If @name collates before a->name, there is no
			 * matching attribute.
			 */
			if (rc == -1) {
				errno = ENOENT;
				return -1;
			}
			/* If the strings are not equal, continue search. */
			if (rc)
				continue;
			rc = ntfs_names_full_collate(name, name_len,
				(ntfschar*)((char*)a +
					le16_to_cpu(a->name_offset)),
				a->name_length, CASE_SENSITIVE,
				upcase, upcase_len);
			if (rc == -1) {
				errno = ENOENT;
				return -1;
			}
			if (rc)
				continue;
		}
		/*
		 * The names match or @name not present and attribute is
		 * unnamed. If no @val specified, we have found the attribute
		 * and are done.
		 */
		if (!val) {
			return 0;
			/* @val is present; compare values. */
		}
		else {
			int rc;

			rc = memcmp(val, (char*)a + le16_to_cpu(a->value_offset),
				min(val_len,
					le32_to_cpu(a->value_length)));
			/*
			 * If @val collates before the current attribute's
			 * value, there is no matching attribute.
			 */
			if (!rc) {
				u32 avl;
				avl = le32_to_cpu(a->value_length);
				if (val_len == avl)
					return 0;
				if (val_len < avl) {
					errno = ENOENT;
					return -1;
				}
			}
			else if (rc < 0) {
				errno = ENOENT;
				return -1;
			}
		}
	}
	ntfs_log_trace("File is corrupt. Run chkdsk.\n");
	errno = EIO;
	return -1;
}

/**
 * ntfs_attr_lookup - find an attribute in an ntfs inode
 * @type:	attribute type to find
 * @name:	attribute name to find (optional, i.e. NULL means don't care)
 * @name_len:	attribute name length (only needed if @name present)
 * @ic:		IGNORE_CASE or CASE_SENSITIVE (ignored if @name not present)
 * @lowest_vcn:	lowest vcn to find (optional, non-resident attributes only)
 * @val:	attribute value to find (optional, resident attributes only)
 * @val_len:	attribute value length
 * @ctx:	search context with mft record and attribute to search from
 *
 * Find an attribute in an ntfs inode. On first search @ctx->ntfs_ino must
 * be the base mft record and @ctx must have been obtained from a call to
 * ntfs_attr_get_search_ctx().
 *
 * This function transparently handles attribute lists and @ctx is used to
 * continue searches where they were left off at.
 *
 * If @type is AT_UNUSED, return the first found attribute, i.e. one can
 * enumerate all attributes by setting @type to AT_UNUSED and then calling
 * ntfs_attr_lookup() repeatedly until it returns -1 with errno set to ENOENT
 * to indicate that there are no more entries. During the enumeration, each
 * successful call of ntfs_attr_lookup() will return the next attribute, with
 * the current attribute being described by the search context @ctx.
 *
 * If @type is AT_END, seek to the end of the base mft record ignoring the
 * attribute list completely and return -1 with errno set to ENOENT.  AT_END is
 * not a valid attribute, its length is zero for example, thus it is safer to
 * return error instead of success in this case.  It should never be needed to
 * do this, but we implement the functionality because it allows for simpler
 * code inside ntfs_external_attr_find().
 *
 * If @name is AT_UNNAMED search for an unnamed attribute. If @name is present
 * but not AT_UNNAMED search for a named attribute matching @name. Otherwise,
 * match both named and unnamed attributes.
 *
 * After finishing with the attribute/mft record you need to call
 * ntfs_attr_put_search_ctx() to cleanup the search context (unmapping any
 * mapped extent inodes, etc).
 *
 * Return 0 if the search was successful and -1 if not, with errno set to the
 * error code.
 *
 * On success, @ctx->attr is the found attribute, it is in mft record
 * @ctx->mrec, and @ctx->al_entry is the attribute list entry for this
 * attribute with @ctx->base_* being the base mft record to which @ctx->attr
 * belongs.  If no attribute list attribute is present @ctx->al_entry and
 * @ctx->base_* are NULL.
 *
 * On error ENOENT, i.e. attribute not found, @ctx->attr is set to the
 * attribute which collates just after the attribute being searched for in the
 * base ntfs inode, i.e. if one wants to add the attribute to the mft record
 * this is the correct place to insert it into, and if there is not enough
 * space, the attribute should be placed in an extent mft record.
 * @ctx->al_entry points to the position within @ctx->base_ntfs_ino->attr_list
 * at which the new attribute's attribute list entry should be inserted.  The
 * other @ctx fields, base_ntfs_ino, base_mrec, and base_attr are set to NULL.
 * The only exception to this is when @type is AT_END, in which case
 * @ctx->al_entry is set to NULL also (see above).
 *
 * The following error codes are defined:
 *	ENOENT	Attribute not found, not an error as such.
 *	EINVAL	Invalid arguments.
 *	EIO	I/O error or corrupt data structures found.
 *	ENOMEM	Not enough memory to allocate necessary buffers.
 */
static int mkntfs_attr_lookup(OPTS* opts, const ATTR_TYPES type, const ntfschar* name,
	const u32 name_len, const IGNORE_CASE_BOOL ic,
	const VCN lowest_vcn, const u8* val,
	const u32 val_len, ntfs_attr_search_ctx* ctx)
{
	ntfs_inode* base_ni;

	if (!ctx || !ctx->mrec || !ctx->attr) {
		errno = EINVAL;
		return -1;
	}
	if (ctx->base_ntfs_ino)
		base_ni = ctx->base_ntfs_ino;
	else
		base_ni = ctx->ntfs_ino;
	if (!base_ni || !NInoAttrList(base_ni) || type == AT_ATTRIBUTE_LIST)
		return mkntfs_attr_find(opts, type, name, name_len, ic, val, val_len,
			ctx);
	errno = EOPNOTSUPP;
	return -1;
}

/**
 * insert_positioned_attr_in_mft_record
 *
 * Create a non-resident attribute with a predefined on disk location
 * specified by the runlist @rl. The clusters specified by @rl are assumed to
 * be allocated already.
 *
 * Return 0 on success and -errno on error.
 */
static int insert_positioned_attr_in_mft_record(OPTS* opts, MFT_RECORD* m,
	const ATTR_TYPES type, const char* name, u32 name_len,
	const IGNORE_CASE_BOOL ic, const ATTR_FLAGS flags,
	const runlist* rl, const u8* val, const s64 val_len)
{
	ntfs_attr_search_ctx* ctx;
	ATTR_RECORD* a;
	u16 hdr_size;
	int asize, mpa_size, err, i;
	s64 bw = 0, inited_size;
	VCN highest_vcn;
	ntfschar* uname = NULL;
	int uname_len = 0;
	/*
	if (base record)
		attr_lookup();
	else
	*/

	uname = ntfs_str2ucs(name, &uname_len);
	if (!uname)
		return -errno;

	/* Check if the attribute is already there. */
	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_error("Failed to allocate attribute search context.\n");
		err = -ENOMEM;
		goto err_out;
	}
	if (ic == IGNORE_CASE) {
		ntfs_log_error("FIXME: Hit unimplemented code path #1.\n");
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (!mkntfs_attr_lookup(opts, type, uname, uname_len, ic, 0, NULL, 0, ctx)) {
		err = -EEXIST;
		goto err_out;
	}
	if (errno != ENOENT) {
		ntfs_log_error("Corrupt inode.\n");
		err = -errno;
		goto err_out;
	}
	a = ctx->attr;
	if (flags & ATTR_COMPRESSION_MASK) {
		ntfs_log_error("Compressed attributes not supported yet.\n");
		/* FIXME: Compress attribute into a temporary buffer, set */
		/* val accordingly and save the compressed size. */
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (flags & (ATTR_IS_ENCRYPTED | ATTR_IS_SPARSE)) {
		ntfs_log_error("Encrypted/sparse attributes not supported.\n");
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (flags & ATTR_COMPRESSION_MASK) {
		hdr_size = 72;
		/* FIXME: This compression stuff is all wrong. Never mind for */
		/* now. (AIA) */
		if (val_len)
			mpa_size = 0; /* get_size_for_compressed_mapping_pairs(rl); */
		else
			mpa_size = 0;
	}
	else {
		hdr_size = 64;
		if (val_len) {
			mpa_size = ntfs_get_size_for_mapping_pairs(opts->g_vol, rl, 0, INT_MAX);
			if (mpa_size < 0) {
				err = -errno;
				ntfs_log_error("Failed to get size for mapping "
					"pairs.\n");
				goto err_out;
			}
		}
		else {
			mpa_size = 0;
		}
	}
	/* Mapping pairs array and next attribute must be 8-byte aligned. */
	asize = (((int)hdr_size + ((name_len + 7) & ~7) + mpa_size) + 7) & ~7;
	/* Get the highest vcn. */
	for (i = 0, highest_vcn = 0LL; rl[i].length; i++)
		highest_vcn += rl[i].length;
	/* Does the value fit inside the allocated size? */
	if (highest_vcn * opts->g_vol->cluster_size < val_len) {
		ntfs_log_error("BUG: Allocated size is smaller than data size!\n");
		err = -EINVAL;
		goto err_out;
	}
	err = make_room_for_attribute(m, (char*)a, asize);
	if (err == -ENOSPC) {
		/*
		 * FIXME: Make space! (AIA)
		 * can we make it non-resident? if yes, do that.
		 *	does it fit now? yes -> do it.
		 * m's $DATA or $BITMAP+$INDEX_ALLOCATION resident?
		 * yes -> make non-resident
		 *	does it fit now? yes -> do it.
		 * make all attributes non-resident
		 *	does it fit now? yes -> do it.
		 * m is a base record? yes -> allocate extension record
		 *	does the new attribute fit in there? yes -> do it.
		 * split up runlist into extents and place each in an extension
		 * record.
		 * FIXME: the check for needing extension records should be
		 * earlier on as it is very quick: asize > m->bytes_allocated?
		 */
		err = -EOPNOTSUPP;
		goto err_out;
#ifdef DEBUG
	}
	else if (err == -EINVAL) {
		ntfs_log_error("BUG(): in insert_positioned_attribute_in_mft_"
			"record(): make_room_for_attribute() returned "
			"error: EINVAL!\n");
		goto err_out;
#endif
	}
	a->type = type;
	a->length = cpu_to_le32(asize);
	a->non_resident = 1;
	a->name_length = name_len;
	a->name_offset = cpu_to_le16(hdr_size);
	a->flags = flags;
	a->instance = m->next_attr_instance;
	m->next_attr_instance = cpu_to_le16((le16_to_cpu(m->next_attr_instance)
		+ 1) & 0xffff);
	a->lowest_vcn = const_cpu_to_sle64(0);
	a->highest_vcn = cpu_to_sle64(highest_vcn - 1LL);
	a->mapping_pairs_offset = cpu_to_le16(hdr_size + ((name_len + 7) & ~7));
	memset(a->reserved1, 0, sizeof(a->reserved1));
	/* FIXME: Allocated size depends on compression. */
	a->allocated_size = cpu_to_sle64(highest_vcn * opts->g_vol->cluster_size);
	a->data_size = cpu_to_sle64(val_len);
	if (name_len)
		memcpy((char*)a + hdr_size, uname, name_len << 1);
	if (flags & ATTR_COMPRESSION_MASK) {
		if (flags & ATTR_COMPRESSION_MASK & ~ATTR_IS_COMPRESSED) {
			ntfs_log_error("Unknown compression format. Reverting "
				"to standard compression.\n");
			a->flags &= ~ATTR_COMPRESSION_MASK;
			a->flags |= ATTR_IS_COMPRESSED;
		}
		a->compression_unit = 4;
		inited_size = val_len;
		/* FIXME: Set the compressed size. */
		a->compressed_size = const_cpu_to_sle64(0);
		/* FIXME: Write out the compressed data. */
		/* FIXME: err = build_mapping_pairs_compressed(); */
		err = -EOPNOTSUPP;
	}
	else {
		a->compression_unit = 0;
		if ((type == AT_DATA)
			&& (m->mft_record_number
				== const_cpu_to_le32(FILE_LogFile)))
			bw = ntfs_rlwrite(opts, opts->g_vol->dev, rl, val, val_len,
				&inited_size, WRITE_LOGFILE);
		else
			bw = ntfs_rlwrite(opts, opts->g_vol->dev, rl, val, val_len,
				&inited_size, WRITE_STANDARD);
		if (bw != val_len) {
			ntfs_log_error("Error writing non-resident attribute "
				"value.\n");
			return -errno;
		}
		err = ntfs_mapping_pairs_build(opts->g_vol, (u8*)a + hdr_size +
			((name_len + 7) & ~7), mpa_size, rl, 0, NULL);
	}
	a->initialized_size = cpu_to_sle64(inited_size);
	if (err < 0 || bw != val_len) {
		/* FIXME: Handle error. */
		/* deallocate clusters */
		/* remove attribute */
		if (err >= 0)
			err = -EIO;
		ntfs_log_error("insert_positioned_attr_in_mft_record failed "
			"with error %i.\n", err < 0 ? err : (int)bw);
	}
err_out:
	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	ntfs_ucsfree(uname);
	return err;
}

/**
 * insert_non_resident_attr_in_mft_record
 *
 * Return 0 on success and -errno on error.
 */
static int insert_non_resident_attr_in_mft_record(OPTS* opts, MFT_RECORD* m,
	const ATTR_TYPES type, const char* name, u32 name_len,
	const IGNORE_CASE_BOOL ic, const ATTR_FLAGS flags,
	const u8* val, const s64 val_len,
	WRITE_TYPE write_type)
{
	ntfs_attr_search_ctx* ctx;
	ATTR_RECORD* a;
	u16 hdr_size;
	int asize, mpa_size, err, i;
	runlist* rl = NULL;
	s64 bw = 0;
	ntfschar* uname = NULL;
	int uname_len = 0;
	/*
	if (base record)
		attr_lookup();
	else
	*/

	uname = ntfs_str2ucs(name, &uname_len);
	if (!uname)
		return -errno;

	/* Check if the attribute is already there. */
	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_error("Failed to allocate attribute search context.\n");
		err = -ENOMEM;
		goto err_out;
	}
	if (ic == IGNORE_CASE) {
		ntfs_log_error("FIXME: Hit unimplemented code path #2.\n");
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (!mkntfs_attr_lookup(opts, type, uname, uname_len, ic, 0, NULL, 0, ctx)) {
		err = -EEXIST;
		goto err_out;
	}
	if (errno != ENOENT) {
		ntfs_log_error("Corrupt inode.\n");
		err = -errno;
		goto err_out;
	}
	a = ctx->attr;
	if (flags & ATTR_COMPRESSION_MASK) {
		ntfs_log_error("Compressed attributes not supported yet.\n");
		/* FIXME: Compress attribute into a temporary buffer, set */
		/* val accordingly and save the compressed size. */
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (flags & (ATTR_IS_ENCRYPTED | ATTR_IS_SPARSE)) {
		ntfs_log_error("Encrypted/sparse attributes not supported.\n");
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (val_len) {
		rl = allocate_scattered_clusters(opts, (val_len +
			opts->g_vol->cluster_size - 1) / opts->g_vol->cluster_size);
		if (!rl) {
			err = -errno;
			ntfs_log_perror("Failed to allocate scattered clusters");
			goto err_out;
		}
	}
	else {
		rl = NULL;
	}
	if (flags & ATTR_COMPRESSION_MASK) {
		hdr_size = 72;
		/* FIXME: This compression stuff is all wrong. Never mind for */
		/* now. (AIA) */
		if (val_len)
			mpa_size = 0; /* get_size_for_compressed_mapping_pairs(rl); */
		else
			mpa_size = 0;
	}
	else {
		hdr_size = 64;
		if (val_len) {
			mpa_size = ntfs_get_size_for_mapping_pairs(opts->g_vol, rl, 0, INT_MAX);
			if (mpa_size < 0) {
				err = -errno;
				ntfs_log_error("Failed to get size for mapping "
					"pairs.\n");
				goto err_out;
			}
		}
		else {
			mpa_size = 0;
		}
	}
	/* Mapping pairs array and next attribute must be 8-byte aligned. */
	asize = (((int)hdr_size + ((name_len + 7) & ~7) + mpa_size) + 7) & ~7;
	err = make_room_for_attribute(m, (char*)a, asize);
	if (err == -ENOSPC) {
		/*
		 * FIXME: Make space! (AIA)
		 * can we make it non-resident? if yes, do that.
		 *	does it fit now? yes -> do it.
		 * m's $DATA or $BITMAP+$INDEX_ALLOCATION resident?
		 * yes -> make non-resident
		 *	does it fit now? yes -> do it.
		 * make all attributes non-resident
		 *	does it fit now? yes -> do it.
		 * m is a base record? yes -> allocate extension record
		 *	does the new attribute fit in there? yes -> do it.
		 * split up runlist into extents and place each in an extension
		 * record.
		 * FIXME: the check for needing extension records should be
		 * earlier on as it is very quick: asize > m->bytes_allocated?
		 */
		err = -EOPNOTSUPP;
		goto err_out;
#ifdef DEBUG
	}
	else if (err == -EINVAL) {
		ntfs_log_error("BUG(): in insert_non_resident_attribute_in_"
			"mft_record(): make_room_for_attribute() "
			"returned error: EINVAL!\n");
		goto err_out;
#endif
	}
	a->type = type;
	a->length = cpu_to_le32(asize);
	a->non_resident = 1;
	a->name_length = name_len;
	a->name_offset = cpu_to_le16(hdr_size);
	a->flags = flags;
	a->instance = m->next_attr_instance;
	m->next_attr_instance = cpu_to_le16((le16_to_cpu(m->next_attr_instance)
		+ 1) & 0xffff);
	a->lowest_vcn = const_cpu_to_sle64(0);
	for (i = 0; rl[i].length; i++)
		;
	a->highest_vcn = cpu_to_sle64(rl[i].vcn - 1);
	a->mapping_pairs_offset = cpu_to_le16(hdr_size + ((name_len + 7) & ~7));
	memset(a->reserved1, 0, sizeof(a->reserved1));
	/* FIXME: Allocated size depends on compression. */
	a->allocated_size = cpu_to_sle64((val_len + (opts->g_vol->cluster_size - 1)) &
		~(opts->g_vol->cluster_size - 1));
	a->data_size = cpu_to_sle64(val_len);
	a->initialized_size = cpu_to_sle64(val_len);
	if (name_len)
		memcpy((char*)a + hdr_size, uname, name_len << 1);
	if (flags & ATTR_COMPRESSION_MASK) {
		if (flags & ATTR_COMPRESSION_MASK & ~ATTR_IS_COMPRESSED) {
			ntfs_log_error("Unknown compression format. Reverting "
				"to standard compression.\n");
			a->flags &= ~ATTR_COMPRESSION_MASK;
			a->flags |= ATTR_IS_COMPRESSED;
		}
		a->compression_unit = 4;
		/* FIXME: Set the compressed size. */
		a->compressed_size = const_cpu_to_sle64(0);
		/* FIXME: Write out the compressed data. */
		/* FIXME: err = build_mapping_pairs_compressed(); */
		err = -EOPNOTSUPP;
	}
	else {
		a->compression_unit = 0;
		bw = ntfs_rlwrite(opts, opts->g_vol->dev, rl, val, val_len, NULL,
			write_type);
		if (bw != val_len) {
			ntfs_log_error("Error writing non-resident attribute "
				"value.\n");
			return -errno;
		}
		err = ntfs_mapping_pairs_build(opts->g_vol, (u8*)a + hdr_size +
			((name_len + 7) & ~7), mpa_size, rl, 0, NULL);
	}
	if (err < 0 || bw != val_len) {
		/* FIXME: Handle error. */
		/* deallocate clusters */
		/* remove attribute */
		if (err >= 0)
			err = -EIO;
		ntfs_log_error("insert_non_resident_attr_in_mft_record failed with "
			"error %lld.\n", (long long)(err < 0 ? err : bw));
	}
err_out:
	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	ntfs_ucsfree(uname);
	free(rl);
	return err;
}

/**
 * insert_non_resident_attr_in_mft_record_nowrite
 *
 * Return 0 on success and -errno on error.
 */
static int insert_non_resident_attr_in_mft_record_nowrite(OPTS* opts, MFT_RECORD* m,
	const ATTR_TYPES type, const char* name, u32 name_len,
	const IGNORE_CASE_BOOL ic, const ATTR_FLAGS flags,
	const s64 val_len, runlist** o_rl)
{
	ntfs_attr_search_ctx* ctx;
	ATTR_RECORD* a;
	u16 hdr_size;
	int asize, mpa_size, err, i;
	ntfschar* uname = NULL;
	int uname_len = 0;
	/*
	if (base record)
		attr_lookup();
	else
	*/

	uname = ntfs_str2ucs(name, &uname_len);
	if (!uname)
		return -errno;

	/* Check if the attribute is already there. */
	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_error("Failed to allocate attribute search context.\n");
		err = -ENOMEM;
		goto err_out;
	}
	if (ic == IGNORE_CASE) {
		ntfs_log_error("FIXME: Hit unimplemented code path #2.\n");
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (!mkntfs_attr_lookup(opts, type, uname, uname_len, ic, 0, NULL, 0, ctx)) {
		err = -EEXIST;
		goto err_out;
	}
	if (errno != ENOENT) {
		ntfs_log_error("Corrupt inode.\n");
		err = -errno;
		goto err_out;
	}
	a = ctx->attr;
	if (flags & ATTR_COMPRESSION_MASK) {
		ntfs_log_error("Compressed attributes not supported yet.\n");
		/* FIXME: Compress attribute into a temporary buffer, set */
		/* val accordingly and save the compressed size. */
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (flags & (ATTR_IS_ENCRYPTED | ATTR_IS_SPARSE)) {
		ntfs_log_error("Encrypted/sparse attributes not supported.\n");
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (val_len) {
		*o_rl = allocate_scattered_clusters(opts, (val_len +
			opts->g_vol->cluster_size - 1) / opts->g_vol->cluster_size);
		if (!*o_rl) {
			err = -errno;
			ntfs_log_perror("Failed to allocate scattered clusters");
			goto err_out;
		}
	}
	else {
		*o_rl = NULL;
	}
	if (flags & ATTR_COMPRESSION_MASK) {
		hdr_size = 72;
		/* FIXME: This compression stuff is all wrong. Never mind for */
		/* now. (AIA) */
		if (val_len)
			mpa_size = 0; /* get_size_for_compressed_mapping_pairs(rl); */
		else
			mpa_size = 0;
	}
	else {
		hdr_size = 64;
		if (val_len) {
			mpa_size = ntfs_get_size_for_mapping_pairs(opts->g_vol, *o_rl, 0, INT_MAX);
			if (mpa_size < 0) {
				err = -errno;
				ntfs_log_error("Failed to get size for mapping "
					"pairs.\n");
				goto err_out;
			}
		}
		else {
			mpa_size = 0;
		}
	}
	/* Mapping pairs array and next attribute must be 8-byte aligned. */
	asize = (((int)hdr_size + ((name_len + 7) & ~7) + mpa_size) + 7) & ~7;
	err = make_room_for_attribute(m, (char*)a, asize);
	if (err == -ENOSPC) {
		/*
		 * FIXME: Make space! (AIA)
		 * can we make it non-resident? if yes, do that.
		 *	does it fit now? yes -> do it.
		 * m's $DATA or $BITMAP+$INDEX_ALLOCATION resident?
		 * yes -> make non-resident
		 *	does it fit now? yes -> do it.
		 * make all attributes non-resident
		 *	does it fit now? yes -> do it.
		 * m is a base record? yes -> allocate extension record
		 *	does the new attribute fit in there? yes -> do it.
		 * split up runlist into extents and place each in an extension
		 * record.
		 * FIXME: the check for needing extension records should be
		 * earlier on as it is very quick: asize > m->bytes_allocated?
		 */
		err = -EOPNOTSUPP;
		goto err_out;
#ifdef DEBUG
	}
	else if (err == -EINVAL) {
		ntfs_log_error("BUG(): in insert_non_resident_attribute_in_"
			"mft_record(): make_room_for_attribute() "
			"returned error: EINVAL!\n");
		goto err_out;
#endif
	}
	a->type = type;
	a->length = cpu_to_le32(asize);
	a->non_resident = 1;
	a->name_length = name_len;
	a->name_offset = cpu_to_le16(hdr_size);
	a->flags = flags;
	a->instance = m->next_attr_instance;
	m->next_attr_instance = cpu_to_le16((le16_to_cpu(m->next_attr_instance)
		+ 1) & 0xffff);
	a->lowest_vcn = const_cpu_to_sle64(0);
	for (i = 0; (*o_rl)[i].length; i++)
		;
	a->highest_vcn = cpu_to_sle64((*o_rl)[i].vcn - 1);
	a->mapping_pairs_offset = cpu_to_le16(hdr_size + ((name_len + 7) & ~7));
	memset(a->reserved1, 0, sizeof(a->reserved1));
	/* FIXME: Allocated size depends on compression. */
	a->allocated_size = cpu_to_sle64((val_len + ((s64)opts->g_vol->cluster_size - 1)) &
		~((s64)opts->g_vol->cluster_size - 1));
	a->data_size = cpu_to_sle64(val_len);
	a->initialized_size = cpu_to_sle64(val_len);
	if (name_len)
		memcpy((char*)a + hdr_size, uname, name_len << 1);
	if (flags & ATTR_COMPRESSION_MASK) {
		if (flags & ATTR_COMPRESSION_MASK & ~ATTR_IS_COMPRESSED) {
			ntfs_log_error("Unknown compression format. Reverting "
				"to standard compression.\n");
			a->flags &= ~ATTR_COMPRESSION_MASK;
			a->flags |= ATTR_IS_COMPRESSED;
		}
		a->compression_unit = 4;
		/* FIXME: Set the compressed size. */
		a->compressed_size = const_cpu_to_sle64(0);
		/* FIXME: Write out the compressed data. */
		/* FIXME: err = build_mapping_pairs_compressed(); */
		err = -EOPNOTSUPP;
	}
	else {
		a->compression_unit = 0;
		//bw = ntfs_rlwrite(g_vol->dev, rl, val, val_len, NULL,
		//	write_type);
		//if (bw != val_len) {
		//	ntfs_log_error("Error writing non-resident attribute "
		//		"value.\n");
		//	return -errno;
		//}
		err = ntfs_mapping_pairs_build(opts->g_vol, (u8*)a + hdr_size +
			((name_len + 7) & ~7), mpa_size, *o_rl, 0, NULL);
	}
	if (err < 0) {
		/* FIXME: Handle error. */
		/* deallocate clusters */
		/* remove attribute */
		if (err >= 0)
			err = -EIO;
		ntfs_log_error("insert_non_resident_attr_in_mft_record failed with "
			"error %lld.\n", (long long)(err));
	}
err_out:
	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	ntfs_ucsfree(uname);
	return err;
}

/**
 * insert_resident_attr_in_mft_record
 *
 * Return 0 on success and -errno on error.
 */
static int insert_resident_attr_in_mft_record(OPTS* opts, MFT_RECORD* m,
	const ATTR_TYPES type, const char* name, u32 name_len,
	const IGNORE_CASE_BOOL ic, const ATTR_FLAGS flags,
	const RESIDENT_ATTR_FLAGS res_flags,
	const u8* val, const u32 val_len)
{
	ntfs_attr_search_ctx* ctx;
	ATTR_RECORD* a;
	int asize, err;
	ntfschar* uname = NULL;
	int uname_len = 0;
	/*
	if (base record)
		mkntfs_attr_lookup();
	else
	*/

	uname = ntfs_str2ucs(name, &uname_len);
	if (!uname)
		return -errno;

	/* Check if the attribute is already there. */
	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_error("Failed to allocate attribute search context.\n");
		err = -ENOMEM;
		goto err_out;
	}
	if (ic == IGNORE_CASE) {
		ntfs_log_error("FIXME: Hit unimplemented code path #3.\n");
		err = -EOPNOTSUPP;
		goto err_out;
	}
	if (!mkntfs_attr_lookup(opts, type, uname, uname_len, ic, 0, val, val_len,
		ctx)) {
		err = -EEXIST;
		goto err_out;
	}
	if (errno != ENOENT) {
		ntfs_log_error("Corrupt inode.\n");
		err = -errno;
		goto err_out;
	}
	a = ctx->attr;
	/* sizeof(resident attribute record header) == 24 */
	asize = ((24 + ((name_len * 2 + 7) & ~7) + val_len) + 7) & ~7;
	err = make_room_for_attribute(m, (char*)a, asize);
	if (err == -ENOSPC) {
		/*
		 * FIXME: Make space! (AIA)
		 * can we make it non-resident? if yes, do that.
		 *	does it fit now? yes -> do it.
		 * m's $DATA or $BITMAP+$INDEX_ALLOCATION resident?
		 * yes -> make non-resident
		 *	does it fit now? yes -> do it.
		 * make all attributes non-resident
		 *	does it fit now? yes -> do it.
		 * m is a base record? yes -> allocate extension record
		 *	does the new attribute fit in there? yes -> do it.
		 * split up runlist into extents and place each in an extension
		 * record.
		 * FIXME: the check for needing extension records should be
		 * earlier on as it is very quick: asize > m->bytes_allocated?
		 */
		err = -EOPNOTSUPP;
		goto err_out;
	}
#ifdef DEBUG
	if (err == -EINVAL) {
		ntfs_log_error("BUG(): in insert_resident_attribute_in_mft_"
			"record(): make_room_for_attribute() returned "
			"error: EINVAL!\n");
		goto err_out;
	}
#endif
	a->type = type;
	a->length = cpu_to_le32(asize);
	a->non_resident = 0;
	a->name_length = name_len;
	if (type == AT_OBJECT_ID)
		a->name_offset = const_cpu_to_le16(0);
	else
		a->name_offset = const_cpu_to_le16(24);
	a->flags = flags;
	a->instance = m->next_attr_instance;
	m->next_attr_instance = cpu_to_le16((le16_to_cpu(m->next_attr_instance)
		+ 1) & 0xffff);
	a->value_length = cpu_to_le32(val_len);
	a->value_offset = cpu_to_le16(24 + ((name_len * 2 + 7) & ~7));
	a->resident_flags = res_flags;
	a->reservedR = 0;
	if (name_len)
		memcpy((char*)a + 24, uname, name_len << 1);
	if (val_len)
		memcpy((char*)a + le16_to_cpu(a->value_offset), val, val_len);
err_out:
	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	ntfs_ucsfree(uname);
	return err;
}


/**
 * add_attr_std_info
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_std_info(OPTS* opts, MFT_RECORD* m, const FILE_ATTR_FLAGS flags,
	le32 security_id)
{
	STANDARD_INFORMATION si;
	int err, sd_size;

	sd_size = 48;

	si.creation_time = mkntfs_time();
	si.last_data_change_time = si.creation_time;
	si.last_mft_change_time = si.creation_time;
	si.last_access_time = si.creation_time;
	si.file_attributes = flags; /* already LE */
	si.maximum_versions = const_cpu_to_le32(0);
	si.version_number = const_cpu_to_le32(0);
	si.class_id = const_cpu_to_le32(0);
	si.security_id = security_id;
	if (si.security_id != const_cpu_to_le32(0))
		sd_size = 72;
	/* FIXME: $Quota support... */
	si.owner_id = const_cpu_to_le32(0);
	si.quota_charged = const_cpu_to_le64(0ULL);
	/* FIXME: $UsnJrnl support... Not needed on fresh w2k3-volume */
	si.usn = const_cpu_to_le64(0ULL);
	/* NTFS 1.2: size of si = 48, NTFS 3.[01]: size of si = 72 */
	err = insert_resident_attr_in_mft_record(opts, m, AT_STANDARD_INFORMATION,
		NULL, 0, CASE_SENSITIVE, const_cpu_to_le16(0),
		0, (u8*)&si, sd_size);
	if (err < 0)
		ntfs_log_perror("add_attr_std_info failed");
	return err;
}

/*
 *		Tell whether the unnamed data is non resident
 */

static BOOL non_resident_unnamed_data(OPTS* opts, MFT_RECORD* m)
{
	ATTR_RECORD* a;
	ntfs_attr_search_ctx* ctx;
	BOOL nonres;

	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (ctx && !mkntfs_attr_find(opts, AT_DATA,
		(const ntfschar*)NULL, 0, CASE_SENSITIVE,
		(u8*)NULL, 0, ctx)) {
		a = ctx->attr;
		nonres = a->non_resident != 0;
	}
	else {
		ntfs_log_error("BUG: Unnamed data not found\n");
		nonres = TRUE;
	}
	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	return (nonres);
}

/*
 *		Get the time stored in the standard information attribute
 */

static ntfs_time stdinfo_time(OPTS* opts, MFT_RECORD* m)
{
	STANDARD_INFORMATION* si;
	ntfs_attr_search_ctx* ctx;
	ntfs_time info_time;

	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (ctx && !mkntfs_attr_find(opts, AT_STANDARD_INFORMATION,
		(const ntfschar*)NULL, 0, CASE_SENSITIVE,
		(u8*)NULL, 0, ctx)) {
		si = (STANDARD_INFORMATION*)((char*)ctx->attr +
			le16_to_cpu(ctx->attr->value_offset));
		info_time = si->creation_time;
	}
	else {
		ntfs_log_error("BUG: Standard information not found\n");
		info_time = mkntfs_time();
	}
	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	return (info_time);
}

/**
 * add_attr_file_name
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_file_name(OPTS* opts, MFT_RECORD* m, const leMFT_REF parent_dir,
	const s64 allocated_size, const s64 data_size,
	const FILE_ATTR_FLAGS flags, const u16 packed_ea_size,
	const u32 reparse_point_tag, const char* file_name,
	const FILE_NAME_TYPE_FLAGS file_name_type)
{
	ntfs_attr_search_ctx* ctx;
	STANDARD_INFORMATION* si;
	FILE_NAME_ATTR* fn;
	int i, fn_size;
	ntfschar* uname;

	/* Check if the attribute is already there. */
	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_error("Failed to get attribute search context.\n");
		return -ENOMEM;
	}
	if (mkntfs_attr_lookup(opts, AT_STANDARD_INFORMATION, AT_UNNAMED, 0,
		CASE_SENSITIVE, 0, NULL, 0, ctx)) {
		int eo = errno;
		ntfs_log_error("BUG: Standard information attribute not "
			"present in file record.\n");
		ntfs_attr_put_search_ctx(ctx);
		return -eo;
	}
	si = (STANDARD_INFORMATION*)((char*)ctx->attr +
		le16_to_cpu(ctx->attr->value_offset));
	i = (strlen(file_name) + 1) * sizeof(ntfschar);
	fn_size = sizeof(FILE_NAME_ATTR) + i;
	fn = ntfs_malloc(fn_size);
	if (!fn) {
		ntfs_attr_put_search_ctx(ctx);
		return -errno;
	}
	fn->parent_directory = parent_dir;

	fn->creation_time = si->creation_time;
	fn->last_data_change_time = si->last_data_change_time;
	fn->last_mft_change_time = si->last_mft_change_time;
	fn->last_access_time = si->last_access_time;
	ntfs_attr_put_search_ctx(ctx);

	fn->allocated_size = cpu_to_sle64(allocated_size);
	fn->data_size = cpu_to_sle64(data_size);
	fn->file_attributes = flags;
	/* These are in a union so can't have both. */
	if (packed_ea_size && reparse_point_tag) {
		free(fn);
		return -EINVAL;
	}
	if (packed_ea_size) {
		fn->packed_ea_size = cpu_to_le16(packed_ea_size);
		fn->reserved = const_cpu_to_le16(0);
	}
	else {
		fn->reparse_point_tag = cpu_to_le32(reparse_point_tag);
	}
	fn->file_name_type = file_name_type;
	uname = fn->file_name;
	i = ntfs_mbstoucs_libntfscompat(file_name, &uname, i);
	if (i < 1) {
		free(fn);
		return -EINVAL;
	}
	if (i > 0xff) {
		free(fn);
		return -ENAMETOOLONG;
	}
	/* No terminating null in file names. */
	fn->file_name_length = i;
	fn_size = sizeof(FILE_NAME_ATTR) + i * sizeof(ntfschar);
	i = insert_resident_attr_in_mft_record(opts, m, AT_FILE_NAME, NULL, 0,
		CASE_SENSITIVE, const_cpu_to_le16(0),
		RESIDENT_ATTR_IS_INDEXED, (u8*)fn, fn_size);
	free(fn);
	if (i < 0)
		ntfs_log_error("add_attr_file_name failed: %s\n", strerror(-i));
	return i;
}

/**
 * add_attr_sd
 *
 * Create the security descriptor attribute adding the security descriptor @sd
 * of length @sd_len to the mft record @m.
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_sd(OPTS* opts, MFT_RECORD* m, const u8* sd, const s64 sd_len)
{
	int err;

	/* Does it fit? NO: create non-resident. YES: create resident. */
	if (le32_to_cpu(m->bytes_in_use) + 24 + sd_len >
		le32_to_cpu(m->bytes_allocated))
		err = insert_non_resident_attr_in_mft_record(opts, m,
			AT_SECURITY_DESCRIPTOR, NULL, 0,
			CASE_SENSITIVE, const_cpu_to_le16(0), sd,
			sd_len, WRITE_STANDARD);
	else
		err = insert_resident_attr_in_mft_record(opts, m,
			AT_SECURITY_DESCRIPTOR, NULL, 0,
			CASE_SENSITIVE, const_cpu_to_le16(0), 0, sd,
			sd_len);
	if (err < 0)
		ntfs_log_error("add_attr_sd failed: %s\n", strerror(-err));
	return err;
}

/**
 * add_attr_data
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_data(OPTS* opts, MFT_RECORD* m, const char* name, const u32 name_len,
	const IGNORE_CASE_BOOL ic, const ATTR_FLAGS flags,
	const u8* val, const s64 val_len)
{
	int err;

	/*
	 * Does it fit? NO: create non-resident. YES: create resident.
	 *
	 * FIXME: Introduced arbitrary limit of mft record allocated size - 512.
	 * This is to get around the problem that if $Bitmap/$DATA becomes too
	 * big, but is just small enough to be resident, we would make it
	 * resident, and later run out of space when creating the other
	 * attributes and this would cause us to abort as making resident
	 * attributes non-resident is not supported yet.
	 * The proper fix is to support making resident attribute non-resident.
	 */
	if (le32_to_cpu(m->bytes_in_use) + 24 + val_len >
		min(le32_to_cpu(m->bytes_allocated),
			le32_to_cpu(m->bytes_allocated) - 512))
		err = insert_non_resident_attr_in_mft_record(opts, m, AT_DATA, name,
			name_len, ic, flags, val, val_len,
			WRITE_STANDARD);
	else
		err = insert_resident_attr_in_mft_record(opts, m, AT_DATA, name,
			name_len, ic, flags, 0, val, val_len);

	if (err < 0)
		ntfs_log_error("add_attr_data failed: %s\n", strerror(-err));
	return err;
}

/**
 * add_attr_data_positioned
 *
 * Create a non-resident data attribute with a predefined on disk location
 * specified by the runlist @rl. The clusters specified by @rl are assumed to
 * be allocated already.
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_data_positioned(OPTS* opts, MFT_RECORD* m, const char* name,
	const u32 name_len, const IGNORE_CASE_BOOL ic,
	const ATTR_FLAGS flags, const runlist* rl,
	const u8* val, const s64 val_len)
{
	int err;

	err = insert_positioned_attr_in_mft_record(opts, m, AT_DATA, name, name_len,
		ic, flags, rl, val, val_len);
	if (err < 0)
		ntfs_log_error("add_attr_data_positioned failed: %s\n",
			strerror(-err));
	return err;
}

/**
 * add_attr_vol_name
 *
 * Create volume name attribute specifying the volume name @vol_name as a null
 * terminated char string of length @vol_name_len (number of characters not
 * including the terminating null), which is converted internally to a little
 * endian ntfschar string. The name is at least 1 character long (though
 * Windows accepts zero characters), and at most 128 characters long (not
 * counting the terminating null).
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_vol_name(OPTS* opts, MFT_RECORD* m, const char* vol_name,
	const int vol_name_len)
{
	ntfschar* uname = NULL;
	int uname_len = 0;
	int i;

	if (vol_name) {
		uname_len = ntfs_mbstoucs(vol_name, &uname);
		if (uname_len < 0)
			return -errno;
		if (uname_len > 128) {
			free(uname);
			return -ENAMETOOLONG;
		}
	}
	i = insert_resident_attr_in_mft_record(opts, m, AT_VOLUME_NAME, NULL, 0,
		CASE_SENSITIVE, const_cpu_to_le16(0),
		0, (u8*)uname, uname_len * sizeof(ntfschar));
	free(uname);
	if (i < 0)
		ntfs_log_error("add_attr_vol_name failed: %s\n", strerror(-i));
	return i;
}

/**
 * add_attr_vol_info
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_vol_info(OPTS* opts, MFT_RECORD* m, const VOLUME_FLAGS flags,
	const u8 major_ver, const u8 minor_ver)
{
	VOLUME_INFORMATION vi;
	int err;

	memset(&vi, 0, sizeof(vi));
	vi.major_ver = major_ver;
	vi.minor_ver = minor_ver;
	vi.flags = flags & VOLUME_FLAGS_MASK;
	err = insert_resident_attr_in_mft_record(opts, m, AT_VOLUME_INFORMATION, NULL,
		0, CASE_SENSITIVE, const_cpu_to_le16(0),
		0, (u8*)&vi, sizeof(vi));
	if (err < 0)
		ntfs_log_error("add_attr_vol_info failed: %s\n", strerror(-err));
	return err;
}

/**
 * add_attr_index_root
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_index_root(OPTS* opts, MFT_RECORD* m, const char* name,
	const u32 name_len, const IGNORE_CASE_BOOL ic,
	const ATTR_TYPES indexed_attr_type,
	const COLLATION_RULES collation_rule,
	const u32 index_block_size)
{
	INDEX_ROOT* r;
	INDEX_ENTRY_HEADER* e;
	int err, val_len;

	val_len = sizeof(INDEX_ROOT) + sizeof(INDEX_ENTRY_HEADER);
	r = ntfs_malloc(val_len);
	if (!r)
		return -errno;
	r->type = (indexed_attr_type == AT_FILE_NAME)
		? AT_FILE_NAME : const_cpu_to_le32(0);
	if (indexed_attr_type == AT_FILE_NAME &&
		collation_rule != COLLATION_FILE_NAME) {
		free(r);
		ntfs_log_error("add_attr_index_root: indexed attribute is $FILE_NAME "
			"but collation rule is not COLLATION_FILE_NAME.\n");
		return -EINVAL;
	}
	r->collation_rule = collation_rule;
	r->index_block_size = cpu_to_le32(index_block_size);
	if (index_block_size >= opts->g_vol->cluster_size) {
		if (index_block_size % opts->g_vol->cluster_size) {
			ntfs_log_error("add_attr_index_root: index block size is not "
				"a multiple of the cluster size.\n");
			free(r);
			return -EINVAL;
		}
		r->clusters_per_index_block = index_block_size /
			opts->g_vol->cluster_size;
	}
	else { /* if (opts->g_vol->cluster_size > index_block_size) */
		if (index_block_size & (index_block_size - 1)) {
			ntfs_log_error("add_attr_index_root: index block size is not "
				"a power of 2.\n");
			free(r);
			return -EINVAL;
		}
		if (index_block_size < (u32)OPT_SECTOR_SIZE) {
			ntfs_log_error("add_attr_index_root: index block size "
				"is smaller than the sector size.\n");
			free(r);
			return -EINVAL;
		}
		r->clusters_per_index_block = index_block_size
			>> NTFS_BLOCK_SIZE_BITS;
	}
	memset(&r->reserved, 0, sizeof(r->reserved));
	r->index.entries_offset = const_cpu_to_le32(sizeof(INDEX_HEADER));
	r->index.index_length = const_cpu_to_le32(sizeof(INDEX_HEADER) +
		sizeof(INDEX_ENTRY_HEADER));
	r->index.allocated_size = r->index.index_length;
	r->index.ih_flags = SMALL_INDEX;
	e = (INDEX_ENTRY_HEADER*)((u8*)&r->index +
		le32_to_cpu(r->index.entries_offset));
	/*
	 * No matter whether this is a file index or a view as this is a
	 * termination entry, hence no key value / data is associated with it
	 * at all. Thus, we just need the union to be all zero.
	 */
	e->indexed_file = const_cpu_to_le64(0LL);
	e->length = const_cpu_to_le16(sizeof(INDEX_ENTRY_HEADER));
	e->key_length = const_cpu_to_le16(0);
	e->flags = INDEX_ENTRY_END;
	err = insert_resident_attr_in_mft_record(opts, m, AT_INDEX_ROOT, name,
		name_len, ic, const_cpu_to_le16(0), 0,
		(u8*)r, val_len);
	free(r);
	if (err < 0)
		ntfs_log_error("add_attr_index_root failed: %s\n", strerror(-err));
	return err;
}

/**
 * add_attr_index_alloc
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_index_alloc(OPTS* opts, MFT_RECORD* m, const char* name,
	const u32 name_len, const IGNORE_CASE_BOOL ic,
	const u8* index_alloc_val, const u32 index_alloc_val_len)
{
	int err;

	err = insert_non_resident_attr_in_mft_record(opts, m, AT_INDEX_ALLOCATION,
		name, name_len, ic, const_cpu_to_le16(0),
		index_alloc_val, index_alloc_val_len, WRITE_STANDARD);
	if (err < 0)
		ntfs_log_error("add_attr_index_alloc failed: %s\n", strerror(-err));
	return err;
}

/**
 * add_attr_bitmap
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_bitmap(OPTS* opts, MFT_RECORD* m, const char* name, const u32 name_len,
	const IGNORE_CASE_BOOL ic, const u8* bitmap,
	const u32 bitmap_len)
{
	int err;

	/* Does it fit? NO: create non-resident. YES: create resident. */
	if (le32_to_cpu(m->bytes_in_use) + 24 + bitmap_len >
		le32_to_cpu(m->bytes_allocated))
		err = insert_non_resident_attr_in_mft_record(opts, m, AT_BITMAP, name,
			name_len, ic, const_cpu_to_le16(0), bitmap,
			bitmap_len, WRITE_STANDARD);
	else
		err = insert_resident_attr_in_mft_record(opts, m, AT_BITMAP, name,
			name_len, ic, const_cpu_to_le16(0), 0,
			bitmap, bitmap_len);

	if (err < 0)
		ntfs_log_error("add_attr_bitmap failed: %s\n", strerror(-err));
	return err;
}

/**
 * add_attr_bitmap_positioned
 *
 * Create a non-resident bitmap attribute with a predefined on disk location
 * specified by the runlist @rl. The clusters specified by @rl are assumed to
 * be allocated already.
 *
 * Return 0 on success or -errno on error.
 */
static int add_attr_bitmap_positioned(OPTS* opts, MFT_RECORD* m, const char* name,
	const u32 name_len, const IGNORE_CASE_BOOL ic,
	const runlist* rl, const u8* bitmap, const u32 bitmap_len)
{
	int err;

	err = insert_positioned_attr_in_mft_record(opts, m, AT_BITMAP, name, name_len,
		ic, const_cpu_to_le16(0), rl, bitmap, bitmap_len);
	if (err < 0)
		ntfs_log_error("add_attr_bitmap_positioned failed: %s\n",
			strerror(-err));
	return err;
}


/**
 * upgrade_to_large_index
 *
 * Create bitmap and index allocation attributes, modify index root
 * attribute accordingly and move all of the index entries from the index root
 * into the index allocation.
 *
 * Return 0 on success or -errno on error.
 */
static int upgrade_to_large_index(OPTS* opts, MFT_RECORD* m, const char* name,
	u32 name_len, const IGNORE_CASE_BOOL ic,
	INDEX_ALLOCATION** idx)
{
	ntfs_attr_search_ctx* ctx;
	ATTR_RECORD* a;
	INDEX_ROOT* r;
	INDEX_ENTRY* re;
	INDEX_ALLOCATION* ia_val = NULL;
	ntfschar* uname = NULL;
	int uname_len = 0;
	u8 bmp[8];
	char* re_start, * re_end;
	int i, err, index_block_size;

	uname = ntfs_str2ucs(name, &uname_len);
	if (!uname)
		return -errno;

	/* Find the index root attribute. */
	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_error("Failed to allocate attribute search context.\n");
		ntfs_ucsfree(uname);
		return -ENOMEM;
	}
	if (ic == IGNORE_CASE) {
		ntfs_log_error("FIXME: Hit unimplemented code path #4.\n");
		err = -EOPNOTSUPP;
		ntfs_ucsfree(uname);
		goto err_out;
	}
	err = mkntfs_attr_lookup(opts, AT_INDEX_ROOT, uname, uname_len, ic, 0, NULL, 0,
		ctx);
	ntfs_ucsfree(uname);
	if (err) {
		err = -ENOTDIR;
		goto err_out;
	}
	a = ctx->attr;
	if (a->non_resident || a->flags) {
		err = -EINVAL;
		goto err_out;
	}
	r = (INDEX_ROOT*)((char*)a + le16_to_cpu(a->value_offset));
	re_end = (char*)r + le32_to_cpu(a->value_length);
	re_start = (char*)&r->index + le32_to_cpu(r->index.entries_offset);
	re = (INDEX_ENTRY*)re_start;
	index_block_size = le32_to_cpu(r->index_block_size);
	memset(bmp, 0, sizeof(bmp));
	ntfs_bit_set(bmp, 0ULL, 1);
	/* Bitmap has to be at least 8 bytes in size. */
	err = add_attr_bitmap(opts, m, name, name_len, ic, bmp, sizeof(bmp));
	if (err)
		goto err_out;
	ia_val = ntfs_calloc(index_block_size);
	if (!ia_val) {
		err = -errno;
		goto err_out;
	}
	/* Setup header. */
	ia_val->magic = magic_INDX;
	ia_val->usa_ofs = const_cpu_to_le16(sizeof(INDEX_ALLOCATION));
	if (index_block_size >= NTFS_BLOCK_SIZE) {
		ia_val->usa_count = cpu_to_le16(index_block_size /
			NTFS_BLOCK_SIZE + 1);
	}
	else {
		ia_val->usa_count = const_cpu_to_le16(1);
	}
	/* Set USN to 1. */
	*(le16*)((char*)ia_val + le16_to_cpu(ia_val->usa_ofs)) =
		const_cpu_to_le16(1);
	ia_val->lsn = const_cpu_to_sle64(0);
	ia_val->index_block_vcn = const_cpu_to_sle64(0);
	ia_val->index.ih_flags = LEAF_NODE;
	/* Align to 8-byte boundary. */
	ia_val->index.entries_offset = cpu_to_le32((sizeof(INDEX_HEADER) +
		le16_to_cpu(ia_val->usa_count) * 2 + 7) & ~7);
	ia_val->index.allocated_size = cpu_to_le32(index_block_size -
		(sizeof(INDEX_ALLOCATION) - sizeof(INDEX_HEADER)));
	/* Find the last entry in the index root and save it in re. */
	while ((char*)re < re_end && !(re->ie_flags & INDEX_ENTRY_END)) {
		/* Next entry in index root. */
		re = (INDEX_ENTRY*)((char*)re + le16_to_cpu(re->length));
	}
	/* Copy all the entries including the termination entry. */
	i = (char*)re - re_start + le16_to_cpu(re->length);
	memcpy((char*)&ia_val->index +
		le32_to_cpu(ia_val->index.entries_offset), re_start, i);
	/* Finish setting up index allocation. */
	ia_val->index.index_length = cpu_to_le32(i +
		le32_to_cpu(ia_val->index.entries_offset));
	/* Move the termination entry forward to the beginning if necessary. */
	if ((char*)re > re_start) {
		memmove(re_start, (char*)re, le16_to_cpu(re->length));
		re = (INDEX_ENTRY*)re_start;
	}
	/* Now fixup empty index root with pointer to index allocation VCN 0. */
	r->index.ih_flags = LARGE_INDEX;
	re->ie_flags |= INDEX_ENTRY_NODE;
	if (le16_to_cpu(re->length) < sizeof(INDEX_ENTRY_HEADER) + sizeof(VCN))
		re->length = cpu_to_le16(le16_to_cpu(re->length) + sizeof(VCN));
	r->index.index_length = cpu_to_le32(le32_to_cpu(r->index.entries_offset)
		+ le16_to_cpu(re->length));
	r->index.allocated_size = r->index.index_length;
	/* Resize index root attribute. */
	if (ntfs_resident_attr_value_resize(m, a, sizeof(INDEX_ROOT) -
		sizeof(INDEX_HEADER) +
		le32_to_cpu(r->index.allocated_size))) {
		/* TODO: Remove the added bitmap! */
		/* Revert index root from index allocation. */
		err = -errno;
		goto err_out;
	}
	/* Set VCN pointer to 0LL. */
	*(leVCN*)((char*)re + le16_to_cpu(re->length) - sizeof(VCN)) =
		const_cpu_to_sle64(0);
	err = ntfs_mst_pre_write_fixup((NTFS_RECORD*)ia_val, index_block_size);
	if (err) {
		err = -errno;
		ntfs_log_error("ntfs_mst_pre_write_fixup() failed in "
			"upgrade_to_large_index.\n");
		goto err_out;
	}
	err = add_attr_index_alloc(opts, m, name, name_len, ic, (u8*)ia_val,
		index_block_size);
	ntfs_mst_post_write_fixup((NTFS_RECORD*)ia_val);
	if (err) {
		/* TODO: Remove the added bitmap! */
		/* Revert index root from index allocation. */
		goto err_out;
	}
	*idx = ia_val;
	ntfs_attr_put_search_ctx(ctx);
	return 0;
err_out:
	ntfs_attr_put_search_ctx(ctx);
	free(ia_val);
	return err;
}

/**
 * make_room_for_index_entry_in_index_block
 *
 * Create space of @size bytes at position @pos inside the index block @idx.
 *
 * Return 0 on success or -errno on error.
 */
static int make_room_for_index_entry_in_index_block(INDEX_BLOCK* idx,
	INDEX_ENTRY* pos, u32 size)
{
	u32 biu;

	if (!size)
		return 0;
	biu = le32_to_cpu(idx->index.index_length);
	/* Do we have enough space? */
	if (biu + size > le32_to_cpu(idx->index.allocated_size))
		return -ENOSPC;
	/* Move everything after pos to pos + size. */
	memmove((char*)pos + size, (char*)pos, biu - ((char*)pos -
		(char*)&idx->index));
	/* Update index block. */
	idx->index.index_length = cpu_to_le32(biu + size);
	return 0;
}

/**
 * ntfs_index_keys_compare
 *
 * not all types of COLLATION_RULES supported yet...
 * added as needed.. (remove this comment when all are added)
 */
static int ntfs_index_keys_compare(u8* key1, u8* key2, int key1_length,
	int key2_length, COLLATION_RULES collation_rule)
{
	u32 u1, u2;
	int i;

	if (collation_rule == COLLATION_NTOFS_ULONG) {
		/* i.e. $SII or $QUOTA-$Q */
		u1 = le32_to_cpup((const le32*)key1);
		u2 = le32_to_cpup((const le32*)key2);
		if (u1 < u2)
			return -1;
		if (u1 > u2)
			return 1;
		/* u1 == u2 */
		return 0;
	}
	if (collation_rule == COLLATION_NTOFS_ULONGS) {
		/* i.e $OBJID-$O */
		i = 0;
		while (i < min(key1_length, key2_length)) {
			u1 = le32_to_cpup((const le32*)(key1 + i));
			u2 = le32_to_cpup((const le32*)(key2 + i));
			if (u1 < u2)
				return -1;
			if (u1 > u2)
				return 1;
			/* u1 == u2 */
			i += sizeof(u32);
		}
		if (key1_length < key2_length)
			return -1;
		if (key1_length > key2_length)
			return 1;
		return 0;
	}
	if (collation_rule == COLLATION_NTOFS_SECURITY_HASH) {
		/* i.e. $SDH */
		u1 = le32_to_cpu(((SDH_INDEX_KEY*)key1)->hash);
		u2 = le32_to_cpu(((SDH_INDEX_KEY*)key2)->hash);
		if (u1 < u2)
			return -1;
		if (u1 > u2)
			return 1;
		/* u1 == u2 */
		u1 = le32_to_cpu(((SDH_INDEX_KEY*)key1)->security_id);
		u2 = le32_to_cpu(((SDH_INDEX_KEY*)key2)->security_id);
		if (u1 < u2)
			return -1;
		if (u1 > u2)
			return 1;
		return 0;
	}
	if (collation_rule == COLLATION_NTOFS_SID) {
		/* i.e. $QUOTA-O */
		i = memcmp(key1, key2, min(key1_length, key2_length));
		if (!i) {
			if (key1_length < key2_length)
				return -1;
			if (key1_length > key2_length)
				return 1;
		}
		return i;
	}
	ntfs_log_critical("ntfs_index_keys_compare called without supported "
		"collation rule.\n");
	return 0;	/* Claim they're equal.  What else can we do? */
}

/**
 * insert_index_entry_in_res_dir_index
 *
 * i.e. insert an index_entry in some named index_root
 * simplified search method, works for mkntfs
 */
static int insert_index_entry_in_res_dir_index(OPTS* opts, INDEX_ENTRY* idx, u32 idx_size,
	MFT_RECORD* m, ntfschar* name, u32 name_size, ATTR_TYPES type)
{
	ntfs_attr_search_ctx* ctx;
	INDEX_HEADER* idx_header;
	INDEX_ENTRY* idx_entry, * idx_end;
	ATTR_RECORD* a;
	COLLATION_RULES collation_rule;
	int err, i;

	err = 0;
	/* does it fit ?*/
	if (opts->g_vol->mft_record_size > idx_size + le32_to_cpu(m->bytes_allocated))
		return -ENOSPC;
	/* find the INDEX_ROOT attribute:*/
	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_error("Failed to allocate attribute search "
			"context.\n");
		err = -ENOMEM;
		goto err_out;
	}
	if (mkntfs_attr_lookup(opts, AT_INDEX_ROOT, name, name_size,
		CASE_SENSITIVE, 0, NULL, 0, ctx)) {
		err = -EEXIST;
		goto err_out;
	}
	/* found attribute */
	a = (ATTR_RECORD*)ctx->attr;
	collation_rule = ((INDEX_ROOT*)((u8*)a +
		le16_to_cpu(a->value_offset)))->collation_rule;
	idx_header = (INDEX_HEADER*)((u8*)a + le16_to_cpu(a->value_offset)
		+ 0x10);
	idx_entry = (INDEX_ENTRY*)((u8*)idx_header +
		le32_to_cpu(idx_header->entries_offset));
	idx_end = (INDEX_ENTRY*)((u8*)idx_entry +
		le32_to_cpu(idx_header->index_length));
	
	/*
	 * Loop until we exceed valid memory (corruption case) or until we
	 * reach the last entry.
	 */
	if (type == AT_FILE_NAME) {
		while (((u8*)idx_entry < (u8*)idx_end) &&
			!(idx_entry->ie_flags & INDEX_ENTRY_END)) {
			/*
			i = ntfs_file_values_compare(&idx->key.file_name,
					&idx_entry->key.file_name, 1,
					IGNORE_CASE, opts->g_vol->upcase,
					opts->g_vol->upcase_len);
			*/
			i = ntfs_names_full_collate(idx->key.file_name.file_name, idx->key.file_name.file_name_length,
				idx_entry->key.file_name.file_name, idx_entry->key.file_name.file_name_length,
				IGNORE_CASE, opts->g_vol->upcase,
				opts->g_vol->upcase_len);
			/*
			 * If @file_name collates before ie->key.file_name,
			 * there is no matching index entry.
			 */
			if (i == -1)
				break;
			/* If file names are not equal, continue search. */
			if (i)
				goto do_next;
			if (idx->key.file_name.file_name_type !=
				FILE_NAME_POSIX ||
				idx_entry->key.file_name.file_name_type
				!= FILE_NAME_POSIX)
				return -EEXIST;
			/*
			i = ntfs_file_values_compare(&idx->key.file_name,
					&idx_entry->key.file_name, 1,
					CASE_SENSITIVE, opts->g_vol->upcase,
					opts->g_vol->upcase_len);
			*/
			i = ntfs_names_full_collate(idx->key.file_name.file_name, idx->key.file_name.file_name_length,
				idx_entry->key.file_name.file_name, idx_entry->key.file_name.file_name_length,
				CASE_SENSITIVE, opts->g_vol->upcase,
				opts->g_vol->upcase_len);
			if (!i)
				return -EEXIST;
			if (i == -1)
				break;
		do_next:
			idx_entry = (INDEX_ENTRY*)((u8*)idx_entry +
				le16_to_cpu(idx_entry->length));
		}
	}
	else if (type == AT_UNUSED) {  /* case view */
		while (((u8*)idx_entry < (u8*)idx_end) &&
			!(idx_entry->ie_flags & INDEX_ENTRY_END)) {
			i = ntfs_index_keys_compare((u8*)idx + 0x10,
				(u8*)idx_entry + 0x10,
				le16_to_cpu(idx->key_length),
				le16_to_cpu(idx_entry->key_length),
				collation_rule);
			if (!i)
				return -EEXIST;
			if (i == -1)
				break;
			idx_entry = (INDEX_ENTRY*)((u8*)idx_entry +
				le16_to_cpu(idx_entry->length));
		}
	}
	else
		return -EINVAL;
	memmove((u8*)idx_entry + idx_size, (u8*)idx_entry,
		le32_to_cpu(m->bytes_in_use) -
		((u8*)idx_entry - (u8*)m));
	memcpy((u8*)idx_entry, (u8*)idx, idx_size);
	/* Adjust various offsets, etc... */
	m->bytes_in_use = cpu_to_le32(le32_to_cpu(m->bytes_in_use) + idx_size);
	a->length = cpu_to_le32(le32_to_cpu(a->length) + idx_size);
	a->value_length = cpu_to_le32(le32_to_cpu(a->value_length) + idx_size);
	idx_header->index_length = cpu_to_le32(
		le32_to_cpu(idx_header->index_length) + idx_size);
	idx_header->allocated_size = cpu_to_le32(
		le32_to_cpu(idx_header->allocated_size) + idx_size);
err_out:
	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	return err;
}

/**
 * initialize_secure
 *
 * initializes $Secure's $SDH and $SII indexes from $SDS datastream
 */
static int initialize_secure(OPTS* opts, char* sds, u32 sds_size, MFT_RECORD* m)
{
	int err, sdh_size, sii_size;
	SECURITY_DESCRIPTOR_HEADER* sds_header;
	INDEX_ENTRY* idx_entry_sdh, * idx_entry_sii;
	SDH_INDEX_DATA* sdh_data;
	SII_INDEX_DATA* sii_data;

	sds_header = (SECURITY_DESCRIPTOR_HEADER*)sds;
	sdh_size = sizeof(INDEX_ENTRY_HEADER);
	sdh_size += sizeof(SDH_INDEX_KEY) + sizeof(SDH_INDEX_DATA);
	sii_size = sizeof(INDEX_ENTRY_HEADER);
	sii_size += sizeof(SII_INDEX_KEY) + sizeof(SII_INDEX_DATA);
	idx_entry_sdh = ntfs_calloc(sizeof(INDEX_ENTRY));
	if (!idx_entry_sdh)
		return -errno;
	idx_entry_sii = ntfs_calloc(sizeof(INDEX_ENTRY));
	if (!idx_entry_sii) {
		free(idx_entry_sdh);
		return -errno;
	}
	err = 0;

	while ((char*)sds_header < (char*)sds + sds_size) {
		if (!sds_header->length)
			break;
		/* SDH index entry */
		idx_entry_sdh->data_offset = const_cpu_to_le16(0x18);
		idx_entry_sdh->data_length = const_cpu_to_le16(0x14);
		idx_entry_sdh->reservedV = const_cpu_to_le32(0x00);
		idx_entry_sdh->length = const_cpu_to_le16(0x30);
		idx_entry_sdh->key_length = const_cpu_to_le16(0x08);
		idx_entry_sdh->ie_flags = const_cpu_to_le16(0x00);
		idx_entry_sdh->key.sdh.hash = sds_header->hash;
		idx_entry_sdh->key.sdh.security_id = sds_header->security_id;
		sdh_data = (SDH_INDEX_DATA*)((u8*)idx_entry_sdh +
			le16_to_cpu(idx_entry_sdh->data_offset));
		sdh_data->hash = sds_header->hash;
		sdh_data->security_id = sds_header->security_id;
		sdh_data->offset = sds_header->offset;
		sdh_data->length = sds_header->length;
		sdh_data->reserved_II = const_cpu_to_le32(0x00490049);

		/* SII index entry */
		idx_entry_sii->data_offset = const_cpu_to_le16(0x14);
		idx_entry_sii->data_length = const_cpu_to_le16(0x14);
		idx_entry_sii->reservedV = const_cpu_to_le32(0x00);
		idx_entry_sii->length = const_cpu_to_le16(0x28);
		idx_entry_sii->key_length = const_cpu_to_le16(0x04);
		idx_entry_sii->ie_flags = const_cpu_to_le16(0x00);
		idx_entry_sii->key.sii.security_id = sds_header->security_id;
		sii_data = (SII_INDEX_DATA*)((u8*)idx_entry_sii +
			le16_to_cpu(idx_entry_sii->data_offset));
		sii_data->hash = sds_header->hash;
		sii_data->security_id = sds_header->security_id;
		sii_data->offset = sds_header->offset;
		sii_data->length = sds_header->length;
		if ((err = insert_index_entry_in_res_dir_index(opts, idx_entry_sdh,
			sdh_size, m, NTFS_INDEX_SDH, 4, AT_UNUSED)))
			break;
		if ((err = insert_index_entry_in_res_dir_index(opts, idx_entry_sii,
			sii_size, m, NTFS_INDEX_SII, 4, AT_UNUSED)))
			break;
		sds_header = (SECURITY_DESCRIPTOR_HEADER*)((u8*)sds_header +
			((le32_to_cpu(sds_header->length) + 15) & ~15));
	}
	free(idx_entry_sdh);
	free(idx_entry_sii);
	return err;
}

/**
 * initialize_quota
 *
 * initialize $Quota with the default quota index-entries.
 */
static int initialize_quota(OPTS* opts, MFT_RECORD* m)
{
	int o_size, q1_size, q2_size, err, i;
	INDEX_ENTRY* idx_entry_o, * idx_entry_q1, * idx_entry_q2;
	QUOTA_O_INDEX_DATA* idx_entry_o_data;
	QUOTA_CONTROL_ENTRY* idx_entry_q1_data, * idx_entry_q2_data;

	err = 0;
	/* q index entry num 1 */
	q1_size = 0x48;
	idx_entry_q1 = ntfs_calloc(q1_size);
	if (!idx_entry_q1)
		return errno;
	idx_entry_q1->data_offset = const_cpu_to_le16(0x14);
	idx_entry_q1->data_length = const_cpu_to_le16(0x30);
	idx_entry_q1->reservedV = const_cpu_to_le32(0x00);
	idx_entry_q1->length = const_cpu_to_le16(0x48);
	idx_entry_q1->key_length = const_cpu_to_le16(0x04);
	idx_entry_q1->ie_flags = const_cpu_to_le16(0x00);
	idx_entry_q1->key.owner_id = const_cpu_to_le32(0x01);
	idx_entry_q1_data = (QUOTA_CONTROL_ENTRY*)((char*)idx_entry_q1
		+ le16_to_cpu(idx_entry_q1->data_offset));
	idx_entry_q1_data->version = const_cpu_to_le32(0x02);
	idx_entry_q1_data->flags = QUOTA_FLAG_DEFAULT_LIMITS;
	idx_entry_q1_data->bytes_used = const_cpu_to_le64(0x00);
	idx_entry_q1_data->change_time = mkntfs_time();
	idx_entry_q1_data->threshold = const_cpu_to_sle64(-1);
	idx_entry_q1_data->limit = const_cpu_to_sle64(-1);
	idx_entry_q1_data->exceeded_time = const_cpu_to_sle64(0);
	err = insert_index_entry_in_res_dir_index(opts, idx_entry_q1, q1_size, m,
		NTFS_INDEX_Q, 2, AT_UNUSED);
	free(idx_entry_q1);
	if (err)
		return err;
	/* q index entry num 2 */
	q2_size = 0x58;
	idx_entry_q2 = ntfs_calloc(q2_size);
	if (!idx_entry_q2)
		return errno;
	idx_entry_q2->data_offset = const_cpu_to_le16(0x14);
	idx_entry_q2->data_length = const_cpu_to_le16(0x40);
	idx_entry_q2->reservedV = const_cpu_to_le32(0x00);
	idx_entry_q2->length = const_cpu_to_le16(0x58);
	idx_entry_q2->key_length = const_cpu_to_le16(0x04);
	idx_entry_q2->ie_flags = const_cpu_to_le16(0x00);
	idx_entry_q2->key.owner_id = QUOTA_FIRST_USER_ID;
	idx_entry_q2_data = (QUOTA_CONTROL_ENTRY*)((char*)idx_entry_q2
		+ le16_to_cpu(idx_entry_q2->data_offset));
	idx_entry_q2_data->version = const_cpu_to_le32(0x02);
	idx_entry_q2_data->flags = QUOTA_FLAG_DEFAULT_LIMITS;
	idx_entry_q2_data->bytes_used = const_cpu_to_le64(0x00);
	idx_entry_q2_data->change_time = mkntfs_time();
	idx_entry_q2_data->threshold = const_cpu_to_sle64(-1);
	idx_entry_q2_data->limit = const_cpu_to_sle64(-1);
	idx_entry_q2_data->exceeded_time = const_cpu_to_sle64(0);
	idx_entry_q2_data->sid.revision = 1;
	idx_entry_q2_data->sid.sub_authority_count = 2;
	for (i = 0; i < 5; i++)
		idx_entry_q2_data->sid.identifier_authority.value[i] = 0;
	idx_entry_q2_data->sid.identifier_authority.value[5] = 0x05;
	idx_entry_q2_data->sid.sub_authority[0] =
		const_cpu_to_le32(SECURITY_BUILTIN_DOMAIN_RID);
	idx_entry_q2_data->sid.sub_authority[1] =
		const_cpu_to_le32(DOMAIN_ALIAS_RID_ADMINS);
	err = insert_index_entry_in_res_dir_index(opts, idx_entry_q2, q2_size, m,
		NTFS_INDEX_Q, 2, AT_UNUSED);
	free(idx_entry_q2);
	if (err)
		return err;
	o_size = 0x28;
	idx_entry_o = ntfs_calloc(o_size);
	if (!idx_entry_o)
		return errno;
	idx_entry_o->data_offset = const_cpu_to_le16(0x20);
	idx_entry_o->data_length = const_cpu_to_le16(0x04);
	idx_entry_o->reservedV = const_cpu_to_le32(0x00);
	idx_entry_o->length = const_cpu_to_le16(0x28);
	idx_entry_o->key_length = const_cpu_to_le16(0x10);
	idx_entry_o->ie_flags = const_cpu_to_le16(0x00);
	idx_entry_o->key.sid.revision = 0x01;
	idx_entry_o->key.sid.sub_authority_count = 0x02;
	for (i = 0; i < 5; i++)
		idx_entry_o->key.sid.identifier_authority.value[i] = 0;
	idx_entry_o->key.sid.identifier_authority.value[5] = 0x05;
	idx_entry_o->key.sid.sub_authority[0] =
		const_cpu_to_le32(SECURITY_BUILTIN_DOMAIN_RID);
	idx_entry_o->key.sid.sub_authority[1] =
		const_cpu_to_le32(DOMAIN_ALIAS_RID_ADMINS);
	idx_entry_o_data = (QUOTA_O_INDEX_DATA*)((char*)idx_entry_o
		+ le16_to_cpu(idx_entry_o->data_offset));
	idx_entry_o_data->owner_id = QUOTA_FIRST_USER_ID;
	/* 20 00 00 00 padding after here on ntfs 3.1. 3.0 is unchecked. */
	idx_entry_o_data->unknown = const_cpu_to_le32(32);
	err = insert_index_entry_in_res_dir_index(opts, idx_entry_o, o_size, m,
		NTFS_INDEX_O, 2, AT_UNUSED);
	free(idx_entry_o);

	return err;
}

/**
 * insert_file_link_in_dir_index
 *
 * Insert the fully completed FILE_NAME_ATTR @file_name which is inside
 * the file with mft reference @file_ref into the index (allocation) block
 * @idx (which belongs to @file_ref's parent directory).
 *
 * Return 0 on success or -errno on error.
 */
static int insert_file_link_in_dir_index(OPTS* opts, INDEX_BLOCK* idx, leMFT_REF file_ref,
	FILE_NAME_ATTR* file_name, u32 file_name_size)
{
	int err, i;
	INDEX_ENTRY* ie;
	char* index_end;

	/*
	 * Lookup dir entry @file_name in dir @idx to determine correct
	 * insertion location. FIXME: Using a very oversimplified lookup
	 * method which is sufficient for mkntfs but no good whatsoever in
	 * real world scenario. (AIA)
	 */

	index_end = (char*)&idx->index + le32_to_cpu(idx->index.index_length);
	ie = (INDEX_ENTRY*)((char*)&idx->index +
		le32_to_cpu(idx->index.entries_offset));
	/*
	 * Loop until we exceed valid memory (corruption case) or until we
	 * reach the last entry.
	 */
	while ((char*)ie < index_end && !(ie->ie_flags & INDEX_ENTRY_END)) {
		/*
		i = ntfs_file_values_compare(file_name,
				(FILE_NAME_ATTR*)&ie->key.file_name, 1,
				IGNORE_CASE, opts->g_vol->upcase, opts->g_vol->upcase_len);
		*/
		i = ntfs_names_full_collate(file_name->file_name, file_name->file_name_length,
			((FILE_NAME_ATTR*)&ie->key.file_name)->file_name, ((FILE_NAME_ATTR*)&ie->key.file_name)->file_name_length,
			IGNORE_CASE, opts->g_vol->upcase, opts->g_vol->upcase_len);
		/*
		 * If @file_name collates before ie->key.file_name, there is no
		 * matching index entry.
		 */
		if (i == -1)
			break;
		/* If file names are not equal, continue search. */
		if (i)
			goto do_next;
		/* File names are equal when compared ignoring case. */
		/*
		 * If BOTH file names are in the POSIX namespace, do a case
		 * sensitive comparison as well. Otherwise the names match so
		 * we return -EEXIST. FIXME: There are problems with this in a
		 * real world scenario, when one is POSIX and one isn't, but
		 * fine for mkntfs where we don't use POSIX namespace at all
		 * and hence this following code is luxury. (AIA)
		 */
		if (file_name->file_name_type != FILE_NAME_POSIX ||
			ie->key.file_name.file_name_type != FILE_NAME_POSIX)
			return -EEXIST;
		/*
		i = ntfs_file_values_compare(file_name,
				(FILE_NAME_ATTR*)&ie->key.file_name, 1,
				CASE_SENSITIVE, opts->g_vol->upcase,
				opts->g_vol->upcase_len);
		*/
		i = ntfs_names_full_collate(file_name->file_name, file_name->file_name_length,
			((FILE_NAME_ATTR*)&ie->key.file_name)->file_name, ((FILE_NAME_ATTR*)&ie->key.file_name)->file_name_length,
			CASE_SENSITIVE, opts->g_vol->upcase, opts->g_vol->upcase_len);
		if (i == -1)
			break;
		/* Complete match. Bugger. Can't insert. */
		if (!i)
			return -EEXIST;
	do_next:
		ie = (INDEX_ENTRY*)((char*)ie + le16_to_cpu(ie->length));
	};
	i = (sizeof(INDEX_ENTRY_HEADER) + file_name_size + 7) & ~7;
	err = make_room_for_index_entry_in_index_block(idx, ie, i);
	if (err) {
		ntfs_log_error("make_room_for_index_entry_in_index_block "
			"failed: %s\n", strerror(-err));
		return err;
	}
	/* Create entry in place and copy file name attribute value. */
	ie->indexed_file = file_ref;
	ie->length = cpu_to_le16(i);
	ie->key_length = cpu_to_le16(file_name_size);
	ie->ie_flags = const_cpu_to_le16(0);
	memcpy((char*)&ie->key.file_name, (char*)file_name, file_name_size);
	return 0;
}

/**
 * create_hardlink_res
 *
 * Create a file_name_attribute in the mft record @m_file which points to the
 * parent directory with mft reference @ref_parent.
 *
 * Then, insert an index entry with this file_name_attribute in the index
 * root @idx of the index_root attribute of the parent directory.
 *
 * @ref_file is the mft reference of @m_file.
 *
 * Return 0 on success or -errno on error.
 */
static int create_hardlink_res(OPTS* opts, MFT_RECORD* m_parent, const leMFT_REF ref_parent,
	MFT_RECORD* m_file, const leMFT_REF ref_file,
	const s64 allocated_size, const s64 data_size,
	const FILE_ATTR_FLAGS flags, const u16 packed_ea_size,
	const u32 reparse_point_tag, const char* file_name,
	const FILE_NAME_TYPE_FLAGS file_name_type)
{
	FILE_NAME_ATTR* fn;
	int i, fn_size, idx_size;
	INDEX_ENTRY* idx_entry_new;
	ntfschar* uname;

	/* Create the file_name attribute. */
	i = (strlen(file_name) + 1) * sizeof(ntfschar);
	fn_size = sizeof(FILE_NAME_ATTR) + i;
	fn = ntfs_malloc(fn_size);
	if (!fn)
		return -errno;
	fn->parent_directory = ref_parent;
	fn->creation_time = stdinfo_time(opts, m_file);
	fn->last_data_change_time = fn->creation_time;
	fn->last_mft_change_time = fn->creation_time;
	fn->last_access_time = fn->creation_time;
	fn->allocated_size = cpu_to_sle64(allocated_size);
	fn->data_size = cpu_to_sle64(data_size);
	fn->file_attributes = flags;
	/* These are in a union so can't have both. */
	if (packed_ea_size && reparse_point_tag) {
		free(fn);
		return -EINVAL;
	}
	if (packed_ea_size) {
		free(fn);
		return -EINVAL;
	}
	if (packed_ea_size) {
		fn->packed_ea_size = cpu_to_le16(packed_ea_size);
		fn->reserved = const_cpu_to_le16(0);
	}
	else {
		fn->reparse_point_tag = cpu_to_le32(reparse_point_tag);
	}
	fn->file_name_type = file_name_type;
	uname = fn->file_name;
	i = ntfs_mbstoucs_libntfscompat(file_name, &uname, i);
	if (i < 1) {
		free(fn);
		return -EINVAL;
	}
	if (i > 0xff) {
		free(fn);
		return -ENAMETOOLONG;
	}
	/* No terminating null in file names. */
	fn->file_name_length = i;
	fn_size = sizeof(FILE_NAME_ATTR) + i * sizeof(ntfschar);
	/* Increment the link count of @m_file. */
	i = le16_to_cpu(m_file->link_count);
	if (i == 0xffff) {
		ntfs_log_error("Too many hardlinks present already.\n");
		free(fn);
		return -EINVAL;
	}
	m_file->link_count = cpu_to_le16(i + 1);
	/* Add the file_name to @m_file. */
	i = insert_resident_attr_in_mft_record(opts, m_file, AT_FILE_NAME, NULL, 0,
		CASE_SENSITIVE, const_cpu_to_le16(0),
		RESIDENT_ATTR_IS_INDEXED, (u8*)fn, fn_size);
	if (i < 0) {
		ntfs_log_error("create_hardlink failed adding file name "
			"attribute: %s\n", strerror(-i));
		free(fn);
		/* Undo link count increment. */
		m_file->link_count = cpu_to_le16(
			le16_to_cpu(m_file->link_count) - 1);
		return i;
	}
	/* Insert the index entry for file_name in @idx. */
	idx_size = (fn_size + 7) & ~7;
	idx_entry_new = ntfs_calloc(idx_size + 0x10);
	if (!idx_entry_new)
		return -errno;
	idx_entry_new->indexed_file = ref_file;
	idx_entry_new->length = cpu_to_le16(idx_size + 0x10);
	idx_entry_new->key_length = cpu_to_le16(fn_size);
	memcpy((u8*)idx_entry_new + 0x10, (u8*)fn, fn_size);
	i = insert_index_entry_in_res_dir_index(opts, idx_entry_new, idx_size + 0x10,
		m_parent, NTFS_INDEX_I30, 4, AT_FILE_NAME);
	if (i < 0) {
		ntfs_log_error("create_hardlink failed inserting index entry: "
			"%s\n", strerror(-i));
		/* FIXME: Remove the file name attribute from @m_file. */
		free(idx_entry_new);
		free(fn);
		/* Undo link count increment. */
		m_file->link_count = cpu_to_le16(
			le16_to_cpu(m_file->link_count) - 1);
		return i;
	}
	free(idx_entry_new);
	free(fn);
	return 0;
}

/**
 * create_hardlink
 *
 * Create a file_name_attribute in the mft record @m_file which points to the
 * parent directory with mft reference @ref_parent.
 *
 * Then, insert an index entry with this file_name_attribute in the index
 * block @idx of the index allocation attribute of the parent directory.
 *
 * @ref_file is the mft reference of @m_file.
 *
 * Return 0 on success or -errno on error.
 */
static int create_hardlink(OPTS* opts, INDEX_BLOCK* idx, const leMFT_REF ref_parent,
	MFT_RECORD* m_file, const leMFT_REF ref_file,
	const s64 allocated_size, const s64 data_size,
	const FILE_ATTR_FLAGS flags, const u16 packed_ea_size,
	const u32 reparse_point_tag, const char* file_name,
	const FILE_NAME_TYPE_FLAGS file_name_type)
{
	FILE_NAME_ATTR* fn;
	int i, fn_size;
	ntfschar* uname;

	/* Create the file_name attribute. */
	i = (strlen(file_name) + 1) * sizeof(ntfschar);
	fn_size = sizeof(FILE_NAME_ATTR) + i;
	fn = ntfs_malloc(fn_size);
	if (!fn)
		return -errno;
	fn->parent_directory = ref_parent;
	fn->creation_time = stdinfo_time(opts, m_file);
	fn->last_data_change_time = fn->creation_time;
	fn->last_mft_change_time = fn->creation_time;
	fn->last_access_time = fn->creation_time;
	/* allocated size depends on unnamed data being resident */
	if (allocated_size && non_resident_unnamed_data(opts, m_file))
		fn->allocated_size = cpu_to_sle64(allocated_size);
	else
		fn->allocated_size = cpu_to_sle64((data_size + 7) & -8);
	fn->data_size = cpu_to_sle64(data_size);
	fn->file_attributes = flags;
	/* These are in a union so can't have both. */
	if (packed_ea_size && reparse_point_tag) {
		free(fn);
		return -EINVAL;
	}
	if (packed_ea_size) {
		fn->packed_ea_size = cpu_to_le16(packed_ea_size);
		fn->reserved = const_cpu_to_le16(0);
	}
	else {
		fn->reparse_point_tag = cpu_to_le32(reparse_point_tag);
	}
	fn->file_name_type = file_name_type;
	uname = fn->file_name;
	i = ntfs_mbstoucs_libntfscompat(file_name, &uname, i);
	if (i < 1) {
		free(fn);
		return -EINVAL;
	}
	if (i > 0xff) {
		free(fn);
		return -ENAMETOOLONG;
	}
	/* No terminating null in file names. */
	fn->file_name_length = i;
	fn_size = sizeof(FILE_NAME_ATTR) + i * sizeof(ntfschar);
	/* Increment the link count of @m_file. */
	i = le16_to_cpu(m_file->link_count);
	if (i == 0xffff) {
		ntfs_log_error("Too many hardlinks present already.\n");
		free(fn);
		return -EINVAL;
	}
	m_file->link_count = cpu_to_le16(i + 1);
	/* Add the file_name to @m_file. */
	i = insert_resident_attr_in_mft_record(opts, m_file, AT_FILE_NAME, NULL, 0,
		CASE_SENSITIVE, const_cpu_to_le16(0),
		RESIDENT_ATTR_IS_INDEXED, (u8*)fn, fn_size);
	if (i < 0) {
		ntfs_log_error("create_hardlink failed adding file name attribute: "
			"%s\n", strerror(-i));
		free(fn);
		/* Undo link count increment. */
		m_file->link_count = cpu_to_le16(
			le16_to_cpu(m_file->link_count) - 1);
		return i;
	}
	/* Insert the index entry for file_name in @idx. */
	i = insert_file_link_in_dir_index(opts, idx, ref_file, fn, fn_size);
	if (i < 0) {
		ntfs_log_error("create_hardlink failed inserting index entry: %s\n",
			strerror(-i));
		/* FIXME: Remove the file name attribute from @m_file. */
		free(fn);
		/* Undo link count increment. */
		m_file->link_count = cpu_to_le16(
			le16_to_cpu(m_file->link_count) - 1);
		return i;
	}
	free(fn);
	return 0;
}

/**
 * mkntfs_cleanup
 */
static void mkntfs_cleanup(OPTS* opts)
{
	struct BITMAP_ALLOCATION* p, * q;

	/* Close the volume */
	if (opts->g_vol) {
		if (opts->g_vol->dev) {
			if (NDevOpen(opts->g_vol->dev) && opts->g_vol->dev->d_ops->close(opts->g_vol->dev))
				ntfs_log_perror("Warning: Could not close %s", opts->g_vol->dev->d_name);
			ntfs_device_free(opts->g_vol->dev);
		}
		free(opts->g_vol->vol_name);
		free(opts->g_vol->attrdef);
		free(opts->g_vol->upcase);
		free(opts->g_vol);
		opts->g_vol = NULL;
	}

	/* Free any memory we've used */
	free(opts->g_bad_blocks);	opts->g_bad_blocks = NULL;
	free(opts->g_buf);			opts->g_buf = NULL;
	free(opts->g_index_block);	opts->g_index_block = NULL;
	free(opts->g_dynamic_buf);	opts->g_dynamic_buf = NULL;
	free(opts->g_upcaseinfo);	opts->g_upcaseinfo = NULL;
	free(opts->g_mft_bitmap);	opts->g_mft_bitmap = NULL;
	free(opts->g_rl_bad);		opts->g_rl_bad = NULL;
	free(opts->g_rl_boot);		opts->g_rl_boot = NULL;
	free(opts->g_rl_logfile);	opts->g_rl_logfile = NULL;
	free(opts->g_rl_mft);		opts->g_rl_mft = NULL;
	free(opts->g_rl_mft_bmp);	opts->g_rl_mft_bmp = NULL;
	free(opts->g_rl_mftmirr);	opts->g_rl_mftmirr = NULL;
	free(opts->g_logfile_buf);	opts->g_logfile_buf = NULL;

	p = opts->g_allocation;
	while (p) {
		q = p->next;
		free(p);
		p = q;
	}

	for (int i = 0; i < opts->egl3_file_count; ++i) {
		if (opts->egl3_files[i].is_directory) {
			// freed like g_index_block
			free(opts->egl3_files[i].reserved);	opts->egl3_files[i].reserved = NULL;
		}
	}
}

/**
 * mkntfs_open_partition -
 */
static BOOL mkntfs_open_partition(OPTS* opts, ntfs_volume* vol)
{
	BOOL result = FALSE;
	int i;
	struct stat sbuf;
	unsigned long mnt_flags;

	/*
	 * Allocate and initialize an ntfs device structure and attach it to
	 * the volume.
	 */
	vol->dev = ntfs_device_alloc("", 0, &ntfs_device_default_io_ops, NULL);
	if (!vol->dev) {
		ntfs_log_perror("Could not create device");
		goto done;
	}

	/* Open the device for reading or reading and writing. */
	if (OPT_NO_ACTION) {
		ntfs_log_quiet("Running in READ-ONLY mode!\n");
		i = O_RDONLY;
	}
	else {
		i = O_RDWR;
	}
	if (vol->dev->d_ops->open(vol->dev, i)) {
		if (errno == ENOENT)
			ntfs_log_error("The device doesn't exist; did you specify it correctly?\n");
		else
			ntfs_log_perror("Could not open %s", vol->dev->d_name);
		goto done;
	}

	result = TRUE;
done:
	return result;
}

/**
 * mkntfs_override_vol_params -
 */
static BOOL mkntfs_override_vol_params(OPTS* opts, ntfs_volume* vol)
{
	opts->num_sectors--; // reserve last 512 bytes/sector for the backup boot sector

	vol->cluster_size = 4096;
	vol->cluster_size_bits = ffs(vol->cluster_size) - 1;
	vol->nr_clusters = (opts->num_sectors * OPT_SECTOR_SIZE) / vol->cluster_size; // volume size / cluster size
	vol->mft_record_size = 1024;
	vol->mft_record_size_bits = ffs(vol->mft_record_size) - 1;
	vol->indx_record_size = 4096;
	vol->indx_record_size_bits = ffs(vol->indx_record_size) - 1;
	// This will never reach since we use MBR, and that has it's own limitations like this anyway
	/* Number of clusters must fit within 32 bits (Win2k limitation). */
	if (vol->nr_clusters >> 32) {
		ntfs_log_error("Number of clusters exceeds 32 bits.  Please "
			"try again with a larger\ncluster size or "
			"leave the cluster size unspecified and the "
			"smallest possible cluster size for the size "
			"of the device will be used.\n");
		return FALSE;
	}
	return TRUE;
}

/**
 * mkntfs_initialize_bitmaps -
 */
static BOOL mkntfs_initialize_bitmaps(OPTS* opts)
{
	u64 i;
	int mft_bitmap_size;

	/* Determine lcn bitmap byte size and allocate it. */
	opts->g_lcn_bitmap_byte_size = (opts->g_vol->nr_clusters + 7) >> 3;
	/* Needs to be multiple of 8 bytes. */
	opts->g_lcn_bitmap_byte_size = (opts->g_lcn_bitmap_byte_size + 7) & ~7;
	i = (opts->g_lcn_bitmap_byte_size + opts->g_vol->cluster_size - 1) &
		~(opts->g_vol->cluster_size - 1);
	ntfs_log_debug("g_lcn_bitmap_byte_size = %i, allocated = %llu\n",
		opts->g_lcn_bitmap_byte_size, (unsigned long long)i);
	opts->g_dynamic_buf_size = 4096; // default page size
	opts->g_dynamic_buf = (u8*)ntfs_calloc(opts->g_dynamic_buf_size);
	if (!opts->g_dynamic_buf)
		return FALSE;
	/*
	 * $Bitmap can overlap the end of the volume. Any bits in this region
	 * must be set. This region also encompasses the backup boot sector.
	 */
	if (!bitmap_allocate(opts, opts->g_vol->nr_clusters,
		((s64)opts->g_lcn_bitmap_byte_size << 3) - opts->g_vol->nr_clusters))
		return (FALSE);
	/*
	 * Mft size is 27 (NTFS 3.0+) mft records or one cluster, whichever is
	 * bigger.
	 */
	opts->g_mft_size = 27 + opts->egl3_file_count;
	opts->g_mft_size *= opts->g_vol->mft_record_size;
	if (opts->g_mft_size < (s32)opts->g_vol->cluster_size)
		opts->g_mft_size = opts->g_vol->cluster_size;
	ntfs_log_debug("MFT size = %i (0x%x) bytes\n", opts->g_mft_size, opts->g_mft_size);
	/* Determine mft bitmap size and allocate it. */
	mft_bitmap_size = opts->g_mft_size / opts->g_vol->mft_record_size;
	/* Convert to bytes, at least one. */
	opts->g_mft_bitmap_byte_size = (mft_bitmap_size + 7) >> 3;
	/* Mft bitmap is allocated in multiples of 8 bytes. */
	opts->g_mft_bitmap_byte_size = (opts->g_mft_bitmap_byte_size + 7) & ~7;
	ntfs_log_debug("mft_bitmap_size = %i, g_mft_bitmap_byte_size = %i\n",
		mft_bitmap_size, opts->g_mft_bitmap_byte_size);
	opts->g_mft_bitmap = ntfs_calloc(opts->g_mft_bitmap_byte_size);
	if (!opts->g_mft_bitmap)
		return FALSE;
	/* Create runlist for mft bitmap. */
	opts->g_rl_mft_bmp = ntfs_malloc(2 * sizeof(runlist));
	if (!opts->g_rl_mft_bmp)
		return FALSE;

	opts->g_rl_mft_bmp[0].vcn = 0LL;
	/* Mft bitmap is right after $Boot's data. */
	i = (8192 + opts->g_vol->cluster_size - 1) / opts->g_vol->cluster_size;
	opts->g_rl_mft_bmp[0].lcn = i;
	/*
	 * Size is always one cluster, even though valid data size and
	 * initialized data size are only 8 bytes.
	 */
	opts->g_rl_mft_bmp[1].vcn = 1LL;
	opts->g_rl_mft_bmp[0].length = 1LL;
	opts->g_rl_mft_bmp[1].lcn = -1LL;
	opts->g_rl_mft_bmp[1].length = 0LL;
	/* Allocate cluster for mft bitmap. */
	return (bitmap_allocate(opts, i, 1));
}

/**
 * mkntfs_initialize_rl_mft -
 */
static BOOL mkntfs_initialize_rl_mft(OPTS* opts)
{
	int j;
	BOOL done;

	/* If user didn't specify the mft lcn, determine it now. */
	if (!opts->g_mft_lcn) {
		/*
		 * We start at the higher value out of 16kiB and just after the
		 * mft bitmap.
		 */
		opts->g_mft_lcn = opts->g_rl_mft_bmp[0].lcn + opts->g_rl_mft_bmp[0].length;
		if (opts->g_mft_lcn * opts->g_vol->cluster_size < 16 * 1024)
			opts->g_mft_lcn = (16 * 1024 + opts->g_vol->cluster_size - 1) /
			opts->g_vol->cluster_size;
	}
	ntfs_log_debug("$MFT logical cluster number = 0x%llx\n", opts->g_mft_lcn);
	/* Determine MFT zone size. */
	opts->g_mft_zone_end = opts->g_vol->nr_clusters >> 3; // 1/8th of total volume size
	ntfs_log_debug("MFT zone size = %lldkiB\n", opts->g_mft_zone_end <<
		opts->g_vol->cluster_size_bits >> 10 /* >> 10 == / 1024 */);
	/*
	 * The mft zone begins with the mft data attribute, not at the beginning
	 * of the device.
	 */
	opts->g_mft_zone_end += opts->g_mft_lcn;
	/* Create runlist for mft. */
	opts->g_rl_mft = ntfs_malloc(2 * sizeof(runlist));
	if (!opts->g_rl_mft)
		return FALSE;

	opts->g_rl_mft[0].vcn = 0LL;
	opts->g_rl_mft[0].lcn = opts->g_mft_lcn;
	/* rounded up division by cluster size */
	j = (opts->g_mft_size + opts->g_vol->cluster_size - 1) / opts->g_vol->cluster_size;
	opts->g_rl_mft[1].vcn = j;
	opts->g_rl_mft[0].length = j;
	opts->g_rl_mft[1].lcn = -1LL;
	opts->g_rl_mft[1].length = 0LL;
	/* Allocate clusters for mft. */
	bitmap_allocate(opts, opts->g_mft_lcn, j);
	/* Determine mftmirr_lcn (middle of volume). */
	opts->g_mftmirr_lcn = (opts->num_sectors * OPT_SECTOR_SIZE >> 1)
		/ opts->g_vol->cluster_size;
	ntfs_log_debug("$MFTMirr logical cluster number = 0x%llx\n",
		opts->g_mftmirr_lcn);
	/* Create runlist for mft mirror. */
	opts->g_rl_mftmirr = ntfs_malloc(2 * sizeof(runlist));
	if (!opts->g_rl_mftmirr)
		return FALSE;

	opts->g_rl_mftmirr[0].vcn = 0LL;
	opts->g_rl_mftmirr[0].lcn = opts->g_mftmirr_lcn;
	/*
	 * The mft mirror is either 4kb (the first four records) or one cluster
	 * in size, which ever is bigger. In either case, it contains a
	 * byte-for-byte identical copy of the beginning of the mft (i.e. either
	 * the first four records (4kb) or the first cluster worth of records,
	 * whichever is bigger).
	 */
	j = (4 * opts->g_vol->mft_record_size + opts->g_vol->cluster_size - 1) / opts->g_vol->cluster_size;
	opts->g_rl_mftmirr[1].vcn = j;
	opts->g_rl_mftmirr[0].length = j;
	opts->g_rl_mftmirr[1].lcn = -1LL;
	opts->g_rl_mftmirr[1].length = 0LL;
	/* Allocate clusters for mft mirror. */
	done = bitmap_allocate(opts, opts->g_mftmirr_lcn, j);
	opts->g_logfile_lcn = opts->g_mftmirr_lcn + j;
	ntfs_log_debug("$LogFile logical cluster number = 0x%llx\n",
		opts->g_logfile_lcn);
	return (done);
}

/**
 * mkntfs_initialize_rl_logfile -
 */
static BOOL mkntfs_initialize_rl_logfile(OPTS* opts)
{
	int j;
	u64 volume_size;

	/* Create runlist for log file. */
	opts->g_rl_logfile = ntfs_malloc(2 * sizeof(runlist));
	if (!opts->g_rl_logfile)
		return FALSE;


	volume_size = opts->g_vol->nr_clusters << opts->g_vol->cluster_size_bits;

	opts->g_rl_logfile[0].vcn = 0LL;
	opts->g_rl_logfile[0].lcn = opts->g_logfile_lcn;
	/*
	 * Determine logfile_size from volume_size (rounded up to a cluster),
	 * making sure it does not overflow the end of the volume.
	 */
	if (volume_size < 2048LL * 1024)		/* < 2MiB	*/
		opts->g_logfile_size = 256LL * 1024;		/*   -> 256kiB	*/
	else if (volume_size < 4000000LL)		/* < 4MB	*/
		opts->g_logfile_size = 512LL * 1024;		/*   -> 512kiB	*/
	else if (volume_size <= 200LL * 1024 * 1024)	/* < 200MiB	*/
		opts->g_logfile_size = 2048LL * 1024;		/*   -> 2MiB	*/
	else {
		/*
		 * FIXME: The $LogFile size is 64 MiB upwards from 12GiB but
		 * the "200" divider below apparently approximates "100" or
		 * some other value as the volume size decreases. For example:
		 *      Volume size   LogFile size    Ratio
		 *	  8799808        46048       191.100
		 *	  8603248        45072       190.877
		 *	  7341704        38768       189.375
		 *	  6144828        32784       187.433
		 *	  4192932        23024       182.111
		 */
		if (volume_size >= 12LL << 30)		/* > 12GiB	*/
			opts->g_logfile_size = 64 << 20;	/*   -> 64MiB	*/
		else
			opts->g_logfile_size = (volume_size / 200) &
			~(opts->g_vol->cluster_size - 1);
	}
	j = opts->g_logfile_size / opts->g_vol->cluster_size;
	while (opts->g_rl_logfile[0].lcn + j >= opts->g_vol->nr_clusters) {
		/*
		 * $Logfile would overflow volume. Need to make it smaller than
		 * the standard size. It's ok as we are creating a non-standard
		 * volume anyway if it is that small.
		 */
		opts->g_logfile_size >>= 1;
		j = opts->g_logfile_size / opts->g_vol->cluster_size;
	}
	opts->g_logfile_size = (opts->g_logfile_size + opts->g_vol->cluster_size - 1) &
		~(opts->g_vol->cluster_size - 1);
	ntfs_log_debug("$LogFile (journal) size = %ikiB\n",
		opts->g_logfile_size / 1024);
	/*
	 * FIXME: The 256kiB limit is arbitrary. Should find out what the real
	 * minimum requirement for Windows is so it doesn't blue screen.
	 */
	if (opts->g_logfile_size < 256 << 10) {
		ntfs_log_error("$LogFile would be created with invalid size. "
			"This is not allowed as it would cause Windows "
			"to blue screen and during boot.\n");
		return FALSE;
	}
	opts->g_rl_logfile[1].vcn = j;
	opts->g_rl_logfile[0].length = j;
	opts->g_rl_logfile[1].lcn = -1LL;
	opts->g_rl_logfile[1].length = 0LL;
	/* Allocate clusters for log file. */
	return (bitmap_allocate(opts, opts->g_logfile_lcn, j));
}

/**
 * mkntfs_initialize_rl_boot -
 */
static BOOL mkntfs_initialize_rl_boot(OPTS* opts)
{
	int j;
	/* Create runlist for $Boot. */
	opts->g_rl_boot = ntfs_malloc(2 * sizeof(runlist));
	if (!opts->g_rl_boot)
		return FALSE;

	opts->g_rl_boot[0].vcn = 0LL;
	opts->g_rl_boot[0].lcn = 0LL;
	/*
	 * $Boot is always 8192 (0x2000) bytes or 1 cluster, whichever is
	 * bigger.
	 */
	j = (8192 + opts->g_vol->cluster_size - 1) / opts->g_vol->cluster_size;
	opts->g_rl_boot[1].vcn = j;
	opts->g_rl_boot[0].length = j;
	opts->g_rl_boot[1].lcn = -1LL;
	opts->g_rl_boot[1].length = 0LL;
	/* Allocate clusters for $Boot. */
	return (bitmap_allocate(opts, 0, j));
}

/**
 * mkntfs_initialize_rl_bad -
 */
static BOOL mkntfs_initialize_rl_bad(OPTS* opts)
{
	/* Create runlist for $BadClus, $DATA named stream $Bad. */
	opts->g_rl_bad = ntfs_malloc(2 * sizeof(runlist));
	if (!opts->g_rl_bad)
		return FALSE;

	opts->g_rl_bad[0].vcn = 0LL;
	opts->g_rl_bad[0].lcn = -1LL;
	/*
	 * $BadClus named stream $Bad contains the whole volume as a single
	 * sparse runlist entry.
	 */
	opts->g_rl_bad[1].vcn = opts->g_vol->nr_clusters;
	opts->g_rl_bad[0].length = opts->g_vol->nr_clusters;
	opts->g_rl_bad[1].lcn = -1LL;
	opts->g_rl_bad[1].length = 0LL;

	/* TODO: Mark bad blocks as such. */
	return TRUE;
}

/**
 * mkntfs_initialize_logfile_rec -
 */
static BOOL mkntfs_initialize_logfile_rec(OPTS* opts)
{
	/* Create first record for $LogFile so everything isn't all FF. */
	opts->g_logfile_buf = ntfs_calloc(DefaultLogPageSize);
	if (!opts->g_logfile_buf)
		return FALSE;

	// RESTART_PAGE_HEADER
	*((u32*)(opts->g_logfile_buf + 0)) = magic_RSTR; // magic
	*((le16*)(opts->g_logfile_buf + 4)) = 30; // usa_ofs
	*((le16*)(opts->g_logfile_buf + 6)) = 9; // usa_count
	*((leLSN*)(opts->g_logfile_buf + 8)) = 0; // chkdsk_lsn
	*((le32*)(opts->g_logfile_buf + 16)) = 4096; // system_page_size
	*((le32*)(opts->g_logfile_buf + 20)) = 4096; // log_page_size
	*((le16*)(opts->g_logfile_buf + 24)) = 48; // restart_area_offset
	*((sle16*)(opts->g_logfile_buf + 26)) = 0; // minor_ver
	*((sle16*)(opts->g_logfile_buf + 28)) = 2; // major_ver
	*((le16*)(opts->g_logfile_buf + 30)) = 3; // usn


	// RESTART_AREA
	*((leLSN*)(opts->g_logfile_buf + 48 + 0)) = 1065992; // current_lsn
	*((le16*)(opts->g_logfile_buf + 48 + 8)) = 1; // log_clients
	*((le16*)(opts->g_logfile_buf + 48 + 10)) = LOGFILE_NO_CLIENT; // client_free_list
	*((le16*)(opts->g_logfile_buf + 48 + 12)) = 0; // client_in_use_list
	*((le16*)(opts->g_logfile_buf + 48 + 14)) = 0; // flags
	*((le32*)(opts->g_logfile_buf + 48 + 16)) = 45; // seq_number_bits
	*((le16*)(opts->g_logfile_buf + 48 + 20)) = 224; // restart_area_length
	*((le16*)(opts->g_logfile_buf + 48 + 22)) = 64; // client_array_offset
	*((sle64*)(opts->g_logfile_buf + 48 + 24)) = 2682880; // file_size
	*((le32*)(opts->g_logfile_buf + 48 + 32)) = 112; // last_lsn_data_length
	*((le16*)(opts->g_logfile_buf + 48 + 36)) = 48; // log_record_header_length
	*((le16*)(opts->g_logfile_buf + 48 + 38)) = 64; // log_page_data_offset
	*((le32*)(opts->g_logfile_buf + 48 + 40)) = 371230280; // restart_log_open_count

	// LOG_CLIENT_RECORD
	*((leLSN*)(opts->g_logfile_buf + 48 + 64 + 0)) = 1048576; // oldest_lsn
	*((leLSN*)(opts->g_logfile_buf + 48 + 64 + 8)) = 1065992; // client_restart_lsn
	*((le16*)(opts->g_logfile_buf + 48 + 64 + 16)) = LOGFILE_NO_CLIENT; // prev_client
	*((le16*)(opts->g_logfile_buf + 48 + 64 + 18)) = LOGFILE_NO_CLIENT; // next_client
	*((le16*)(opts->g_logfile_buf + 48 + 64 + 20)) = 0; // seq_number
	*((le32*)(opts->g_logfile_buf + 48 + 64 + 28)) = 8; // client_name_length
	memcpy(opts->g_logfile_buf + 48 + 64 + 32, L"NTFS", 8); // client_name

	// usn patches
	*((le16*)(opts->g_logfile_buf + 0x1FE)) = 3;
	*((le16*)(opts->g_logfile_buf + 0x3FE)) = 3;
	*((le16*)(opts->g_logfile_buf + 0x5FE)) = 3;
	*((le16*)(opts->g_logfile_buf + 0x7FE)) = 3;
	*((le16*)(opts->g_logfile_buf + 0x9FE)) = 3;
	*((le16*)(opts->g_logfile_buf + 0xBFE)) = 3;
	*((le16*)(opts->g_logfile_buf + 0xDFE)) = 3;
	*((le16*)(opts->g_logfile_buf + 0xFFE)) = 3;

	return TRUE;
}

/**
 * mkntfs_sync_index_record
 *
 * (ERSO) made a function out of this, but the reason for doing that
 * disappeared during coding....
 */
static BOOL mkntfs_sync_index_record(OPTS* opts, INDEX_ALLOCATION* idx, MFT_RECORD* m,
	ntfschar* name, u32 name_len)
{
	int i, err;
	ntfs_attr_search_ctx* ctx;
	ATTR_RECORD* a;
	long long lw;
	runlist* rl_index = NULL;

	i = 5 * sizeof(ntfschar);
	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_perror("Failed to allocate attribute search context");
		return FALSE;
	}
	/* FIXME: This should be IGNORE_CASE! */
	if (mkntfs_attr_lookup(opts, AT_INDEX_ALLOCATION, name, name_len,
		CASE_SENSITIVE, 0, NULL, 0, ctx)) {
		ntfs_attr_put_search_ctx(ctx);
		ntfs_log_error("BUG: $INDEX_ALLOCATION attribute not found.\n");
		return FALSE;
	}
	a = ctx->attr;
	rl_index = ntfs_mapping_pairs_decompress(opts->g_vol, a, NULL);
	if (!rl_index) {
		ntfs_attr_put_search_ctx(ctx);
		ntfs_log_error("Failed to decompress runlist of $INDEX_ALLOCATION "
			"attribute.\n");
		return FALSE;
	}
	if (sle64_to_cpu(a->initialized_size) < i) {
		ntfs_attr_put_search_ctx(ctx);
		free(rl_index);
		ntfs_log_error("BUG: $INDEX_ALLOCATION attribute too short.\n");
		return FALSE;
	}
	ntfs_attr_put_search_ctx(ctx);
	i = sizeof(INDEX_BLOCK) - sizeof(INDEX_HEADER) +
		le32_to_cpu(idx->index.allocated_size);
	err = ntfs_mst_pre_write_fixup((NTFS_RECORD*)idx, i);
	if (err) {
		free(rl_index);
		ntfs_log_error("ntfs_mst_pre_write_fixup() failed while "
			"syncing index block.\n");
		return FALSE;
	}
	lw = ntfs_rlwrite(opts, opts->g_vol->dev, rl_index, (u8*)idx, i, NULL,
		WRITE_STANDARD);
	free(rl_index);
	if (lw != i) {
		ntfs_log_error("Error writing $INDEX_ALLOCATION.\n");
		return FALSE;
	}
	/* No more changes to @idx below here so no need for fixup: */
	/* ntfs_mst_post_write_fixup((NTFS_RECORD*)idx); */
	return TRUE;
}

/**
 * create_file_volume -
 */
static BOOL create_file_volume(OPTS* opts, MFT_RECORD* m, leMFT_REF root_ref,
	VOLUME_FLAGS fl, const GUID* volume_guid)
{
	int i, err;
	u8* sd;

	ntfs_log_verbose("Creating $Volume (mft record 3)\n");
	m = (MFT_RECORD*)(opts->g_buf + 3 * opts->g_vol->mft_record_size);
	err = create_hardlink(opts, opts->g_index_block, root_ref, m,
		MK_LE_MREF(FILE_Volume, FILE_Volume), 0LL, 0LL,
		FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM, 0, 0,
		"$Volume", FILE_NAME_WIN32_AND_DOS);
	if (!err) {
		init_system_file_sd(FILE_Volume, &sd, &i);
		err = add_attr_sd(opts, m, sd, i);
	}
	if (!err)
		err = add_attr_data(opts, m, NULL, 0, CASE_SENSITIVE,
			const_cpu_to_le16(0), NULL, 0);
	if (!err)
		err = add_attr_vol_name(opts, m, opts->g_vol->vol_name, opts->g_vol->vol_name ?
			strlen(opts->g_vol->vol_name) : 0);
	if (!err) {
		if (fl & VOLUME_IS_DIRTY)
			ntfs_log_quiet("Setting the volume dirty so check "
				"disk runs on next reboot into "
				"Windows.\n");
		err = add_attr_vol_info(opts, m, fl, opts->g_vol->major_ver,
			opts->g_vol->minor_ver);
	}
	if (err < 0) {
		ntfs_log_error("Couldn't create $Volume: %s\n",
			strerror(-err));
		return FALSE;
	}
	return TRUE;
}

/**
 * create_backup_boot_sector
 *
 * Return 0 on success or -1 if it couldn't be created.
 */
static int create_backup_boot_sector(OPTS* opts, u8* buff)
{
	const char* s;
	ssize_t bw;
	int size, e;

	ntfs_log_verbose("Creating backup boot sector.\n");
	/*
	 * Write the first max(512, opts.sector_size) bytes from buf to the
	 * last sector, but limit that to 8192 bytes of written data since that
	 * is how big $Boot is (and how big our buffer is)..
	 */
	size = 512;
	if (size < OPT_SECTOR_SIZE)
		size = OPT_SECTOR_SIZE;
	if (opts->g_vol->dev->d_ops->seek(opts->g_vol->dev, (opts->num_sectors + 1) *
		OPT_SECTOR_SIZE - size, SEEK_SET) == (off_t)-1) {
		ntfs_log_perror("Seek failed");
		goto bb_err;
	}
	if (size > 8192)
		size = 8192;
	bw = mkntfs_write(opts, opts->g_vol->dev, buff, size);
	if (bw == size)
		return 0;
	e = errno;
	if (bw == -1LL)
		s = strerror(e);
	else
		s = "unknown error";
	/* At least some 2.4 kernels return EIO instead of ENOSPC. */
	if (bw != -1LL || (bw == -1LL && e != ENOSPC && e != EIO)) {
		ntfs_log_critical("Couldn't write backup boot sector: %s\n", s);
		return -1;
	}
bb_err:
	return -1;
}

/**
 * mkntfs_create_root_structures -
 */
static BOOL mkntfs_create_root_structures(OPTS* opts)
{
	NTFS_BOOT_SECTOR* bs;
	MFT_RECORD* m;
	leMFT_REF root_ref;
	leMFT_REF extend_ref;
	int i;
	int j;
	int err;
	u8* sd;
	FILE_ATTR_FLAGS extend_flags;
	VOLUME_FLAGS volume_flags = const_cpu_to_le16(0);
	int sectors_per_cluster;
	int nr_sysfiles;
	int nr_allfiles;
	int buf_sds_first_size;
	char* buf_sds;
	GUID vol_guid;

	ntfs_log_quiet("Creating NTFS volume structures.\n");
	nr_sysfiles = 27;
	nr_allfiles = nr_sysfiles + opts->egl3_file_count;
	/*
	 * Setup an empty mft record.  Note, we can just give 0 as the mft
	 * reference as we are creating an NTFS 1.2 volume for which the mft
	 * reference is ignored by ntfs_mft_record_layout().
	 *
	 * Copy the mft record onto all 16 records in the buffer and setup the
	 * sequence numbers of each system file to equal the mft record number
	 * of that file (only for $MFT is the sequence number 1 rather than 0).
	 */
	for (i = 0; i < nr_allfiles; i++) {
		if (ntfs_mft_record_layout(opts->g_vol, 0, m = (MFT_RECORD*)(opts->g_buf +
			i * opts->g_vol->mft_record_size))) {
			ntfs_log_error("Failed to layout system mft records."
				"\n");
			return FALSE;
		}
		if (i == 0 || i > 23)
			m->sequence_number = const_cpu_to_le16(1);
		else
			m->sequence_number = cpu_to_le16(i);
	}
	/*
	 * If only one cluster contains all system files then
	 * fill the rest of it with empty, formatted records.
	 */
	if (nr_allfiles * (s32)opts->g_vol->mft_record_size < opts->g_mft_size) {
		for (i = nr_allfiles;
			i * (s32)opts->g_vol->mft_record_size < opts->g_mft_size; i++) {
			m = (MFT_RECORD*)(opts->g_buf + i * opts->g_vol->mft_record_size);
			if (ntfs_mft_record_layout(opts->g_vol, 0, m)) {
				ntfs_log_error("Failed to layout mft record."
					"\n");
				return FALSE;
			}
			m->flags = const_cpu_to_le16(0);
			m->sequence_number = cpu_to_le16(i);
		}
	}
	/*
	 * Create the 16 system files, adding the system information attribute
	 * to each as well as marking them in use in the mft bitmap.
	 */
	for (i = 0; i < nr_sysfiles; i++) {
		le32 file_attrs;

		m = (MFT_RECORD*)(opts->g_buf + i * opts->g_vol->mft_record_size);
		if (i < 16 || i > 23) {
			m->mft_record_number = cpu_to_le32(i);
			m->flags |= MFT_RECORD_IN_USE;
			ntfs_bit_set(opts->g_mft_bitmap, 0LL + i, 1);
		}
		file_attrs = FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM;
		if (i == FILE_root) {
			file_attrs |= FILE_ATTR_ARCHIVE;
		}
		/* setting specific security_id flag and */
		/* file permissions for ntfs 3.x */
		if (i == 0 || i == 1 || i == 2 || i == 6 || i == 8 ||
			i == 10) {
			add_attr_std_info(opts, m, file_attrs,
				const_cpu_to_le32(0x0100));
		}
		else if (i == 9) {
			file_attrs |= FILE_ATTR_VIEW_INDEX_PRESENT;
			add_attr_std_info(opts, m, file_attrs,
				const_cpu_to_le32(0x0101));
		}
		else if (i == 11) {
			add_attr_std_info(opts, m, file_attrs,
				const_cpu_to_le32(0x0101));
		}
		else if (i == 24 || i == 25 || i == 26) {
			file_attrs |= FILE_ATTR_ARCHIVE;
			file_attrs |= FILE_ATTR_VIEW_INDEX_PRESENT;
			add_attr_std_info(opts, m, file_attrs,
				const_cpu_to_le32(0x0101));
		}
		else {
			add_attr_std_info(opts, m, file_attrs,
				const_cpu_to_le32(0x00));
		}
	}
	for (i = nr_sysfiles; i < nr_allfiles; i++) {
		m = (MFT_RECORD*)(opts->g_buf + i * opts->g_vol->mft_record_size);
		m->mft_record_number = cpu_to_le32(i);
		m->flags |= MFT_RECORD_IN_USE;
		ntfs_bit_set(opts->g_mft_bitmap, 0LL + i, 1);
		add_attr_std_info(opts, m, FILE_ATTR_ARCHIVE,
			const_cpu_to_le32(0));
	}
	/* The root directory mft reference. */
	root_ref = MK_LE_MREF(FILE_root, FILE_root);
	extend_ref = MK_LE_MREF(11, 11);
	ntfs_log_verbose("Creating root directory (mft record 5)\n");
	m = (MFT_RECORD*)(opts->g_buf + 5 * opts->g_vol->mft_record_size);
	m->flags |= MFT_RECORD_IS_DIRECTORY;
	m->link_count = cpu_to_le16(le16_to_cpu(m->link_count) + 1);
	err = add_attr_file_name(opts, m, root_ref, 0LL, 0LL,
		FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM |
		FILE_ATTR_I30_INDEX_PRESENT, 0, 0, ".",
		FILE_NAME_WIN32_AND_DOS);
	if (!err) {
		init_root_sd(&sd, &i);
		err = add_attr_sd(opts, m, sd, i);
	}
	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = add_attr_index_root(opts, m, "$I30", 4, CASE_SENSITIVE,
			AT_FILE_NAME, COLLATION_FILE_NAME,
			opts->g_vol->indx_record_size);
	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = upgrade_to_large_index(opts, m, "$I30", 4, CASE_SENSITIVE,
			&opts->g_index_block);
	if (!err) {
		ntfs_attr_search_ctx* ctx;
		ATTR_RECORD* a;
		ctx = ntfs_attr_get_search_ctx(NULL, m);
		if (!ctx) {
			ntfs_log_perror("Failed to allocate attribute search "
				"context");
			return FALSE;
		}
		/* There is exactly one file name so this is ok. */
		if (mkntfs_attr_lookup(opts, AT_FILE_NAME, AT_UNNAMED, 0,
			CASE_SENSITIVE, 0, NULL, 0, ctx)) {
			ntfs_attr_put_search_ctx(ctx);
			ntfs_log_error("BUG: $FILE_NAME attribute not found."
				"\n");
			return FALSE;
		}
		a = ctx->attr;
		err = insert_file_link_in_dir_index(opts, opts->g_index_block, root_ref,
			(FILE_NAME_ATTR*)((char*)a +
				le16_to_cpu(a->value_offset)),
			le32_to_cpu(a->value_length));
		ntfs_attr_put_search_ctx(ctx);
	}
	if (err) {
		ntfs_log_error("Couldn't create root directory: %s\n",
			strerror(-err));
		return FALSE;
	}
	/* Add all other attributes, on a per-file basis for clarity. */
	ntfs_log_verbose("Creating $MFT (mft record 0)\n");
	m = (MFT_RECORD*)opts->g_buf;
	err = add_attr_data_positioned(opts, m, NULL, 0, CASE_SENSITIVE,
		const_cpu_to_le16(0), opts->g_rl_mft, opts->g_buf, opts->g_mft_size);
	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(FILE_MFT, 1),
			((opts->g_mft_size - 1)
				| (opts->g_vol->cluster_size - 1)) + 1,
			opts->g_mft_size, FILE_ATTR_HIDDEN |
			FILE_ATTR_SYSTEM, 0, 0, "$MFT",
			FILE_NAME_WIN32_AND_DOS);
	/* mft_bitmap is not modified in mkntfs; no need to sync it later. */
	if (!err)
		err = add_attr_bitmap_positioned(opts, m, NULL, 0, CASE_SENSITIVE,
			opts->g_rl_mft_bmp,
			opts->g_mft_bitmap, opts->g_mft_bitmap_byte_size);
	if (err < 0) {
		ntfs_log_error("Couldn't create $MFT: %s\n", strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $MFTMirr (mft record 1)\n");
	m = (MFT_RECORD*)(opts->g_buf + 1 * opts->g_vol->mft_record_size);
	err = add_attr_data_positioned(opts, m, NULL, 0, CASE_SENSITIVE,
		const_cpu_to_le16(0), opts->g_rl_mftmirr, opts->g_buf,
		opts->g_rl_mftmirr[0].length * opts->g_vol->cluster_size);
	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(FILE_MFTMirr, FILE_MFTMirr),
			opts->g_rl_mftmirr[0].length * opts->g_vol->cluster_size,
			opts->g_rl_mftmirr[0].length * opts->g_vol->cluster_size,
			FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM, 0, 0,
			"$MFTMirr", FILE_NAME_WIN32_AND_DOS);
	if (err < 0) {
		ntfs_log_error("Couldn't create $MFTMirr: %s\n",
			strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $LogFile (mft record 2)\n");
	m = (MFT_RECORD*)(opts->g_buf + 2 * opts->g_vol->mft_record_size);
	err = add_attr_data_positioned(opts, m, NULL, 0, CASE_SENSITIVE,
		const_cpu_to_le16(0), opts->g_rl_logfile,
		(const u8*)NULL, opts->g_logfile_size);
	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(FILE_LogFile, FILE_LogFile),
			opts->g_logfile_size, opts->g_logfile_size,
			FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM, 0, 0,
			"$LogFile", FILE_NAME_WIN32_AND_DOS);
	if (err < 0) {
		ntfs_log_error("Couldn't create $LogFile: %s\n",
			strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $AttrDef (mft record 4)\n");
	m = (MFT_RECORD*)(opts->g_buf + 4 * opts->g_vol->mft_record_size);
	err = add_attr_data(opts, m, NULL, 0, CASE_SENSITIVE, const_cpu_to_le16(0),
		(u8*)opts->g_vol->attrdef, opts->g_vol->attrdef_len);
	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(FILE_AttrDef, FILE_AttrDef),
			(opts->g_vol->attrdef_len + opts->g_vol->cluster_size - 1) &
			~(opts->g_vol->cluster_size - 1), opts->g_vol->attrdef_len,
			FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM, 0, 0,
			"$AttrDef", FILE_NAME_WIN32_AND_DOS);
	if (!err) {
		init_system_file_sd(FILE_AttrDef, &sd, &i);
		err = add_attr_sd(opts, m, sd, i);
	}
	if (err < 0) {
		ntfs_log_error("Couldn't create $AttrDef: %s\n",
			strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $Bitmap (mft record 6)\n");
	m = (MFT_RECORD*)(opts->g_buf + 6 * opts->g_vol->mft_record_size);
	/* the data attribute of $Bitmap must be non-resident or otherwise */
	/* windows 2003 will regard the volume as corrupt (ERSO) */
	if (!err)
		err = insert_non_resident_attr_in_mft_record(opts, m,
			AT_DATA, NULL, 0, CASE_SENSITIVE,
			const_cpu_to_le16(0), (const u8*)NULL,
			opts->g_lcn_bitmap_byte_size, WRITE_BITMAP);


	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(FILE_Bitmap, FILE_Bitmap),
			(opts->g_lcn_bitmap_byte_size + opts->g_vol->cluster_size -
				1) & ~(opts->g_vol->cluster_size - 1),
			opts->g_lcn_bitmap_byte_size,
			FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM, 0, 0,
			"$Bitmap", FILE_NAME_WIN32_AND_DOS);
	if (err < 0) {
		ntfs_log_error("Couldn't create $Bitmap: %s\n", strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $Boot (mft record 7)\n");
	m = (MFT_RECORD*)(opts->g_buf + 7 * opts->g_vol->mft_record_size);
	bs = ntfs_calloc(8192);
	if (!bs)
		return FALSE;
	memcpy(bs, boot_array, sizeof(boot_array));
	/*
	 * Create the boot sector in bs. Note, that bs is already zeroed
	 * in the boot sector section and that it has the NTFS OEM id/magic
	 * already inserted, so no need to worry about these things.
	 */
	bs->bpb.bytes_per_sector = cpu_to_le16(OPT_SECTOR_SIZE);
	sectors_per_cluster = opts->g_vol->cluster_size / OPT_SECTOR_SIZE;
	if (sectors_per_cluster > 128)
		bs->bpb.sectors_per_cluster = 257 - ffs(sectors_per_cluster);
	else
		bs->bpb.sectors_per_cluster = sectors_per_cluster;
	bs->bpb.media_type = 0xf8; /* hard disk */
	bs->bpb.sectors_per_track = 0;
	bs->bpb.heads = 0;
	bs->bpb.hidden_sectors = cpu_to_le32(opts->mbr_hidden_sectors);
	bs->physical_drive = 0x80;  	    /* boot from hard disk */
	bs->extended_boot_signature = 0x80; /* everybody sets this, so we do */
	bs->number_of_sectors = cpu_to_sle64(opts->num_sectors);
	bs->mft_lcn = cpu_to_sle64(opts->g_mft_lcn);
	bs->mftmirr_lcn = cpu_to_sle64(opts->g_mftmirr_lcn);
	if (opts->g_vol->mft_record_size >= opts->g_vol->cluster_size) {
		bs->clusters_per_mft_record = opts->g_vol->mft_record_size /
			opts->g_vol->cluster_size;
	}
	else {
		bs->clusters_per_mft_record = -(ffs(opts->g_vol->mft_record_size) -
			1);
		if ((u32)(1 << -bs->clusters_per_mft_record) !=
			opts->g_vol->mft_record_size) {
			free(bs);
			ntfs_log_error("BUG: calculated clusters_per_mft_record"
				" is wrong (= 0x%x)\n",
				bs->clusters_per_mft_record);
			return FALSE;
		}
	}
	ntfs_log_debug("clusters per mft record = %i (0x%x)\n",
		bs->clusters_per_mft_record,
		bs->clusters_per_mft_record);
	if (opts->g_vol->indx_record_size >= opts->g_vol->cluster_size) {
		bs->clusters_per_index_record = opts->g_vol->indx_record_size /
			opts->g_vol->cluster_size;
	}
	else {
		bs->clusters_per_index_record = -opts->g_vol->indx_record_size_bits;
		if ((1 << -bs->clusters_per_index_record) !=
			(s32)opts->g_vol->indx_record_size) {
			free(bs);
			ntfs_log_error("BUG: calculated "
				"clusters_per_index_record is wrong "
				"(= 0x%x)\n",
				bs->clusters_per_index_record);
			return FALSE;
		}
	}
	ntfs_log_debug("clusters per index block = %i (0x%x)\n",
		bs->clusters_per_index_record,
		bs->clusters_per_index_record);
	/* Generate a 64-bit random number for the serial number. */
	bs->volume_serial_number = cpu_to_le64(((u64)rand() << 32) |
		((u64)rand() & 0xffffffff));
	/*
	 * Leave zero for now as NT4 leaves it zero, too. If want it later, see
	 * ../libntfs/bootsect.c for how to calculate it.
	 */
	bs->checksum = const_cpu_to_le32(0);
	/* Make sure the bootsector is ok. */
	if (!ntfs_boot_sector_is_ntfs(bs)) {
		free(bs);
		ntfs_log_error("FATAL: Generated boot sector is invalid!\n");
		return FALSE;
	}
	err = add_attr_data_positioned(opts, m, NULL, 0, CASE_SENSITIVE,
		const_cpu_to_le16(0), opts->g_rl_boot, (u8*)bs, 8192);
	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(FILE_Boot, FILE_Boot),
			(8192 + opts->g_vol->cluster_size - 1) &
			~(opts->g_vol->cluster_size - 1), 8192,
			FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM, 0, 0,
			"$Boot", FILE_NAME_WIN32_AND_DOS);
	if (!err) {
		init_system_file_sd(FILE_Boot, &sd, &i);
		err = add_attr_sd(opts, m, sd, i);
	}
	if (err < 0) {
		free(bs);
		ntfs_log_error("Couldn't create $Boot: %s\n", strerror(-err));
		return FALSE;
	}
	if (create_backup_boot_sector(opts, (u8*)bs)) {
		/*
		 * Pre-2.6 kernels couldn't access the last sector if it was
		 * odd and we failed to set the device block size to the sector
		 * size, hence we schedule chkdsk to create it.
		 */
		volume_flags |= VOLUME_IS_DIRTY;
	}
	free(bs);
	/* Generate a GUID for the volume. */
	ntfs_generate_guid(&vol_guid);
	if (!create_file_volume(opts, m, root_ref, volume_flags, &vol_guid))
		return FALSE;
	ntfs_log_verbose("Creating $BadClus (mft record 8)\n");
	m = (MFT_RECORD*)(opts->g_buf + 8 * opts->g_vol->mft_record_size);
	/* FIXME: This should be IGNORE_CASE */
	/* Create a sparse named stream of size equal to the volume size. */
	err = add_attr_data_positioned(opts, m, "$Bad", 4, CASE_SENSITIVE,
		const_cpu_to_le16(0), opts->g_rl_bad, NULL,
		opts->g_vol->nr_clusters * opts->g_vol->cluster_size);
	if (!err) {
		err = add_attr_data(opts, m, NULL, 0, CASE_SENSITIVE,
			const_cpu_to_le16(0), NULL, 0);
	}
	if (!err) {
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(FILE_BadClus, FILE_BadClus),
			0LL, 0LL, FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM,
			0, 0, "$BadClus", FILE_NAME_WIN32_AND_DOS);
	}
	if (err < 0) {
		ntfs_log_error("Couldn't create $BadClus: %s\n",
			strerror(-err));
		return FALSE;
	}
	/* create $Secure (NTFS 3.0+) */
	ntfs_log_verbose("Creating $Secure (mft record 9)\n");
	m = (MFT_RECORD*)(opts->g_buf + 9 * opts->g_vol->mft_record_size);
	m->flags |= MFT_RECORD_IS_VIEW_INDEX;
	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(9, 9), 0LL, 0LL,
			FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM |
			FILE_ATTR_VIEW_INDEX_PRESENT, 0, 0,
			"$Secure", FILE_NAME_WIN32_AND_DOS);
	buf_sds = NULL;
	buf_sds_first_size = 0;
	if (!err) {
		int buf_sds_size;

		buf_sds_first_size = 0xfc;
		buf_sds_size = 0x40000 + buf_sds_first_size;
		buf_sds = ntfs_calloc(buf_sds_size);
		if (!buf_sds)
			return FALSE;
		init_secure_sds(buf_sds);
		memcpy(buf_sds + 0x40000, buf_sds, buf_sds_first_size);
		err = add_attr_data(opts, m, "$SDS", 4, CASE_SENSITIVE,
			const_cpu_to_le16(0), (u8*)buf_sds,
			buf_sds_size);
	}
	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = add_attr_index_root(opts, m, "$SDH", 4, CASE_SENSITIVE,
			AT_UNUSED, COLLATION_NTOFS_SECURITY_HASH,
			opts->g_vol->indx_record_size);
	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = add_attr_index_root(opts, m, "$SII", 4, CASE_SENSITIVE,
			AT_UNUSED, COLLATION_NTOFS_ULONG,
			opts->g_vol->indx_record_size);
	if (!err)
		err = initialize_secure(opts, buf_sds, buf_sds_first_size, m);
	free(buf_sds);
	if (err < 0) {
		ntfs_log_error("Couldn't create $Secure: %s\n",
			strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $UpCase (mft record 0xa)\n");
	m = (MFT_RECORD*)(opts->g_buf + 0xa * opts->g_vol->mft_record_size);
	err = add_attr_data(opts, m, NULL, 0, CASE_SENSITIVE, const_cpu_to_le16(0),
		(u8*)opts->g_vol->upcase, opts->g_vol->upcase_len << 1);
	/*
	 * The $Info only exists since Windows 8, but it apparently
	 * does not disturb chkdsk from earlier versions.
	 */
	if (!err)
		err = add_attr_data(opts, m, "$Info", 5, CASE_SENSITIVE,
			const_cpu_to_le16(0),
			(u8*)opts->g_upcaseinfo, sizeof(struct UPCASEINFO));
	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(FILE_UpCase, FILE_UpCase),
			((opts->g_vol->upcase_len << 1) +
				opts->g_vol->cluster_size - 1) &
			~(opts->g_vol->cluster_size - 1),
			opts->g_vol->upcase_len << 1,
			FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM, 0, 0,
			"$UpCase", FILE_NAME_WIN32_AND_DOS);
	if (err < 0) {
		ntfs_log_error("Couldn't create $UpCase: %s\n", strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $Extend (mft record 11)\n");
	/*
	 * $Extend index must be resident.  Otherwise, w2k3 will regard the
	 * volume as corrupt. (ERSO)
	 */
	m = (MFT_RECORD*)(opts->g_buf + 11 * opts->g_vol->mft_record_size);
	m->flags |= MFT_RECORD_IS_DIRECTORY;
	if (!err)
		err = create_hardlink(opts, opts->g_index_block, root_ref, m,
			MK_LE_MREF(11, 11), 0LL, 0LL,
			FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM |
			FILE_ATTR_I30_INDEX_PRESENT, 0, 0,
			"$Extend", FILE_NAME_WIN32_AND_DOS);
	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = add_attr_index_root(opts, m, "$I30", 4, CASE_SENSITIVE,
			AT_FILE_NAME, COLLATION_FILE_NAME,
			opts->g_vol->indx_record_size);
	if (err < 0) {
		ntfs_log_error("Couldn't create $Extend: %s\n",
			strerror(-err));
		return FALSE;
	}
	/* NTFS reserved system files (mft records 0xc-0xf) */
	for (i = 0xc; i < 0x10; i++) {
		ntfs_log_verbose("Creating system file (mft record 0x%x)\n", i);
		m = (MFT_RECORD*)(opts->g_buf + i * opts->g_vol->mft_record_size);
		err = add_attr_data(opts, m, NULL, 0, CASE_SENSITIVE,
			const_cpu_to_le16(0), NULL, 0);
		if (!err) {
			init_system_file_sd(i, &sd, &j);
			err = add_attr_sd(opts, m, sd, j);
		}
		if (err < 0) {
			ntfs_log_error("Couldn't create system file %i (0x%x): "
				"%s\n", i, i, strerror(-err));
			return FALSE;
		}
	}
	/* create systemfiles for ntfs volumes (3.1) */
	/* starting with file 24 (ignoring file 16-23) */
	extend_flags = FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM |
		FILE_ATTR_ARCHIVE | FILE_ATTR_VIEW_INDEX_PRESENT;
	ntfs_log_verbose("Creating $Quota (mft record 24)\n");
	m = (MFT_RECORD*)(opts->g_buf + 24 * opts->g_vol->mft_record_size);
	m->flags |= MFT_RECORD_IS_4;
	m->flags |= MFT_RECORD_IS_VIEW_INDEX;
	if (!err)
		err = create_hardlink_res(opts, (MFT_RECORD*)(opts->g_buf +
			11 * opts->g_vol->mft_record_size), extend_ref, m,
			MK_LE_MREF(24, 1), 0LL, 0LL, extend_flags,
			0, 0, "$Quota", FILE_NAME_WIN32_AND_DOS);
	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = add_attr_index_root(opts, m, "$Q", 2, CASE_SENSITIVE, AT_UNUSED,
			COLLATION_NTOFS_ULONG, opts->g_vol->indx_record_size);
	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = add_attr_index_root(opts, m, "$O", 2, CASE_SENSITIVE, AT_UNUSED,
			COLLATION_NTOFS_SID, opts->g_vol->indx_record_size);
	if (!err)
		err = initialize_quota(opts, m);
	if (err < 0) {
		ntfs_log_error("Couldn't create $Quota: %s\n", strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $ObjId (mft record 25)\n");
	m = (MFT_RECORD*)(opts->g_buf + 25 * opts->g_vol->mft_record_size);
	m->flags |= MFT_RECORD_IS_4;
	m->flags |= MFT_RECORD_IS_VIEW_INDEX;
	if (!err)
		err = create_hardlink_res(opts, (MFT_RECORD*)(opts->g_buf +
			11 * opts->g_vol->mft_record_size), extend_ref,
			m, MK_LE_MREF(25, 1), 0LL, 0LL,
			extend_flags, 0, 0, "$ObjId",
			FILE_NAME_WIN32_AND_DOS);

	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = add_attr_index_root(opts, m, "$O", 2, CASE_SENSITIVE, AT_UNUSED,
			COLLATION_NTOFS_ULONGS,
			opts->g_vol->indx_record_size);
	if (err < 0) {
		ntfs_log_error("Couldn't create $ObjId: %s\n",
			strerror(-err));
		return FALSE;
	}
	ntfs_log_verbose("Creating $Reparse (mft record 26)\n");
	m = (MFT_RECORD*)(opts->g_buf + 26 * opts->g_vol->mft_record_size);
	m->flags |= MFT_RECORD_IS_4;
	m->flags |= MFT_RECORD_IS_VIEW_INDEX;
	if (!err)
		err = create_hardlink_res(opts, (MFT_RECORD*)(opts->g_buf +
			11 * opts->g_vol->mft_record_size),
			extend_ref, m, MK_LE_MREF(26, 1),
			0LL, 0LL, extend_flags, 0, 0,
			"$Reparse", FILE_NAME_WIN32_AND_DOS);
	/* FIXME: This should be IGNORE_CASE */
	if (!err)
		err = add_attr_index_root(opts, m, "$R", 2, CASE_SENSITIVE, AT_UNUSED,
			COLLATION_NTOFS_ULONGS, opts->g_vol->indx_record_size);
	if (err < 0) {
		ntfs_log_error("Couldn't create $Reparse: %s\n",
			strerror(-err));
		return FALSE;
	}

	const u8* ro_sd;
	const s64* ro_sd_len;
	if (!err)
		err = create_sd_sddl("O:WDG:SYD:PAI(A;;0x1200a9;;;WD)", &ro_sd, &ro_sd_len);

	ntfs_log_verbose("Creating EGL3 files\n");

	for (int i = 0; i < opts->egl3_file_count; ++i) {
		EGL3File* File = opts->egl3_files + i;
		s64 MFTrecnum = i + 27;
		ntfs_log_verbose("Creating %s (mft record %d)\n", File->name, MFTrecnum);
		m = (MFT_RECORD*)(opts->g_buf + MFTrecnum * opts->g_vol->mft_record_size);
		if (File->is_directory) {
			m->flags |= MFT_RECORD_IS_DIRECTORY;
			if (File->parent_index == -1) {
				if (!err)
					err = create_hardlink(opts, opts->g_index_block, root_ref, m,
						MK_LE_MREF(MFTrecnum, 1),
						0, 0,
						FILE_ATTR_I30_INDEX_PRESENT, 0, 0,
						File->name, FILE_NAME_POSIX);
			}
			else {
				EGL3File* ParentFile = opts->egl3_files + File->parent_index;
				if (!ParentFile->reserved) {
					if (!err)
						err = upgrade_to_large_index(opts, (MFT_RECORD*)(opts->g_buf + (File->parent_index + 27) * opts->g_vol->mft_record_size), "$I30", 4, CASE_SENSITIVE, &ParentFile->reserved);
				}
				if (!err)
					err = create_hardlink(opts, ParentFile->reserved, MK_LE_MREF(File->parent_index + 27, 1), m,
						MK_LE_MREF(MFTrecnum, 1),
						0, 0,
						FILE_ATTR_I30_INDEX_PRESENT, 0, 0,
						File->name, FILE_NAME_POSIX);
			}
			if (!err)
				err = add_attr_index_root(opts, m, "$I30", 4, CASE_SENSITIVE,
					AT_FILE_NAME, COLLATION_FILE_NAME,
					opts->g_vol->indx_record_size);
		}
		else {
			if (!err) {
				err = insert_non_resident_attr_in_mft_record_nowrite(opts, m, AT_DATA, NULL,
					0, CASE_SENSITIVE, const_cpu_to_le16(0), File->size, &File->o_runlist);
			}
			if (File->parent_index == -1) {
				if (!err)
					err = create_hardlink(opts, opts->g_index_block, root_ref,
						m,
						MK_LE_MREF(MFTrecnum, 1),
						((File->size) + opts->g_vol->cluster_size - 1) & ~(opts->g_vol->cluster_size - 1),
						File->size, FILE_ATTR_ARCHIVE, 0, 0,
						File->name, FILE_NAME_POSIX);
			}
			else {
				EGL3File* ParentFile = opts->egl3_files + File->parent_index;
				if (!ParentFile->reserved) {
					if (!err)
						err = upgrade_to_large_index(opts, (MFT_RECORD*)(opts->g_buf + (File->parent_index + 27) * opts->g_vol->mft_record_size), "$I30", 4, CASE_SENSITIVE, &ParentFile->reserved);
				}
				if (!err)
					err = create_hardlink(opts, ParentFile->reserved, MK_LE_MREF(File->parent_index + 27, 1), m,
						MK_LE_MREF(MFTrecnum, 1),
						((File->size) + opts->g_vol->cluster_size - 1) & ~(opts->g_vol->cluster_size - 1),
						File->size, FILE_ATTR_ARCHIVE, 0, 0,
						File->name, FILE_NAME_POSIX);
			}
		}
		if (!err)
			err = add_attr_sd(opts, m, ro_sd, ro_sd_len);
		if (err < 0) {
			ntfs_log_error("Couldn't create %s: %s\n", File->name,
				strerror(-err));
			return FALSE;
		}
	}

	ntfs_log_verbose("Syncing EGL3 folder index records\n");
	for (int i = 0; i < opts->egl3_file_count; ++i) {
		EGL3File* File = opts->egl3_files + i;
		s64 MFTrecnum = i + 27;
		if (!File->is_directory || !File->reserved)
			continue;
		ntfs_log_verbose("Syncing %s index record (mft record %X)\n", File->name, MFTrecnum);
		m = (MFT_RECORD*)(opts->g_buf + MFTrecnum * opts->g_vol->mft_record_size);

		if (!err)
			err = mkntfs_sync_index_record(opts, File->reserved, m, L"$I30", 4);

		if (err < 0) {
			ntfs_log_error("Couldn't sync %s: %s\n", File->name,
				strerror(-err));
			return FALSE;
		}
	}
	
	return TRUE;
}

/**
 * mkntfs_redirect
 */
static int mkntfs_redirect(OPTS* opts)
{
	u64 upcase_crc;
	int result = 1;
	ntfs_attr_search_ctx* ctx = NULL;
	long long lw, pos;
	ATTR_RECORD* a;
	MFT_RECORD* m;
	int i, err;

	if (!opts) {
		ntfs_log_error("Internal error: invalid parameters to mkntfs_options.\n");
		goto done;
	}
	/* Initialize the random number generator with the current time. */
	srand(sle64_to_cpu(mkntfs_time()) / 10000000);
	/* Allocate and initialize ntfs_volume structure opts->g_vol. */
	opts->g_vol = ntfs_volume_alloc();
	if (!opts->g_vol) {
		ntfs_log_perror("Could not create volume");
		goto done;
	}
	/* Create NTFS 3.1 (Windows XP/Vista) volumes. */
	opts->g_vol->major_ver = 3;
	opts->g_vol->minor_ver = 1;
	/* Transfer some options to the volume. */
	if (opts->label) {
		opts->g_vol->vol_name = strdup(opts->label);
		if (!opts->g_vol->vol_name) {
			ntfs_log_perror("Could not copy volume name");
			goto done;
		}
	}
	if (OPT_CLUSTER_SIZE >= 0)
		opts->g_vol->cluster_size = OPT_CLUSTER_SIZE;
	/* Length is in unicode characters. */
	opts->g_vol->upcase_len = ntfs_upcase_build_default(&opts->g_vol->upcase);
	/* Since Windows 8, there is a $Info stream in $UpCase */
	opts->g_upcaseinfo =
		(struct UPCASEINFO*)ntfs_malloc(sizeof(struct UPCASEINFO));
	if (!opts->g_vol->upcase_len || !opts->g_upcaseinfo)
		goto done;
	/* If the CRC is correct, chkdsk does not warn about obsolete table */
	crc64(0, (byte*)NULL, 0); /* initialize the crc computation */
	upcase_crc = crc64(0, (byte*)opts->g_vol->upcase,
		opts->g_vol->upcase_len * sizeof(ntfschar));
	/* keep the version fields as zero */
	memset(opts->g_upcaseinfo, 0, sizeof(struct UPCASEINFO));
	opts->g_upcaseinfo->len = const_cpu_to_le32(sizeof(struct UPCASEINFO));
	opts->g_upcaseinfo->crc = cpu_to_le64(upcase_crc);
	opts->g_vol->attrdef = ntfs_malloc(sizeof(attrdef_ntfs3x_array));
	if (!opts->g_vol->attrdef) {
		ntfs_log_perror("Could not create attrdef structure");
		goto done;
	}
	memcpy(opts->g_vol->attrdef, attrdef_ntfs3x_array,
		sizeof(attrdef_ntfs3x_array));
	opts->g_vol->attrdef_len = sizeof(attrdef_ntfs3x_array);
	/* Open the partition. */
	if (!mkntfs_open_partition(opts, opts->g_vol))
		goto done;
	/*
	 * Decide on the sector size, cluster size, mft record and index record
	 * sizes as well as the number of sectors/tracks/heads/size, etc.
	 */
	if (!mkntfs_override_vol_params(opts, opts->g_vol))
		goto done;
	/* Initialize $Bitmap and $MFT/$BITMAP related stuff. */
	if (!mkntfs_initialize_bitmaps(opts))
		goto done;
	/* Initialize MFT & set opts->g_logfile_lcn. */
	if (!mkntfs_initialize_rl_mft(opts))
		goto done;
	/* Initialize $LogFile. */
	if (!mkntfs_initialize_rl_logfile(opts))
		goto done;
	/* Initialize $Boot. */
	if (!mkntfs_initialize_rl_boot(opts))
		goto done;
	/* Allocate a buffer large enough to hold the mft. */
	opts->g_buf = ntfs_calloc(opts->g_mft_size);
	if (!opts->g_buf)
		goto done;
	/* Create runlist for $BadClus, $DATA named stream $Bad. */
	if (!mkntfs_initialize_rl_bad(opts))
		goto done;
	/* Create a simple $LogFile record at the beginning so Windows can read it */
	if (!mkntfs_initialize_logfile_rec(opts))
		goto done;

	// this is where we start writing data
	/* Create NTFS volume structures. */
	if (!mkntfs_create_root_structures(opts))
		goto done;
	/*
	 * - Do not step onto bad blocks!!!
	 * - If any bad blocks were specified or found, modify $BadClus,
	 *   allocating the bad clusters in $Bitmap.
	 * - C&w bootsector backup bootsector (backup in last sector of the
	 *   partition).
	 * - If NTFS 3.0+, c&w $Secure file and $Extend directory with the
	 *   corresponding special files in it, i.e. $ObjId, $Quota, $Reparse,
	 *   and $UsnJrnl. And others? Or not all necessary?
	 * - RE: Populate $root with the system files (and $Extend directory if
	 *   applicable). Possibly should move this as far to the top as
	 *   possible and update during each subsequent c&w of each system file.
	 */
	ntfs_log_verbose("Syncing root directory index record.\n");
	if (!mkntfs_sync_index_record(opts, opts->g_index_block, (MFT_RECORD*)(opts->g_buf + 5 *
		opts->g_vol->mft_record_size), NTFS_INDEX_I30, 4))
		goto done;

	ntfs_log_verbose("Syncing $Bitmap.\n");
	m = (MFT_RECORD*)(opts->g_buf + 6 * opts->g_vol->mft_record_size);

	ctx = ntfs_attr_get_search_ctx(NULL, m);
	if (!ctx) {
		ntfs_log_perror("Could not create an attribute search context");
		goto done;
	}

	if (mkntfs_attr_lookup(opts, AT_DATA, AT_UNNAMED, 0, CASE_SENSITIVE,
		0, NULL, 0, ctx)) {
		ntfs_log_error("BUG: $DATA attribute not found.\n");
		goto done;
	}

	a = ctx->attr;
	if (a->non_resident) {
		runlist* rl = ntfs_mapping_pairs_decompress(opts->g_vol, a, NULL);
		if (!rl) {
			ntfs_log_error("ntfs_mapping_pairs_decompress() failed\n");
			goto done;
		}
		lw = ntfs_rlwrite(opts, opts->g_vol->dev, rl, (const u8*)NULL,
			opts->g_lcn_bitmap_byte_size, NULL, WRITE_BITMAP);
		err = errno;
		free(rl);
		if (lw != opts->g_lcn_bitmap_byte_size) {
			ntfs_log_error("ntfs_rlwrite: %s\n", lw == -1 ?
				strerror(err) : "unknown error");
			goto done;
		}
	}
	else {
		/* Error : the bitmap must be created non resident */
		ntfs_log_error("Error : the global bitmap is resident\n");
		goto done;
	}

	/*
	 * No need to sync $MFT/$BITMAP as that has never been modified since
	 * its creation.
	 */
	ntfs_log_verbose("Syncing $MFT.\n");
	pos = opts->g_mft_lcn * opts->g_vol->cluster_size;
	lw = 1;
	for (i = 0; i < opts->g_mft_size / (s32)opts->g_vol->mft_record_size; i++) {
		if (!OPT_NO_ACTION)
			lw = ntfs_mst_pwrite(opts->g_vol->dev, pos, 1, opts->g_vol->mft_record_size, opts->g_buf + i * opts->g_vol->mft_record_size);
		if (lw != 1) {
			ntfs_log_error("ntfs_mst_pwrite: %s\n", lw == -1 ?
				strerror(errno) : "unknown error");
			goto done;
		}
		pos += opts->g_vol->mft_record_size;
	}
	ntfs_log_verbose("Updating $MFTMirr.\n");
	pos = opts->g_mftmirr_lcn * opts->g_vol->cluster_size;
	lw = 1;
	for (i = 0; i < opts->g_rl_mftmirr[0].length * opts->g_vol->cluster_size / opts->g_vol->mft_record_size; i++) {
		m = (MFT_RECORD*)(opts->g_buf + i * opts->g_vol->mft_record_size);
		/*
		 * Decrement the usn by one, so it becomes the same as the one
		 * in $MFT once it is mst protected. - This is as we need the
		 * $MFTMirr to have the exact same byte by byte content as
		 * $MFT, rather than just equivalent meaning content.
		 */
		if (ntfs_mft_usn_dec(m)) {
			ntfs_log_error("ntfs_mft_usn_dec");
			goto done;
		}
		if (!OPT_NO_ACTION)
			lw = ntfs_mst_pwrite(opts->g_vol->dev, pos, 1, opts->g_vol->mft_record_size, opts->g_buf + i * opts->g_vol->mft_record_size);
		if (lw != 1) {
			ntfs_log_error("ntfs_mst_pwrite: %s\n", lw == -1 ?
				strerror(errno) : "unknown error");
			goto done;
		}
		pos += opts->g_vol->mft_record_size;
	}

	ntfs_log_verbose("Creating the base $LogFile record.\n");
	pos = opts->g_logfile_lcn * opts->g_vol->cluster_size;
	lw = 1;
	if (!OPT_NO_ACTION)
		lw = ntfs_pwrite(opts->g_vol->dev, pos, DefaultLogPageSize, opts->g_logfile_buf);
	if (lw != DefaultLogPageSize) {
		ntfs_log_error("ntfs_mst_pwrite: %s\n", lw == -1 ?
			strerror(errno) : "unknown error");
		goto done;
	}

	ntfs_log_verbose("Syncing device.\n");
	if (opts->g_vol->dev->d_ops->sync(opts->g_vol->dev)) {
		ntfs_log_error("Syncing device. FAILED");
		goto done;
	}
	ntfs_log_quiet("mkntfs completed successfully. Have a nice day.\n");
	result = 0;
done:
	ntfs_attr_put_search_ctx(ctx);
	opts->g_vol->dev->d_ops->ioctl(opts->g_vol->dev, 0x444F4641, &opts->o_data);
	mkntfs_cleanup(opts);	/* Device is unlocked and closed here */
	return result;
}

int EGL3CreateDisk(u64 sector_size, const char* label, const EGL3File files[], u32 file_count, void** o_data)
{
	ntfs_log_set_handler(ntfs_log_handler_outerr);
	utils_set_locale();

	OPTS opts = { 0 };
	opts.num_sectors = sector_size;
	opts.egl3_files = files;
	opts.egl3_file_count = file_count;
	opts.label = label;

	int result = mkntfs_redirect(&opts);
	*o_data = opts.o_data;
	return result;
}