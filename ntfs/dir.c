/**
 * dir.c - Directory handling code. Originated from the Linux-NTFS project.
 *
 * Copyright (c) 2002-2005 Anton Altaparmakov
 * Copyright (c) 2004-2005 Richard Russon
 * Copyright (c) 2004-2008 Szabolcs Szakacsits
 * Copyright (c) 2005-2007 Yura Pakhuchiy
 * Copyright (c) 2008-2020 Jean-Pierre Andre
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <string.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#endif
#ifdef MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif

#include "param.h"
#include "types.h"
#include "debug.h"
#include "attrib.h"
#include "inode.h"
#include "dir.h"
#include "volume.h"
#include "mft.h"
#include "index.h"
#include "ntfstime.h"
#include "lcnalloc.h"
#include "logging.h"
#include "cache.h"
#include "misc.h"
#include "security.h"
#include "object_id.h"
#include "xattrs.h"

#define S_ISREG(n) 0
#define S_ISDIR(n) 0

/*
 * The little endian Unicode strings "$I30", "$SII", "$SDH", "$O"
 *  and "$Q" as global constants.
 */
ntfschar NTFS_INDEX_I30[5] = { const_cpu_to_le16('$'), const_cpu_to_le16('I'),
		const_cpu_to_le16('3'), const_cpu_to_le16('0'),
		const_cpu_to_le16('\0') };
ntfschar NTFS_INDEX_SII[5] = { const_cpu_to_le16('$'), const_cpu_to_le16('S'),
		const_cpu_to_le16('I'), const_cpu_to_le16('I'),
		const_cpu_to_le16('\0') };
ntfschar NTFS_INDEX_SDH[5] = { const_cpu_to_le16('$'), const_cpu_to_le16('S'),
		const_cpu_to_le16('D'), const_cpu_to_le16('H'),
		const_cpu_to_le16('\0') };
ntfschar NTFS_INDEX_O[3] = { const_cpu_to_le16('$'), const_cpu_to_le16('O'),
		const_cpu_to_le16('\0') };
ntfschar NTFS_INDEX_Q[3] = { const_cpu_to_le16('$'), const_cpu_to_le16('Q'),
		const_cpu_to_le16('\0') };
ntfschar NTFS_INDEX_R[3] = { const_cpu_to_le16('$'), const_cpu_to_le16('R'),
		const_cpu_to_le16('\0') };

#if CACHE_INODE_SIZE

/*
 *		Pathname hashing
 *
 *	Based on first char and second char (which may be '\0')
 */

int ntfs_dir_inode_hash(const struct CACHED_GENERIC *cached)
{
	const char *path;
	const unsigned char *name;

	path = (const char*)cached->variable;
	if (!path) {
		ntfs_log_error("Bad inode cache entry\n");
		return (-1);
	}
	name = (const unsigned char*)strrchr(path,'/');
	if (!name)
		name = (const unsigned char*)path;
	return (((name[0] << 1) + name[1] + strlen((const char*)name))
				% (2*CACHE_INODE_SIZE));
}

/*
 *		Pathname comparing for entering/fetching from cache
 */

static int inode_cache_compare(const struct CACHED_GENERIC *cached,
			const struct CACHED_GENERIC *wanted)
{
	return (!cached->variable
		    || strcmp(cached->variable, wanted->variable));
}

/*
 *		Pathname comparing for invalidating entries in cache
 *
 *	A partial path is compared in order to invalidate all paths
 *	related to a renamed directory
 *	inode numbers are also checked, as deleting a long name may
 *	imply deleting a short name and conversely
 *
 *	Only use associated with a CACHE_NOHASH flag
 */

static int inode_cache_inv_compare(const struct CACHED_GENERIC *cached,
			const struct CACHED_GENERIC *wanted)
{
	int len;
	BOOL different;
	const struct CACHED_INODE *w;
	const struct CACHED_INODE *c;

	w = (const struct CACHED_INODE*)wanted;
	c = (const struct CACHED_INODE*)cached;
	if (w->pathname) {
		len = strlen(w->pathname);
		different = !cached->variable
			|| ((w->inum != MREF(c->inum))
			   && (strncmp(c->pathname, w->pathname, len)
				|| ((c->pathname[len] != '\0')
				   && (c->pathname[len] != '/'))));
	} else
		different = !c->pathname
			|| (w->inum != MREF(c->inum));
	return (different);
}

#endif

#if CACHE_LOOKUP_SIZE

/*
 *		File name comparing for entering/fetching from lookup cache
 */

static int lookup_cache_compare(const struct CACHED_GENERIC *cached,
			const struct CACHED_GENERIC *wanted)
{
	const struct CACHED_LOOKUP *c = (const struct CACHED_LOOKUP*) cached;
	const struct CACHED_LOOKUP *w = (const struct CACHED_LOOKUP*) wanted;
	return (!c->name
		    || (c->parent != w->parent)
		    || (c->namesize != w->namesize)
		    || memcmp(c->name, w->name, c->namesize));
}

/*
 *		Inode number comparing for invalidating lookup cache
 *
 *	All entries with designated inode number are invalidated
 *
 *	Only use associated with a CACHE_NOHASH flag
 */

static int lookup_cache_inv_compare(const struct CACHED_GENERIC *cached,
			const struct CACHED_GENERIC *wanted)
{
	const struct CACHED_LOOKUP *c = (const struct CACHED_LOOKUP*) cached;
	const struct CACHED_LOOKUP *w = (const struct CACHED_LOOKUP*) wanted;
	return (!c->name
		    || (c->parent != w->parent)
		    || (MREF(c->inum) != MREF(w->inum)));
}

/*
 *		Lookup hashing
 *
 *	Based on first, second and and last char
 */

int ntfs_dir_lookup_hash(const struct CACHED_GENERIC *cached)
{
	const unsigned char *name;
	int count;
	unsigned int val;

	name = (const unsigned char*)cached->variable;
	count = cached->varsize;
	if (!name || !count) {
		ntfs_log_error("Bad lookup cache entry\n");
		return (-1);
	}
	val = (name[0] << 2) + (name[1] << 1) + name[count - 1] + count;
	return (val % (2*CACHE_LOOKUP_SIZE));
}

#endif

/**
 * ntfs_inode_lookup_by_name - find an inode in a directory given its name
 * @dir_ni:	ntfs inode of the directory in which to search for the name
 * @uname:	Unicode name for which to search in the directory
 * @uname_len:	length of the name @uname in Unicode characters
 *
 * Look for an inode with name @uname in the directory with inode @dir_ni.
 * ntfs_inode_lookup_by_name() walks the contents of the directory looking for
 * the Unicode name. If the name is found in the directory, the corresponding
 * inode number (>= 0) is returned as a mft reference in cpu format, i.e. it
 * is a 64-bit number containing the sequence number.
 *
 * On error, return -1 with errno set to the error code. If the inode is is not
 * found errno is ENOENT.
 *
 * Note, @uname_len does not include the (optional) terminating NULL character.
 *
 * Note, we look for a case sensitive match first but we also look for a case
 * insensitive match at the same time. If we find a case insensitive match, we
 * save that for the case that we don't find an exact match, where we return
 * the mft reference of the case insensitive match.
 *
 * If the volume is mounted with the case sensitive flag set, then we only
 * allow exact matches.
 */
