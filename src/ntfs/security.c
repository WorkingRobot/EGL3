/**
 * security.c - Handling security/ACLs in NTFS.  Originated from the Linux-NTFS project.
 *
 * Copyright (c) 2004 Anton Altaparmakov
 * Copyright (c) 2005-2006 Szabolcs Szakacsits
 * Copyright (c) 2006 Yura Pakhuchiy
 * Copyright (c) 2007-2015 Jean-Pierre Andre
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
#include <time.h>

#include "inode.h"
#include "index.h"

struct SII {		/* this is an image of an $SII index entry */
	le16 offs;
	le16 size;
	le32 fill1;
	le16 indexsz;
	le16 indexksz;
	le16 flags;
	le16 fill2;
	le32 keysecurid;

	/* did not find official description for the following */
	le32 hash;
	le32 securid;
	le32 dataoffsl;	/* documented as badly aligned */
	le32 dataoffsh;
	le32 datasize;
} ;

struct SDH {		/* this is an image of an $SDH index entry */
	le16 offs;
	le16 size;
	le32 fill1;
	le16 indexsz;
	le16 indexksz;
	le16 flags;
	le16 fill2;
	le32 keyhash;
	le32 keysecurid;

	/* did not find official description for the following */
	le32 hash;
	le32 securid;
	le32 dataoffsl;
	le32 dataoffsh;
	le32 datasize;
	le32 fill3;
} ;

/**
 * ntfs_generate_guid - generatates a random current guid.
 * @guid:	[OUT]   pointer to a GUID struct to hold the generated guid.
 *
 * perhaps not a very good random number generator though...
 */
void ntfs_generate_guid(GUID* guid)
{
	unsigned int i;
	u8* p = (u8*)guid;

	/* this is called at most once from mkntfs */
	srand(time((time_t*)NULL) ^ (getpid() << 16));
	for (i = 0; i < sizeof(GUID); i++) {
		p[i] = (u8)(rand() & 0xFF);
		if (i == 7)
			p[7] = (p[7] & 0x0F) | 0x40;
		if (i == 8)
			p[8] = (p[8] & 0x3F) | 0x80;
	}
}

/*
 *	Close the volume's security descriptor index ($Secure)
 *
 *	returns  0 if it succeeds
 *		-1 with errno set if it fails
 */
int ntfs_close_secure(ntfs_volume* vol)
{
	int res = 0;

	if (vol->secure_ni) {
		ntfs_index_ctx_put(vol->secure_xsdh);
		ntfs_index_ctx_put(vol->secure_xsii);
		res = ntfs_inode_close(vol->secure_ni);
		vol->secure_ni = NULL;
	}
	return res;
}