u64 ntfs_inode_lookup_by_name(ntfs_inode *dir_ni,
		const ntfschar *uname, const int uname_len)
{
	VCN vcn;
	u64 mref = 0;
	s64 br;
	ntfs_volume *vol = dir_ni->vol;
	ntfs_attr_search_ctx *ctx;
	INDEX_ROOT *ir;
	INDEX_ENTRY *ie;
	INDEX_ALLOCATION *ia;
	IGNORE_CASE_BOOL case_sensitivity;
	u8 *index_end;
	ntfs_attr *ia_na;
	int eo, rc;
	u32 index_block_size;
	u8 index_vcn_size_bits;

	ntfs_log_trace("Entering\n");

	if (!dir_ni || !dir_ni->mrec || !uname || uname_len <= 0) {
		errno = EINVAL;
		return -1;
	}

	ctx = ntfs_attr_get_search_ctx(dir_ni, NULL);
	if (!ctx)
		return -1;

	/* Find the index root attribute in the mft record. */
	if (ntfs_attr_lookup(AT_INDEX_ROOT, NTFS_INDEX_I30, 4, CASE_SENSITIVE, 0, NULL,
			0, ctx)) {
		ntfs_log_perror("Index root attribute missing in directory inode "
				"%lld", (unsigned long long)dir_ni->mft_no);
		goto put_err_out;
	}
	case_sensitivity = (NVolCaseSensitive(vol) ? CASE_SENSITIVE : IGNORE_CASE);
	/* Get to the index root value. */
	ir = (INDEX_ROOT*)((u8*)ctx->attr +
			le16_to_cpu(ctx->attr->value_offset));
	index_block_size = le32_to_cpu(ir->index_block_size);
	if (index_block_size < NTFS_BLOCK_SIZE ||
			index_block_size & (index_block_size - 1)) {
		ntfs_log_error("Index block size %u is invalid.\n",
				(unsigned)index_block_size);
		goto put_err_out;
	}
	index_end = (u8*)&ir->index + le32_to_cpu(ir->index.index_length);
	/* The first index entry. */
	ie = (INDEX_ENTRY*)((u8*)&ir->index +
			le32_to_cpu(ir->index.entries_offset));
	/*
	 * Loop until we exceed valid memory (corruption case) or until we
	 * reach the last entry.
	 */
	for (;; ie = (INDEX_ENTRY*)((u8*)ie + le16_to_cpu(ie->length))) {
		/* Bounds checks. */
		if ((u8*)ie < (u8*)ctx->mrec || (u8*)ie +
				sizeof(INDEX_ENTRY_HEADER) > index_end ||
				(u8*)ie + le16_to_cpu(ie->key_length) >
				index_end) {
			ntfs_log_error("Index entry out of bounds in inode %lld"
				       "\n", (unsigned long long)dir_ni->mft_no);
			goto put_err_out;
		}
		/*
		 * The last entry cannot contain a name. It can however contain
		 * a pointer to a child node in the B+tree so we just break out.
		 */
		if (ie->ie_flags & INDEX_ENTRY_END)
			break;
		
		if (!le16_to_cpu(ie->length)) {
			ntfs_log_error("Zero length index entry in inode %lld"
				       "\n", (unsigned long long)dir_ni->mft_no);
			goto put_err_out;
		}
		/*
		 * Not a perfect match, need to do full blown collation so we
		 * know which way in the B+tree we have to go.
		 */
		rc = ntfs_names_full_collate(uname, uname_len,
				(ntfschar*)&ie->key.file_name.file_name,
				ie->key.file_name.file_name_length,
				case_sensitivity, vol->upcase, vol->upcase_len);
		/*
		 * If uname collates before the name of the current entry, there
		 * is definitely no such name in this index but we might need to
		 * descend into the B+tree so we just break out of the loop.
		 */
		if (rc == -1)
			break;
		/* The names are not equal, continue the search. */
		if (rc)
			continue;
		/*
		 * Perfect match, this will never happen as the
		 * ntfs_are_names_equal() call will have gotten a match but we
		 * still treat it correctly.
		 */
		mref = le64_to_cpu(ie->indexed_file);
		ntfs_attr_put_search_ctx(ctx);
		return mref;
	}
	/*
	 * We have finished with this index without success. Check for the
	 * presence of a child node and if not present return error code
	 * ENOENT, unless we have got the mft reference of a matching name
	 * cached in mref in which case return mref.
	 */
	if (!(ie->ie_flags & INDEX_ENTRY_NODE)) {
		ntfs_attr_put_search_ctx(ctx);
		if (mref)
			return mref;
		ntfs_log_debug("Entry not found - between root entries.\n");
		errno = ENOENT;
		return -1;
	} /* Child node present, descend into it. */

	/* Open the index allocation attribute. */
	ia_na = ntfs_attr_open(dir_ni, AT_INDEX_ALLOCATION, NTFS_INDEX_I30, 4);
	if (!ia_na) {
		ntfs_log_perror("Failed to open index allocation (inode %lld)",
				(unsigned long long)dir_ni->mft_no);
		goto put_err_out;
	}

	/* Allocate a buffer for the current index block. */
	ia = ntfs_malloc(index_block_size);
	if (!ia) {
		ntfs_attr_close(ia_na);
		goto put_err_out;
	}

	/* Determine the size of a vcn in the directory index. */
	if (vol->cluster_size <= index_block_size) {
		index_vcn_size_bits = vol->cluster_size_bits;
	} else {
		index_vcn_size_bits = NTFS_BLOCK_SIZE_BITS;
	}

	/* Get the starting vcn of the index_block holding the child node. */
	vcn = sle64_to_cpup((sle64*)((u8*)ie + le16_to_cpu(ie->length) - 8));

descend_into_child_node:

	/* Read the index block starting at vcn. */
	br = ntfs_attr_mst_pread(ia_na, vcn << index_vcn_size_bits, 1,
			index_block_size, ia);
	if (br != 1) {
		if (br != -1)
			errno = EIO;
		ntfs_log_perror("Failed to read vcn 0x%llx",
			       	(unsigned long long)vcn);
		goto close_err_out;
	}

	if (sle64_to_cpu(ia->index_block_vcn) != vcn) {
		ntfs_log_error("Actual VCN (0x%llx) of index buffer is different "
				"from expected VCN (0x%llx).\n",
				(long long)sle64_to_cpu(ia->index_block_vcn),
				(long long)vcn);
		errno = EIO;
		goto close_err_out;
	}
	if (le32_to_cpu(ia->index.allocated_size) + 0x18 != index_block_size) {
		ntfs_log_error("Index buffer (VCN 0x%llx) of directory inode 0x%llx "
				"has a size (%u) differing from the directory "
				"specified size (%u).\n", (long long)vcn,
				(unsigned long long)dir_ni->mft_no,
				(unsigned) le32_to_cpu(ia->index.allocated_size) + 0x18,
				(unsigned)index_block_size);
		errno = EIO;
		goto close_err_out;
	}
	index_end = (u8*)&ia->index + le32_to_cpu(ia->index.index_length);
	if (index_end > (u8*)ia + index_block_size) {
		ntfs_log_error("Size of index buffer (VCN 0x%llx) of directory inode "
				"0x%llx exceeds maximum size.\n",
				(long long)vcn, (unsigned long long)dir_ni->mft_no);
		errno = EIO;
		goto close_err_out;
	}

	/* The first index entry. */
	ie = (INDEX_ENTRY*)((u8*)&ia->index +
			le32_to_cpu(ia->index.entries_offset));
	/*
	 * Iterate similar to above big loop but applied to index buffer, thus
	 * loop until we exceed valid memory (corruption case) or until we
	 * reach the last entry.
	 */
	for (;; ie = (INDEX_ENTRY*)((u8*)ie + le16_to_cpu(ie->length))) {
		/* Bounds check. */
		if ((u8*)ie < (u8*)ia || (u8*)ie +
				sizeof(INDEX_ENTRY_HEADER) > index_end ||
				(u8*)ie + le16_to_cpu(ie->key_length) >
				index_end) {
			ntfs_log_error("Index entry out of bounds in directory "
				       "inode %lld.\n", 
				       (unsigned long long)dir_ni->mft_no);
			errno = EIO;
			goto close_err_out;
		}
		/*
		 * The last entry cannot contain a name. It can however contain
		 * a pointer to a child node in the B+tree so we just break out.
		 */
		if (ie->ie_flags & INDEX_ENTRY_END)
			break;
		
		if (!le16_to_cpu(ie->length)) {
			errno = EIO;
			ntfs_log_error("Zero length index entry in inode %lld"
				       "\n", (unsigned long long)dir_ni->mft_no);
			goto close_err_out;
		}
		/*
		 * Not a perfect match, need to do full blown collation so we
		 * know which way in the B+tree we have to go.
		 */
		rc = ntfs_names_full_collate(uname, uname_len,
				(ntfschar*)&ie->key.file_name.file_name,
				ie->key.file_name.file_name_length,
				case_sensitivity, vol->upcase, vol->upcase_len);
		/*
		 * If uname collates before the name of the current entry, there
		 * is definitely no such name in this index but we might need to
		 * descend into the B+tree so we just break out of the loop.
		 */
		if (rc == -1)
			break;
		/* The names are not equal, continue the search. */
		if (rc)
			continue;
		mref = le64_to_cpu(ie->indexed_file);
		free(ia);
		ntfs_attr_close(ia_na);
		ntfs_attr_put_search_ctx(ctx);
		return mref;
	}
	/*
	 * We have finished with this index buffer without success. Check for
	 * the presence of a child node.
	 */
	if (ie->ie_flags & INDEX_ENTRY_NODE) {
		if ((ia->index.ih_flags & NODE_MASK) == LEAF_NODE) {
			ntfs_log_error("Index entry with child node found in a leaf "
					"node in directory inode %lld.\n",
					(unsigned long long)dir_ni->mft_no);
			errno = EIO;
			goto close_err_out;
		}
		/* Child node present, descend into it. */
		vcn = sle64_to_cpup((sle64*)((u8*)ie + le16_to_cpu(ie->length) - 8));
		if (vcn >= 0)
			goto descend_into_child_node;
		ntfs_log_error("Negative child node vcn in directory inode "
			       "0x%llx.\n", (unsigned long long)dir_ni->mft_no);
		errno = EIO;
		goto close_err_out;
	}
	free(ia);
	ntfs_attr_close(ia_na);
	ntfs_attr_put_search_ctx(ctx);
	/*
	 * No child node present, return error code ENOENT, unless we have got
	 * the mft reference of a matching name cached in mref in which case
	 * return mref.
	 */
	if (mref)
		return mref;
	ntfs_log_debug("Entry not found.\n");
	errno = ENOENT;
	return -1;
put_err_out:
	eo = EIO;
	ntfs_log_debug("Corrupt directory. Aborting lookup.\n");
eo_put_err_out:
	ntfs_attr_put_search_ctx(ctx);
	errno = eo;
	return -1;
close_err_out:
	eo = errno;
	free(ia);
	ntfs_attr_close(ia_na);
	goto eo_put_err_out;
}

/*
 *		Lookup a file in a directory from its UTF-8 name
 *
 *	The name is first fetched from cache if one is defined
 *
 *	Returns the inode number
 *		or -1 if not possible (errno tells why)
 */

u64 ntfs_inode_lookup_by_mbsname(ntfs_inode *dir_ni, const char *name)
{
	int uname_len;
	ntfschar *uname = (ntfschar*)NULL;
	u64 inum;
	char *cached_name;
	const char *const_name;

	if (!NVolCaseSensitive(dir_ni->vol)) {
		cached_name = ntfs_uppercase_mbs(name,
			dir_ni->vol->upcase, dir_ni->vol->upcase_len);
		const_name = cached_name;
	} else {
		cached_name = (char*)NULL;
		const_name = name;
	}
	if (const_name) {
#if CACHE_LOOKUP_SIZE

		/*
		 * fetch inode from cache
		 */

		if (dir_ni->vol->lookup_cache) {
			struct CACHED_LOOKUP item;
			struct CACHED_LOOKUP *cached;

			item.name = const_name;
			item.namesize = strlen(const_name) + 1;
			item.parent = dir_ni->mft_no;
			cached = (struct CACHED_LOOKUP*)ntfs_fetch_cache(
					dir_ni->vol->lookup_cache,
					GENERIC(&item), lookup_cache_compare);
			if (cached) {
				inum = cached->inum;
				if (inum == (u64)-1)
					errno = ENOENT;
			} else {
				/* Generate unicode name. */
				uname_len = ntfs_mbstoucs(name, &uname);
				if (uname_len >= 0) {
					inum = ntfs_inode_lookup_by_name(dir_ni,
							uname, uname_len);
					item.inum = inum;
				/* enter into cache, even if not found */
					ntfs_enter_cache(dir_ni->vol->lookup_cache,
							GENERIC(&item),
							lookup_cache_compare);
					free(uname);
				} else
					inum = (s64)-1;
			}
		} else
#endif
			{
				/* Generate unicode name. */
			uname_len = ntfs_mbstoucs(cached_name, &uname);
			if (uname_len >= 0)
				inum = ntfs_inode_lookup_by_name(dir_ni,
						uname, uname_len);
			else
				inum = (s64)-1;
		}
		if (cached_name)
			free(cached_name);
	} else
		inum = (s64)-1;
	return (inum);
}

/*
 *		Update a cache lookup record when a name has been defined
 *
 *	The UTF-8 name is required
 */

void ntfs_inode_update_mbsname(ntfs_inode *dir_ni, const char *name, u64 inum)
{
#if CACHE_LOOKUP_SIZE
	struct CACHED_LOOKUP item;
	struct CACHED_LOOKUP *cached;
	char *cached_name;

	if (dir_ni->vol->lookup_cache) {
		if (!NVolCaseSensitive(dir_ni->vol)) {
			cached_name = ntfs_uppercase_mbs(name,
				dir_ni->vol->upcase, dir_ni->vol->upcase_len);
			item.name = cached_name;
		} else {
			cached_name = (char*)NULL;
			item.name = name;
		}
		if (item.name) {
			item.namesize = strlen(item.name) + 1;
			item.parent = dir_ni->mft_no;
			item.inum = inum;
			cached = (struct CACHED_LOOKUP*)ntfs_enter_cache(
					dir_ni->vol->lookup_cache,
					GENERIC(&item), lookup_cache_compare);
			if (cached)
				cached->inum = inum;
			if (cached_name)
				free(cached_name);
		}
	}
#endif
}

/**
 * ntfs_pathname_to_inode - Find the inode which represents the given pathname
 * @vol:       An ntfs volume obtained from ntfs_mount
 * @parent:    A directory inode to begin the search (may be NULL)
 * @pathname:  Pathname to be located
 *
 * Take an ASCII pathname and find the inode that represents it.  The function
 * splits the path and then descends the directory tree.  If @parent is NULL,
 * then the root directory '.' will be used as the base for the search.
 *
 * Return:  inode  Success, the pathname was valid
 *	    NULL   Error, the pathname was invalid, or some other error occurred
 */
ntfs_inode *ntfs_pathname_to_inode(ntfs_volume *vol, ntfs_inode *parent,
		const char *pathname)
{
	u64 inum;
	int len, err = 0;
	char *p, *q;
	ntfs_inode *ni;
	ntfs_inode *result = NULL;
	ntfschar *unicode = NULL;
	char *ascii = NULL;
#if CACHE_INODE_SIZE
	struct CACHED_INODE item;
	struct CACHED_INODE *cached;
	char *fullname;
#endif

	if (!vol || !pathname) {
		errno = EINVAL;
		return NULL;
	}
	
	ntfs_log_trace("path: '%s'\n", pathname);
	
	ascii = strdup(pathname);
	if (!ascii) {
		ntfs_log_error("Out of memory.\n");
		err = ENOMEM;
		goto out;
	}

	p = ascii;
	/* Remove leading /'s. */
	while (p && *p && *p == PATH_SEP)
		p++;
#if CACHE_INODE_SIZE
	fullname = p;
	if (p[0] && (p[strlen(p)-1] == PATH_SEP))
		ntfs_log_error("Unnormalized path %s\n",ascii);
#endif
	if (parent) {
		ni = parent;
	} else {
#if CACHE_INODE_SIZE
			/*
			 * fetch inode for full path from cache
			 */
		if (*fullname) {
			item.pathname = fullname;
			item.varsize = strlen(fullname) + 1;
			cached = (struct CACHED_INODE*)ntfs_fetch_cache(
				vol->xinode_cache, GENERIC(&item),
				inode_cache_compare);
		} else
			cached = (struct CACHED_INODE*)NULL;
		if (cached) {
			/*
			 * return opened inode if found in cache
			 */
			inum = MREF(cached->inum);
			ni = ntfs_inode_open(vol, inum);
			if (!ni) {
				ntfs_log_debug("Cannot open inode %llu: %s.\n",
						(unsigned long long)inum, p);
				err = EIO;
			}
			result = ni;
			goto out;
		}
#endif
		ni = ntfs_inode_open(vol, FILE_root);
		if (!ni) {
			ntfs_log_debug("Couldn't open the inode of the root "
					"directory.\n");
			err = EIO;
			result = (ntfs_inode*)NULL;
			goto out;
		}
	}

	while (p && *p) {
		/* Find the end of the first token. */
		q = strchr(p, PATH_SEP);
		if (q != NULL) {
			*q = '\0';
		}
#if CACHE_INODE_SIZE
			/*
			 * fetch inode for partial path from cache
			 */
		cached = (struct CACHED_INODE*)NULL;
		if (!parent) {
			item.pathname = fullname;
			item.varsize = strlen(fullname) + 1;
			cached = (struct CACHED_INODE*)ntfs_fetch_cache(
					vol->xinode_cache, GENERIC(&item),
					inode_cache_compare);
			if (cached) {
				inum = cached->inum;
			}
		}
			/*
			 * if not in cache, translate, search, then
			 * insert into cache if found
			 */
		if (!cached) {
			len = ntfs_mbstoucs(p, &unicode);
			if (len < 0) {
				ntfs_log_perror("Could not convert filename to Unicode:"
					" '%s'", p);
				err = errno;
				goto close;
			} else if (len > NTFS_MAX_NAME_LEN) {
				err = ENAMETOOLONG;
				goto close;
			}
			inum = ntfs_inode_lookup_by_name(ni, unicode, len);
			if (!parent && (inum != (u64) -1)) {
				item.inum = inum;
				ntfs_enter_cache(vol->xinode_cache,
						GENERIC(&item),
						inode_cache_compare);
			}
		}
#else
		len = ntfs_mbstoucs(p, &unicode);
		if (len < 0) {
			ntfs_log_perror("Could not convert filename to Unicode:"
					" '%s'", p);
			err = errno;
			goto close;
		} else if (len > NTFS_MAX_NAME_LEN) {
			err = ENAMETOOLONG;
			goto close;
		}
		inum = ntfs_inode_lookup_by_name(ni, unicode, len);
#endif
		if (inum == (u64) -1) {
			ntfs_log_debug("Couldn't find name '%s' in pathname "
					"'%s'.\n", p, pathname);
			err = ENOENT;
			goto close;
		}

		if (ni != parent)
			if (ntfs_inode_close(ni)) {
				err = errno;
				goto out;
			}

		inum = MREF(inum);
		ni = ntfs_inode_open(vol, inum);
		if (!ni) {
			ntfs_log_debug("Cannot open inode %llu: %s.\n",
					(unsigned long long)inum, p);
			err = EIO;
			goto close;
		}
	
		free(unicode);
		unicode = NULL;

		if (q) *q++ = PATH_SEP; /* JPA */
		p = q;
		while (p && *p && *p == PATH_SEP)
			p++;
	}

	result = ni;
	ni = NULL;
close:
	if (ni && (ni != parent))
		if (ntfs_inode_close(ni) && !err)
			err = errno;
out:
	free(ascii);
	free(unicode);
	if (err)
		errno = err;
	return result;
}

/*
 * The little endian Unicode string ".." for ntfs_readdir().
 */
static const ntfschar dotdot[3] = { const_cpu_to_le16('.'),
				   const_cpu_to_le16('.'),
				   const_cpu_to_le16('\0') };

/*
 * union index_union -
 * More helpers for ntfs_readdir().
 */
typedef void* index_union;

/**
 * enum INDEX_TYPE -
 * More helpers for ntfs_readdir().
 */
typedef enum {
	INDEX_TYPE_ROOT,	/* index root */
	INDEX_TYPE_ALLOCATION,	/* index allocation */
} INDEX_TYPE;

/*
 *		Decode Interix file types
 *
 *	Non-Interix types are returned as plain files, because a
 *	Windows user may force patterns very similar to Interix,
 *	and most metadata files have such similar patters.
 */

u32 ntfs_interix_types(ntfs_inode *ni)
{
	ntfs_attr *na;
	u32 dt_type;
	le64 magic;

	dt_type = NTFS_DT_UNKNOWN;
	na = ntfs_attr_open(ni, AT_DATA, NULL, 0);
	if (na) {
		/*
		 * Unrecognized patterns (eg HID + SYST for metadata)
		 * are plain files or directories
		 */
		if (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)
			dt_type = NTFS_DT_DIR;
		else
			dt_type = NTFS_DT_REG;
		if (na->data_size <= 1) {
			if (!(ni->flags & FILE_ATTR_HIDDEN))
				dt_type = (na->data_size ?
						NTFS_DT_SOCK : NTFS_DT_FIFO);
		} else {
			if ((na->data_size >= (s64)sizeof(magic))
			    && (ntfs_attr_pread(na, 0, sizeof(magic), &magic)
				== sizeof(magic))) {
				if (magic == INTX_SYMBOLIC_LINK)
					dt_type = NTFS_DT_LNK;
				else if (magic == INTX_BLOCK_DEVICE)
					dt_type = NTFS_DT_BLK;
				else if (magic == INTX_CHARACTER_DEVICE)
					dt_type = NTFS_DT_CHR;
			}
		}
		ntfs_attr_close(na);
	}
	return (dt_type);
}

/*
 *		Decode file types
 *
 *	Better only use for Interix types and junctions,
 *	unneeded complexity when used for plain files or directories
 *
 *	Error cases are logged and returned as unknown.
 */

static u32 ntfs_dir_entry_type(ntfs_inode *dir_ni, MFT_REF mref,
					FILE_ATTR_FLAGS attributes)
{
	ntfs_inode *ni;
	u32 dt_type;

	dt_type = NTFS_DT_UNKNOWN;
	ni = ntfs_inode_open(dir_ni->vol, mref);
	if (ni) {
		if ((attributes & FILE_ATTR_SYSTEM)
			&& !(attributes & FILE_ATTR_I30_INDEX_PRESENT))
			dt_type = ntfs_interix_types(ni);
		else
			dt_type = (attributes
					& FILE_ATTR_I30_INDEX_PRESENT
				? NTFS_DT_DIR : NTFS_DT_REG);
		if (ntfs_inode_close(ni)) {
				 /* anything special worth doing ? */
			ntfs_log_error("Failed to close inode %lld\n",
				(long long)MREF(mref));
		}
	}
	if (dt_type == NTFS_DT_UNKNOWN)
		ntfs_log_error("Could not decode the type of inode %lld\n",
				(long long)MREF(mref));
	return (dt_type);
}

/**
 * ntfs_filldir - ntfs specific filldir method
 * @dir_ni:	ntfs inode of current directory
 * @pos:	current position in directory
 * @ivcn_bits:	log(2) of index vcn size
 * @index_type:	specifies whether @iu is an index root or an index allocation
 * @iu:		index root or index block to which @ie belongs
 * @ie:		current index entry
 * @dirent:	context for filldir callback supplied by the caller
 * @filldir:	filldir callback supplied by the caller
 *
 * Pass information specifying the current directory entry @ie to the @filldir
 * callback.
 */
static int ntfs_filldir(ntfs_inode *dir_ni, s64 *pos, u8 ivcn_bits,
		const INDEX_TYPE index_type, index_union iu, INDEX_ENTRY *ie,
		void *dirent, ntfs_filldir_t filldir)
{
	FILE_NAME_ATTR *fn = &ie->key.file_name;
	unsigned dt_type;
	BOOL metadata;
	ntfschar *loname;
	int res;
	MFT_REF mref;

	ntfs_log_trace("Entering.\n");
	
	/* Advance the position even if going to skip the entry. */
	if (index_type == INDEX_TYPE_ALLOCATION)
		*pos = (u8*)ie - (u8*)iu + (sle64_to_cpu(
				((INDEX_ALLOCATION*)iu)->index_block_vcn) << ivcn_bits) +
				dir_ni->vol->mft_record_size;
	else /* if (index_type == INDEX_TYPE_ROOT) */
		*pos = (u8*)ie - (u8*)iu;
	mref = le64_to_cpu(ie->indexed_file);
	metadata = (MREF(mref) != FILE_root) && (MREF(mref) < FILE_first_user);
	/* Skip root directory self reference entry. */
	if (MREF_LE(ie->indexed_file) == FILE_root)
		return 0;
	if ((ie->key.file_name.file_attributes
		     & (FILE_ATTR_REPARSE_POINT | FILE_ATTR_SYSTEM))
	    && !metadata)
		dt_type = ntfs_dir_entry_type(dir_ni, mref,
					ie->key.file_name.file_attributes);
	else if (ie->key.file_name.file_attributes
		     & FILE_ATTR_I30_INDEX_PRESENT)
		dt_type = NTFS_DT_DIR;
	else
		dt_type = NTFS_DT_REG;

		/* return metadata files and hidden files if requested */
        if ((!metadata && (NVolShowHidFiles(dir_ni->vol)
				|| !(fn->file_attributes & FILE_ATTR_HIDDEN)))
            || (NVolShowSysFiles(dir_ni->vol) && (NVolShowHidFiles(dir_ni->vol)
				|| metadata))) {
		if (NVolCaseSensitive(dir_ni->vol)) {
			res = filldir(dirent, fn->file_name,
					fn->file_name_length,
					fn->file_name_type, *pos,
					mref, dt_type);
		} else {
			loname = (ntfschar*)ntfs_malloc(2*fn->file_name_length);
			if (loname) {
				memcpy(loname, fn->file_name,
					2*fn->file_name_length);
				ntfs_name_locase(loname, fn->file_name_length,
					dir_ni->vol->locase,
					dir_ni->vol->upcase_len);
				res = filldir(dirent, loname,
					fn->file_name_length,
					fn->file_name_type, *pos,
					mref, dt_type);
				free(loname);
			} else
				res = -1;
		}
	} else
		res = 0;
	return (res);
}

/**
 * ntfs_mft_get_parent_ref - find mft reference of parent directory of an inode
 * @ni:		ntfs inode whose parent directory to find
 *
 * Find the parent directory of the ntfs inode @ni. To do this, find the first
 * file name attribute in the mft record of @ni and return the parent mft
 * reference from that.
 *
 * Note this only makes sense for directories, since files can be hard linked
 * from multiple directories and there is no way for us to tell which one is
 * being looked for.
 *
 * Technically directories can have hard links, too, but we consider that as
 * illegal as Linux/UNIX do not support directory hard links.
 *
 * Return the mft reference of the parent directory on success or -1 on error
 * with errno set to the error code.
 */
static MFT_REF ntfs_mft_get_parent_ref(ntfs_inode *ni)
{
	MFT_REF mref;
	ntfs_attr_search_ctx *ctx;
	FILE_NAME_ATTR *fn;
	int eo;

	ntfs_log_trace("Entering.\n");
	
	if (!ni) {
		errno = EINVAL;
		return ERR_MREF(-1);
	}

	ctx = ntfs_attr_get_search_ctx(ni, NULL);
	if (!ctx)
		return ERR_MREF(-1);
	if (ntfs_attr_lookup(AT_FILE_NAME, AT_UNNAMED, 0, 0, 0, NULL, 0, ctx)) {
		ntfs_log_error("No file name found in inode %lld\n", 
			       (unsigned long long)ni->mft_no);
		goto err_out;
	}
	if (ctx->attr->non_resident) {
		ntfs_log_error("File name attribute must be resident (inode "
			       "%lld)\n", (unsigned long long)ni->mft_no);
		goto io_err_out;
	}
	fn = (FILE_NAME_ATTR*)((u8*)ctx->attr +
			le16_to_cpu(ctx->attr->value_offset));
	if ((u8*)fn +	le32_to_cpu(ctx->attr->value_length) >
			(u8*)ctx->attr + le32_to_cpu(ctx->attr->length)) {
		ntfs_log_error("Corrupt file name attribute in inode %lld.\n",
			       (unsigned long long)ni->mft_no);
		goto io_err_out;
	}
	mref = le64_to_cpu(fn->parent_directory);
	ntfs_attr_put_search_ctx(ctx);
	return mref;
io_err_out:
	errno = EIO;
err_out:
	eo = errno;
	ntfs_attr_put_search_ctx(ctx);
	errno = eo;
	return ERR_MREF(-1);
}

static int ffs(int i)
{
	int bit;

	if (0 == i)
		return 0;

	for (bit = 1; !(i & 1); ++bit)
		i >>= 1;
	return bit;
}

/**
 * ntfs_readdir - read the contents of an ntfs directory
 * @dir_ni:	ntfs inode of current directory
 * @pos:	current position in directory
 * @dirent:	context for filldir callback supplied by the caller
 * @filldir:	filldir callback supplied by the caller
 *
 * Parse the index root and the index blocks that are marked in use in the
 * index bitmap and hand each found directory entry to the @filldir callback
 * supplied by the caller.
 *
 * Return 0 on success or -1 on error with errno set to the error code.
 *
 * Note: Index blocks are parsed in ascending vcn order, from which follows
 * that the directory entries are not returned sorted.
 */
int ntfs_readdir(ntfs_inode *dir_ni, s64 *pos,
		void *dirent, ntfs_filldir_t filldir)
{
	s64 i_size, br, ia_pos, bmp_pos, ia_start;
	ntfs_volume *vol;
	ntfs_attr *ia_na, *bmp_na = NULL;
	ntfs_attr_search_ctx *ctx = NULL;
	u8 *index_end, *bmp = NULL;
	INDEX_ROOT *ir;
	INDEX_ENTRY *ie;
	INDEX_ALLOCATION *ia = NULL;
	int rc, ir_pos, bmp_buf_size, bmp_buf_pos, eo;
	u32 index_block_size;
	u8 index_block_size_bits, index_vcn_size_bits;

	ntfs_log_trace("Entering.\n");
	
	if (!dir_ni || !pos || !filldir) {
		errno = EINVAL;
		return -1;
	}

	if (!(dir_ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)) {
		errno = ENOTDIR;
		return -1;
	}

	vol = dir_ni->vol;

	ntfs_log_trace("Entering for inode %lld, *pos 0x%llx.\n",
			(unsigned long long)dir_ni->mft_no, (long long)*pos);

	/* Open the index allocation attribute. */
	ia_na = ntfs_attr_open(dir_ni, AT_INDEX_ALLOCATION, NTFS_INDEX_I30, 4);
	if (!ia_na) {
		if (errno != ENOENT) {
			ntfs_log_perror("Failed to open index allocation attribute. "
				"Directory inode %lld is corrupt or bug",
				(unsigned long long)dir_ni->mft_no);
			return -1;
		}
		i_size = 0;
	} else
		i_size = ia_na->data_size;

	rc = 0;

	/* Are we at end of dir yet? */
	if (*pos >= i_size + vol->mft_record_size)
		goto done;

	/* Emulate . and .. for all directories. */
	if (!*pos) {
		rc = filldir(dirent, dotdot, 1, FILE_NAME_POSIX, *pos,
				MK_MREF(dir_ni->mft_no,
				le16_to_cpu(dir_ni->mrec->sequence_number)),
				NTFS_DT_DIR);
		if (rc)
			goto err_out;
		++*pos;
	}
	if (*pos == 1) {
		MFT_REF parent_mref;

		parent_mref = ntfs_mft_get_parent_ref(dir_ni);
		if (parent_mref == ERR_MREF(-1)) {
			ntfs_log_perror("Parent directory not found");
			goto dir_err_out;
		}

		rc = filldir(dirent, dotdot, 2, FILE_NAME_POSIX, *pos,
				parent_mref, NTFS_DT_DIR);
		if (rc)
			goto err_out;
		++*pos;
	}

	ctx = ntfs_attr_get_search_ctx(dir_ni, NULL);
	if (!ctx)
		goto err_out;

	/* Get the offset into the index root attribute. */
	ir_pos = (int)*pos;
	/* Find the index root attribute in the mft record. */
	if (ntfs_attr_lookup(AT_INDEX_ROOT, NTFS_INDEX_I30, 4, CASE_SENSITIVE, 0, NULL,
			0, ctx)) {
		ntfs_log_perror("Index root attribute missing in directory inode "
				"%lld", (unsigned long long)dir_ni->mft_no);
		goto dir_err_out;
	}
	/* Get to the index root value. */
	ir = (INDEX_ROOT*)((u8*)ctx->attr +
			le16_to_cpu(ctx->attr->value_offset));

	/* Determine the size of a vcn in the directory index. */
	index_block_size = le32_to_cpu(ir->index_block_size);
	if (index_block_size < NTFS_BLOCK_SIZE ||
			index_block_size & (index_block_size - 1)) {
		ntfs_log_error("Index block size %u is invalid.\n",
				(unsigned)index_block_size);
		goto dir_err_out;
	}
	index_block_size_bits = ffs(index_block_size) - 1;
	if (vol->cluster_size <= index_block_size) {
		index_vcn_size_bits = vol->cluster_size_bits;
	} else {
		index_vcn_size_bits = NTFS_BLOCK_SIZE_BITS;
	}

	/* Are we jumping straight into the index allocation attribute? */
	if (*pos >= vol->mft_record_size) {
		ntfs_attr_put_search_ctx(ctx);
		ctx = NULL;
		goto skip_index_root;
	}

	index_end = (u8*)&ir->index + le32_to_cpu(ir->index.index_length);
	/* The first index entry. */
	ie = (INDEX_ENTRY*)((u8*)&ir->index +
			le32_to_cpu(ir->index.entries_offset));
	/*
	 * Loop until we exceed valid memory (corruption case) or until we
	 * reach the last entry or until filldir tells us it has had enough
	 * or signals an error (both covered by the rc test).
	 */
	for (;; ie = (INDEX_ENTRY*)((u8*)ie + le16_to_cpu(ie->length))) {
		ntfs_log_debug("In index root, offset %d.\n", (int)((u8*)ie - (u8*)ir));
		/* Bounds checks. */
		if ((u8*)ie < (u8*)ctx->mrec || (u8*)ie +
				sizeof(INDEX_ENTRY_HEADER) > index_end ||
				(u8*)ie + le16_to_cpu(ie->key_length) >
				index_end)
			goto dir_err_out;
		/* The last entry cannot contain a name. */
		if (ie->ie_flags & INDEX_ENTRY_END)
			break;
		
		if (!le16_to_cpu(ie->length))
			goto dir_err_out;
		
		/* Skip index root entry if continuing previous readdir. */
		if (ir_pos > (u8*)ie - (u8*)ir)
			continue;
		/*
		 * Submit the directory entry to ntfs_filldir(), which will
		 * invoke the filldir() callback as appropriate.
		 */
		rc = ntfs_filldir(dir_ni, pos, index_vcn_size_bits,
				INDEX_TYPE_ROOT, ir, ie, dirent, filldir);
		if (rc) {
			ntfs_attr_put_search_ctx(ctx);
			ctx = NULL;
			goto err_out;
		}
	}
	ntfs_attr_put_search_ctx(ctx);
	ctx = NULL;

	/* If there is no index allocation attribute we are finished. */
	if (!ia_na)
		goto EOD;

	/* Advance *pos to the beginning of the index allocation. */
	*pos = vol->mft_record_size;

skip_index_root:

	if (!ia_na)
		goto done;

	/* Allocate a buffer for the current index block. */
	ia = ntfs_malloc(index_block_size);
	if (!ia)
		goto err_out;

	bmp_na = ntfs_attr_open(dir_ni, AT_BITMAP, NTFS_INDEX_I30, 4);
	if (!bmp_na) {
		ntfs_log_perror("Failed to open index bitmap attribute");
		goto dir_err_out;
	}

	/* Get the offset into the index allocation attribute. */
	ia_pos = *pos - vol->mft_record_size;

	bmp_pos = ia_pos >> index_block_size_bits;
	if (bmp_pos >> 3 >= bmp_na->data_size) {
		ntfs_log_error("Current index position exceeds index bitmap "
				"size.\n");
		goto dir_err_out;
	}

	bmp_buf_size = min(bmp_na->data_size - (bmp_pos >> 3), 4096);
	bmp = ntfs_malloc(bmp_buf_size);
	if (!bmp)
		goto err_out;

	br = ntfs_attr_pread(bmp_na, bmp_pos >> 3, bmp_buf_size, bmp);
	if (br != bmp_buf_size) {
		if (br != -1)
			errno = EIO;
		ntfs_log_perror("Failed to read from index bitmap attribute");
		goto err_out;
	}

	bmp_buf_pos = 0;
	/* If the index block is not in use find the next one that is. */
	while (!(bmp[bmp_buf_pos >> 3] & (1 << (bmp_buf_pos & 7)))) {
find_next_index_buffer:
		bmp_pos++;
		bmp_buf_pos++;
		/* If we have reached the end of the bitmap, we are done. */
		if (bmp_pos >> 3 >= bmp_na->data_size)
			goto EOD;
		ia_pos = bmp_pos << index_block_size_bits;
		if (bmp_buf_pos >> 3 < bmp_buf_size)
			continue;
		/* Read next chunk from the index bitmap. */
		bmp_buf_pos = 0;
		if ((bmp_pos >> 3) + bmp_buf_size > bmp_na->data_size)
			bmp_buf_size = bmp_na->data_size - (bmp_pos >> 3);
		br = ntfs_attr_pread(bmp_na, bmp_pos >> 3, bmp_buf_size, bmp);
		if (br != bmp_buf_size) {
			if (br != -1)
				errno = EIO;
			ntfs_log_perror("Failed to read from index bitmap attribute");
			goto err_out;
		}
	}

	ntfs_log_debug("Handling index block 0x%llx.\n", (long long)bmp_pos);

	/* Read the index block starting at bmp_pos. */
	br = ntfs_attr_mst_pread(ia_na, bmp_pos << index_block_size_bits, 1,
			index_block_size, ia);
	if (br != 1) {
		if (br != -1)
			errno = EIO;
		ntfs_log_perror("Failed to read index block");
		goto err_out;
	}

	ia_start = ia_pos & ~(s64)(index_block_size - 1);
	if (sle64_to_cpu(ia->index_block_vcn) != ia_start >>
			index_vcn_size_bits) {
		ntfs_log_error("Actual VCN (0x%llx) of index buffer is different "
				"from expected VCN (0x%llx) in inode 0x%llx.\n",
				(long long)sle64_to_cpu(ia->index_block_vcn),
				(long long)ia_start >> index_vcn_size_bits,
				(unsigned long long)dir_ni->mft_no);
		goto dir_err_out;
	}
	if (le32_to_cpu(ia->index.allocated_size) + 0x18 != index_block_size) {
		ntfs_log_error("Index buffer (VCN 0x%llx) of directory inode %lld "
				"has a size (%u) differing from the directory "
				"specified size (%u).\n", (long long)ia_start >>
				index_vcn_size_bits,
				(unsigned long long)dir_ni->mft_no,
				(unsigned) le32_to_cpu(ia->index.allocated_size)
				+ 0x18, (unsigned)index_block_size);
		goto dir_err_out;
	}
	index_end = (u8*)&ia->index + le32_to_cpu(ia->index.index_length);
	if (index_end > (u8*)ia + index_block_size) {
		ntfs_log_error("Size of index buffer (VCN 0x%llx) of directory inode "
				"%lld exceeds maximum size.\n",
				(long long)ia_start >> index_vcn_size_bits,
				(unsigned long long)dir_ni->mft_no);
		goto dir_err_out;
	}
	/* The first index entry. */
	ie = (INDEX_ENTRY*)((u8*)&ia->index +
			le32_to_cpu(ia->index.entries_offset));
	/*
	 * Loop until we exceed valid memory (corruption case) or until we
	 * reach the last entry or until ntfs_filldir tells us it has had
	 * enough or signals an error (both covered by the rc test).
	 */
	for (;; ie = (INDEX_ENTRY*)((u8*)ie + le16_to_cpu(ie->length))) {
		ntfs_log_debug("In index allocation, offset 0x%llx.\n",
				(long long)ia_start + ((u8*)ie - (u8*)ia));
		/* Bounds checks. */
		if ((u8*)ie < (u8*)ia || (u8*)ie +
				sizeof(INDEX_ENTRY_HEADER) > index_end ||
				(u8*)ie + le16_to_cpu(ie->key_length) >
				index_end) {
			ntfs_log_error("Index entry out of bounds in directory inode "
				"%lld.\n", (unsigned long long)dir_ni->mft_no);
			goto dir_err_out;
		}
		/* The last entry cannot contain a name. */
		if (ie->ie_flags & INDEX_ENTRY_END)
			break;
		
		if (!le16_to_cpu(ie->length))
			goto dir_err_out;
		
		/* Skip index entry if continuing previous readdir. */
		if (ia_pos - ia_start > (u8*)ie - (u8*)ia)
			continue;
		/*
		 * Submit the directory entry to ntfs_filldir(), which will
		 * invoke the filldir() callback as appropriate.
		 */
		rc = ntfs_filldir(dir_ni, pos, index_vcn_size_bits,
				INDEX_TYPE_ALLOCATION, ia, ie, dirent, filldir);
		if (rc)
			goto err_out;
	}
	goto find_next_index_buffer;
EOD:
	/* We are finished, set *pos to EOD. */
	*pos = i_size + vol->mft_record_size;
done:
	free(ia);
	free(bmp);
	if (bmp_na)
		ntfs_attr_close(bmp_na);
	if (ia_na)
		ntfs_attr_close(ia_na);
	ntfs_log_debug("EOD, *pos 0x%llx, returning 0.\n", (long long)*pos);
	return 0;
dir_err_out:
	errno = EIO;
err_out:
	eo = errno;
	ntfs_log_trace("failed.\n");
	if (ctx)
		ntfs_attr_put_search_ctx(ctx);
	free(ia);
	free(bmp);
	if (bmp_na)
		ntfs_attr_close(bmp_na);
	if (ia_na)
		ntfs_attr_close(ia_na);
	errno = eo;
	return -1;
}

int ntfs_check_empty_dir(ntfs_inode *ni)
{
	ntfs_attr *na;
	int ret = 0;
	
	if (!(ni->mrec->flags & MFT_RECORD_IS_DIRECTORY))
		return 0;

	na = ntfs_attr_open(ni, AT_INDEX_ROOT, NTFS_INDEX_I30, 4);
	if (!na) {
		errno = EIO;
		ntfs_log_perror("Failed to open directory");
		return -1;
	}
	
	/* Non-empty directory? */
	if ((na->data_size != sizeof(INDEX_ROOT) + sizeof(INDEX_ENTRY_HEADER))){
		/* Both ENOTEMPTY and EEXIST are ok. We use the more common. */
		errno = ENOTEMPTY;
		ntfs_log_debug("Directory is not empty\n");
		ret = -1;
	}
	
	ntfs_attr_close(na);
	return ret;
}

static int ntfs_check_unlinkable_dir(ntfs_inode *ni, FILE_NAME_ATTR *fn)
{
	int link_count = le16_to_cpu(ni->mrec->link_count);
	int ret;
	
	ret = ntfs_check_empty_dir(ni);
	if (!ret || errno != ENOTEMPTY)
		return ret;
	/* 
	 * Directory is non-empty, so we can unlink only if there is more than
	 * one "real" hard link, i.e. links aren't different DOS and WIN32 names
	 */
	if ((link_count == 1) || 
	    (link_count == 2 && fn->file_name_type == FILE_NAME_DOS)) {
		errno = ENOTEMPTY;
		ntfs_log_debug("Non-empty directory without hard links\n");
		goto no_hardlink;
	}
	
	ret = 0;
no_hardlink:	
	return ret;
}

/**
 * ntfs_link - create hard link for file or directory
 * @ni:		ntfs inode for object to create hard link
 * @dir_ni:	ntfs inode for directory in which new link should be placed
 * @name:	unicode name of the new link
 * @name_len:	length of the name in unicode characters
 *
 * NOTE: At present we allow creating hardlinks to directories, we use them
 * in a temporary state during rename. But it's defenitely bad idea to have
 * hard links to directories as a result of operation.
 * FIXME: Create internal  __ntfs_link that allows hard links to a directories
 * and external ntfs_link that do not. Write ntfs_rename that uses __ntfs_link.
 *
 * Return 0 on success or -1 on error with errno set to the error code.
 */
static int ntfs_link_i(ntfs_inode *ni, ntfs_inode *dir_ni, const ntfschar *name,
			 u8 name_len, FILE_NAME_TYPE_FLAGS nametype)
{
	FILE_NAME_ATTR *fn = NULL;
	int fn_len, err;

	ntfs_log_trace("Entering.\n");
	
	if (!ni || !dir_ni || !name || !name_len || 
			ni->mft_no == dir_ni->mft_no) {
		err = EINVAL;
		ntfs_log_perror("ntfs_link wrong arguments");
		goto err_out;
	}
	
	if (NVolHideDotFiles(dir_ni->vol)) {
		/* Set hidden flag according to the latest name */
		if ((name_len > 1)
		    && (name[0] == const_cpu_to_le16('.'))
		    && (name[1] != const_cpu_to_le16('.')))
			ni->flags |= FILE_ATTR_HIDDEN;
		else
			ni->flags &= ~FILE_ATTR_HIDDEN;
	}
	
	/* Create FILE_NAME attribute. */
	fn_len = sizeof(FILE_NAME_ATTR) + name_len * sizeof(ntfschar);
	fn = ntfs_calloc(fn_len);
	if (!fn) {
		err = errno;
		goto err_out;
	}
	fn->parent_directory = MK_LE_MREF(dir_ni->mft_no,
			le16_to_cpu(dir_ni->mrec->sequence_number));
	fn->file_name_length = name_len;
	fn->file_name_type = nametype;
	fn->file_attributes = ni->flags;
	if (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY) {
		fn->file_attributes |= FILE_ATTR_I30_INDEX_PRESENT;
		fn->data_size = fn->allocated_size = const_cpu_to_sle64(0);
	} else {
		fn->allocated_size = cpu_to_sle64(ni->allocated_size);
		fn->data_size = cpu_to_sle64(ni->data_size);
	}
	fn->creation_time = ni->creation_time;
	fn->last_data_change_time = ni->last_data_change_time;
	fn->last_mft_change_time = ni->last_mft_change_time;
	fn->last_access_time = ni->last_access_time;
	memcpy(fn->file_name, name, name_len * sizeof(ntfschar));
	/* Add FILE_NAME attribute to index. */
	if (ntfs_index_add_filename(dir_ni, fn, MK_MREF(ni->mft_no,
			le16_to_cpu(ni->mrec->sequence_number)))) {
		err = errno;
		ntfs_log_perror("Failed to add filename to the index");
		goto err_out;
	}
	/* Add FILE_NAME attribute to inode. */
	if (ntfs_attr_add(ni, AT_FILE_NAME, AT_UNNAMED, 0, (u8*)fn, fn_len)) {
		ntfs_log_error("Failed to add FILE_NAME attribute.\n");
		err = errno;
		/* Try to remove just added attribute from index. */
		if (ntfs_index_remove(dir_ni, ni, fn, fn_len))
			goto rollback_failed;
		goto err_out;
	}
	/* Increment hard links count. */
	ni->mrec->link_count = cpu_to_le16(le16_to_cpu(
			ni->mrec->link_count) + 1);
	/* Done! */
	ntfs_inode_mark_dirty(ni);
	free(fn);
	ntfs_log_trace("Done.\n");
	return 0;
rollback_failed:
	ntfs_log_error("Rollback failed. Leaving inconsistent metadata.\n");
err_out:
	free(fn);
	errno = err;
	return -1;
}

int ntfs_link(ntfs_inode *ni, ntfs_inode *dir_ni, const ntfschar *name,
		u8 name_len)
{
	return (ntfs_link_i(ni, dir_ni, name, name_len, FILE_NAME_POSIX));
}

/*
 *		Get a parent directory from an inode entry
 *
 *	This is only used in situations where the path used to access
 *	the current file is not known for sure. The result may be different
 *	from the path when the file is linked in several parent directories.
 *
 *	Currently this is only used for translating ".." in the target
 *	of a Vista relative symbolic link
 */

ntfs_inode *ntfs_dir_parent_inode(ntfs_inode *ni)
{
	ntfs_inode *dir_ni = (ntfs_inode*)NULL;
	u64 inum;
	FILE_NAME_ATTR *fn;
	ntfs_attr_search_ctx *ctx;

	if (ni->mft_no != FILE_root) {
			/* find the name in the attributes */
		ctx = ntfs_attr_get_search_ctx(ni, NULL);
		if (!ctx)
			return ((ntfs_inode*)NULL);

		if (!ntfs_attr_lookup(AT_FILE_NAME, AT_UNNAMED, 0,
				CASE_SENSITIVE,	0, NULL, 0, ctx)) {
			/* We know this will always be resident. */
			fn = (FILE_NAME_ATTR*)((u8*)ctx->attr +
					le16_to_cpu(ctx->attr->value_offset));
			inum = le64_to_cpu(fn->parent_directory);
			if (inum != (u64)-1) {
				dir_ni = ntfs_inode_open(ni->vol, MREF(inum));
			}
		}
		ntfs_attr_put_search_ctx(ctx);
	}
	return (dir_ni);
}

#define MAX_DOS_NAME_LENGTH	 12

/*
 *		Get a DOS name for a file in designated directory
 *
 *	Not allowed if there are several non-dos names (EMLINK)
 *
 *	Returns size if found
 *		0 if not found
 *		-1 if there was an error (described by errno)
 */

static int get_dos_name(ntfs_inode *ni, u64 dnum, ntfschar *dosname)
{
	size_t outsize = 0;
	int namecount = 0;
	FILE_NAME_ATTR *fn;
	ntfs_attr_search_ctx *ctx;

		/* find the name in the attributes */
	ctx = ntfs_attr_get_search_ctx(ni, NULL);
	if (!ctx)
		return -1;

	while (!ntfs_attr_lookup(AT_FILE_NAME, AT_UNNAMED, 0, CASE_SENSITIVE,
			0, NULL, 0, ctx)) {
		/* We know this will always be resident. */
		fn = (FILE_NAME_ATTR*)((u8*)ctx->attr +
				le16_to_cpu(ctx->attr->value_offset));

		if (fn->file_name_type != FILE_NAME_DOS)
			namecount++;
		if ((fn->file_name_type & FILE_NAME_DOS)
		    && (MREF_LE(fn->parent_directory) == dnum)) {
				/*
				 * Found a DOS or WIN32+DOS name for the entry
				 * copy name, after truncation for safety
				 */
			outsize = fn->file_name_length;
/* TODO : reject if name is too long ? */
			if (outsize > MAX_DOS_NAME_LENGTH)
				outsize = MAX_DOS_NAME_LENGTH;
			memcpy(dosname,fn->file_name,outsize*sizeof(ntfschar));
		}
	}
	ntfs_attr_put_search_ctx(ctx);
	if ((outsize > 0) && (namecount > 1)) {
		outsize = -1;
		errno = EMLINK; /* this error implies there is a dos name */
	}
	return (outsize);
}


/*
 *		Get a long name for a file in designated directory
 *
 *	Not allowed if there are several non-dos names (EMLINK)
 *
 *	Returns size if found
 *		0 if not found
 *		-1 if there was an error (described by errno)
 */

static int get_long_name(ntfs_inode *ni, u64 dnum, ntfschar *longname)
{
	size_t outsize = 0;
	int namecount = 0;
	FILE_NAME_ATTR *fn;
	ntfs_attr_search_ctx *ctx;

		/* find the name in the attributes */
	ctx = ntfs_attr_get_search_ctx(ni, NULL);
	if (!ctx)
		return -1;

		/* first search for WIN32 or DOS+WIN32 names */
	while (!ntfs_attr_lookup(AT_FILE_NAME, AT_UNNAMED, 0, CASE_SENSITIVE,
			0, NULL, 0, ctx)) {
		/* We know this will always be resident. */
		fn = (FILE_NAME_ATTR*)((u8*)ctx->attr +
				le16_to_cpu(ctx->attr->value_offset));

		if (fn->file_name_type != FILE_NAME_DOS)
			namecount++;
		if ((fn->file_name_type & FILE_NAME_WIN32)
		    && (MREF_LE(fn->parent_directory) == dnum)) {
				/*
				 * Found a WIN32 or WIN32+DOS name for the entry
				 * copy name
				 */
			outsize = fn->file_name_length;
			memcpy(longname,fn->file_name,outsize*sizeof(ntfschar));
		}
	}
	if (namecount > 1) {
		ntfs_attr_put_search_ctx(ctx);
		errno = EMLINK;
		return -1;
	}
		/* if not found search for POSIX names */
	if (!outsize) {
		ntfs_attr_reinit_search_ctx(ctx);
	while (!ntfs_attr_lookup(AT_FILE_NAME, AT_UNNAMED, 0, CASE_SENSITIVE,
			0, NULL, 0, ctx)) {
		/* We know this will always be resident. */
		fn = (FILE_NAME_ATTR*)((u8*)ctx->attr +
				le16_to_cpu(ctx->attr->value_offset));

		if ((fn->file_name_type == FILE_NAME_POSIX)
		    && (MREF_LE(fn->parent_directory) == dnum)) {
				/*
				 * Found a POSIX name for the entry
				 * copy name
				 */
			outsize = fn->file_name_length;
			memcpy(longname,fn->file_name,outsize*sizeof(ntfschar));
		}
	}
	}
	ntfs_attr_put_search_ctx(ctx);
	return (outsize);
}


/*
 *		Get the ntfs DOS name into an extended attribute
 */

int ntfs_get_ntfs_dos_name(ntfs_inode *ni, ntfs_inode *dir_ni,
			char *value, size_t size)
{
	int outsize = 0;
	char *outname = (char*)NULL;
	u64 dnum;
	int doslen;
	ntfschar dosname[MAX_DOS_NAME_LENGTH];

	dnum = dir_ni->mft_no;
	doslen = get_dos_name(ni, dnum, dosname);
	if (doslen > 0) {
			/*
			 * Found a DOS name for the entry, make
			 * uppercase and encode into the buffer
			 * if there is enough space
			 */
		ntfs_name_upcase(dosname, doslen,
				ni->vol->upcase, ni->vol->upcase_len);
		outsize = ntfs_ucstombs(dosname, doslen, &outname, 0);
		if (outsize < 0) {
			ntfs_log_error("Cannot represent dosname in current locale.\n");
			outsize = -errno;
		} else {
			if (value && (outsize <= (int)size))
				memcpy(value, outname, outsize);
			else
				if (size && (outsize > (int)size))
					outsize = -ERANGE;
			free(outname);
		}
	} else {
		if (doslen == 0)
			errno = ENODATA;
		outsize = -errno;
	}
	return (outsize);
}

/*
 *		Change the name space of an existing file or directory
 *
 *	Returns the old namespace if successful
 *		-1 if an error occurred (described by errno)
 */

static int set_namespace(ntfs_inode *ni, ntfs_inode *dir_ni,
			const ntfschar *name, int len,
			FILE_NAME_TYPE_FLAGS nametype)
{
	ntfs_attr_search_ctx *actx;
	ntfs_index_context *icx;
	FILE_NAME_ATTR *fnx;
	FILE_NAME_ATTR *fn = NULL;
	BOOL found;
	int lkup;
	int ret;

	ret = -1;
	actx = ntfs_attr_get_search_ctx(ni, NULL);
	if (actx) {
		found = FALSE;
		do {
			lkup = ntfs_attr_lookup(AT_FILE_NAME, AT_UNNAMED, 0,
	                        CASE_SENSITIVE, 0, NULL, 0, actx);
			if (!lkup) {
				fn = (FILE_NAME_ATTR*)((u8*)actx->attr +
				     le16_to_cpu(actx->attr->value_offset));
				found = (MREF_LE(fn->parent_directory)
						== dir_ni->mft_no)
					&& !memcmp(fn->file_name, name,
						len*sizeof(ntfschar));
			}
		} while (!lkup && !found);
		if (found) {
			icx = ntfs_index_ctx_get(dir_ni, NTFS_INDEX_I30, 4);
			if (icx) {
				lkup = ntfs_index_lookup((char*)fn, len, icx);
				if (!lkup && icx->data && icx->data_len) {
					fnx = (FILE_NAME_ATTR*)icx->data;
					ret = fn->file_name_type;
					fn->file_name_type = nametype;
					fnx->file_name_type = nametype;
					ntfs_inode_mark_dirty(ni);
					ntfs_index_entry_mark_dirty(icx);
				}
			ntfs_index_ctx_put(icx);
			}
		}
		ntfs_attr_put_search_ctx(actx);
	}
	return (ret);
}

/*
 *		Increment the count of subdirectories
 *		(excluding entries with a short name)
 */

static int nlink_increment(void *nlink_ptr,
			const ntfschar *name,
			const int len,
			const int type,
			const s64 pos,
			const MFT_REF mref,
			const unsigned int dt_type)
{
	if ((dt_type == NTFS_DT_DIR) && (type != FILE_NAME_DOS))
		(*((int*)nlink_ptr))++;
	return (0);
}

/*
 *		Compute the number of hard links according to Posix
 *	For a directory count the subdirectories whose name is not
 *		a short one, but count "." and ".."
 *	Otherwise count the names, excluding the short ones.
 *
 *	if there is an error, a null count is returned.
 */

int ntfs_dir_link_cnt(ntfs_inode *ni)
{
	ntfs_attr_search_ctx *actx;
	FILE_NAME_ATTR *fn;
	s64 pos;
	int err = 0;
	int nlink = 0;

	if (!ni) {
		ntfs_log_error("Invalid argument.\n");
		errno = EINVAL;
		goto err_out;
	}
	if (ni->nr_extents == -1)
		ni = ni->base_ni;
	if (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY) {
		/*
		 * Directory : scan the directory and count
		 * subdirectories whose name is not DOS-only.
		 * The directory names are ignored, but "." and ".."
		 * are taken into account.
		 */
		pos = 0;
		err = ntfs_readdir(ni, &pos, &nlink, nlink_increment);
		if (err)
			nlink = 0;
	} else {
		/*
		 * Non-directory : search for FILE_NAME attributes,
		 * and count those which are not DOS-only ones.
		 */
		actx = ntfs_attr_get_search_ctx(ni, NULL);
		if (!actx)
			goto err_out;
		while (!(err = ntfs_attr_lookup(AT_FILE_NAME, AT_UNNAMED, 0,
					CASE_SENSITIVE, 0, NULL, 0, actx))) {
			fn = (FILE_NAME_ATTR*)((u8*)actx->attr +
					le16_to_cpu(actx->attr->value_offset));
			if (fn->file_name_type != FILE_NAME_DOS)
				nlink++;
		}
		if (err && (errno != ENOENT))
			nlink = 0;
		ntfs_attr_put_search_ctx(actx);
	}
	if (!nlink)
		ntfs_log_perror("Failed to compute nlink of inode %lld",
			(long long)ni->mft_no);
err_out :
	return (nlink);
}
