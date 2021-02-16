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
#include <errno.h>
#include <sys/stat.h>

#include "security.h"
#include "inode.h"
#include "index.h"
#include "dir.h"
#include "cache.h"
#include "misc.h"
#include "winsd.h"

struct SII {        /* this is an image of an $SII index entry */
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
    le32 dataoffsl; /* documented as badly aligned */
    le32 dataoffsh;
    le32 datasize;
} ;

struct SDH {        /* this is an image of an $SDH index entry */
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

/*
 *	A few useful constants
 */

static ntfschar sii_stream[] = { const_cpu_to_le16('$'),
                 const_cpu_to_le16('S'),
                 const_cpu_to_le16('I'),
                 const_cpu_to_le16('I'),
                 const_cpu_to_le16(0) };
static ntfschar sdh_stream[] = { const_cpu_to_le16('$'),
                 const_cpu_to_le16('S'),
                 const_cpu_to_le16('D'),
                 const_cpu_to_le16('H'),
                 const_cpu_to_le16(0) };

/**
 * ntfs_generate_guid - generatates a random current guid.
 * @guid:   [OUT]   pointer to a GUID struct to hold the generated guid.
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
 *		Check whether user can create a file (or directory)
 *
 *	Returns TRUE if access is allowed,
 *	Also returns the gid and dsetgid applicable to the created file
 */

int ntfs_allowed_create(struct SECURITY_CONTEXT* scx,
    ntfs_inode* dir_ni, gid_t* pgid, mode_t* pdsetgid)
{
    int perm;
    int res;
    int allow;
    struct stat stbuf;

    *pgid = 0;
    *pdsetgid = 0777;
    return TRUE;

    ///*
    // * Always allow for root.
    // * Also always allow if no mapping has been defined
    // */
    //if (!scx->mapping[MAPUSERS])
    //    perm = 0777;
    //else
    //    perm = ntfs_get_perm(scx, dir_ni, S_IWRITE + S_IEXEC);
    //if (!scx->mapping[MAPUSERS]
    //    || !scx->uid) {
    //    allow = 1;
    //}
    //else {
    //    perm = ntfs_get_perm(scx, dir_ni, S_IWRITE + S_IEXEC);
    //    if (perm >= 0) {
    //        res = EACCES;
    //        allow = ((perm & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0)
    //            && ((perm & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0);
    //        if (!allow)
    //            errno = res;
    //    }
    //    else
    //        allow = 0;
    //}
    //*pgid = scx->gid;
    //*pdsetgid = 0;
    ///* return directory group if S_ISGID is set */
    //if (allow && (perm & S_ISGID)) {
    //    if (ntfs_get_owner_mode(scx, dir_ni, &stbuf) >= 0) {
    //        *pdsetgid = stbuf.st_mode & S_ISGID;
    //        if (perm & S_ISGID)
    //            *pgid = stbuf.st_gid;
    //    }
    //}
    //return (allow);
}

/*
 *		Get an inherited security id
 *
 *	For Windows compatibility, the normal initial permission setting
 *	may be inherited from the parent directory instead of being
 *	defined by the creation arguments.
 *
 *	The following creates an inherited id for that purpose.
 *
 *	Note : the owner and group of parent directory are also
 *	inherited (which is not the case on Windows) if no user mapping
 *	is defined.
 *
 *	Returns the inherited id, or zero if not possible (eg on NTFS 1.x)
 */

le32 ntfs_inherited_id(struct SECURITY_CONTEXT* scx,
    ntfs_inode* dir_ni, BOOL fordir)
{
    struct CACHED_PERMISSIONS* cached;
    char* parentattr;
    le32 securid;

    return 0;
    
    //securid = const_cpu_to_le32(0);
    //cached = (struct CACHED_PERMISSIONS*)NULL;
    ///*
    // * Try to get inherited id from cache, possible when
    // * the current process owns the parent directory
    // */
    //if (test_nino_flag(dir_ni, v3_Extensions)
    //    && dir_ni->security_id) {
    //    cached = fetch_cache(scx, dir_ni);
    //    if (cached
    //        && (cached->uid == scx->uid) && (cached->gid == scx->gid))
    //        securid = (fordir ? cached->inh_dirid
    //            : cached->inh_fileid);
    //}
    ///*
    // * Not cached or not available in cache, compute it all
    // * Note : if parent directory has no id, it is not cacheable
    // */
    //if (!securid) {
    //    parentattr = getsecurityattr(scx->vol, dir_ni);
    //    if (parentattr) {
    //        securid = build_inherited_id(scx,
    //            parentattr, fordir);
    //        free(parentattr);
    //        /*
    //         * Store the result into cache for further use
    //         * if the current process owns the parent directory
    //         */
    //        if (securid) {
    //            cached = fetch_cache(scx, dir_ni);
    //            if (cached
    //                && (cached->uid == scx->uid)
    //                && (cached->gid == scx->gid)) {
    //                if (fordir)
    //                    cached->inh_dirid = securid;
    //                else
    //                    cached->inh_fileid = securid;
    //            }
    //        }
    //    }
    //}
    //return (securid);
}

/*
 *	Create a default security descriptor for files whose descriptor
 *	cannot be inherited
 */

int ntfs_sd_add_everyone(ntfs_inode* ni)
{
#if 1
    /* JPA SECURITY_DESCRIPTOR_ATTR *sd; */
    SECURITY_DESCRIPTOR_RELATIVE* sd;
    ACL* acl;
    ACCESS_ALLOWED_ACE* ace;
    SID* sid;
    int ret, sd_len;

    /* Create SECURITY_DESCRIPTOR attribute (everyone has full access). */
    /*
     * Calculate security descriptor length. We have 2 sub-authorities in
     * owner and group SIDs, but structure SID contain only one, so add
     * 4 bytes to every SID.
     */
    sd_len = sizeof(SECURITY_DESCRIPTOR_ATTR) + 2 * (sizeof(SID) + 4) +
        sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE);
    sd = (SECURITY_DESCRIPTOR_RELATIVE*)ntfs_calloc(sd_len);
    if (!sd)
        return -1;

    sd->revision = SECURITY_DESCRIPTOR_REVISION;
    sd->control = SE_DACL_PRESENT | SE_SELF_RELATIVE;

    sid = (SID*)((u8*)sd + sizeof(SECURITY_DESCRIPTOR_ATTR));
    sid->revision = SID_REVISION;
    sid->sub_authority_count = 2;
    sid->sub_authority[0] = const_cpu_to_le32(SECURITY_BUILTIN_DOMAIN_RID);
    sid->sub_authority[1] = const_cpu_to_le32(DOMAIN_ALIAS_RID_ADMINS);
    sid->identifier_authority.value[5] = 5;
    sd->owner = cpu_to_le32((u8*)sid - (u8*)sd);

    sid = (SID*)((u8*)sid + sizeof(SID) + 4);
    sid->revision = SID_REVISION;
    sid->sub_authority_count = 2;
    sid->sub_authority[0] = const_cpu_to_le32(SECURITY_BUILTIN_DOMAIN_RID);
    sid->sub_authority[1] = const_cpu_to_le32(DOMAIN_ALIAS_RID_ADMINS);
    sid->identifier_authority.value[5] = 5;
    sd->group = cpu_to_le32((u8*)sid - (u8*)sd);

    acl = (ACL*)((u8*)sid + sizeof(SID) + 4);
    acl->revision = ACL_REVISION;
    acl->size = const_cpu_to_le16(sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE));
    acl->ace_count = const_cpu_to_le16(1);
    sd->dacl = cpu_to_le32((u8*)acl - (u8*)sd);

    ace = (ACCESS_ALLOWED_ACE*)((u8*)acl + sizeof(ACL));
    ace->type = ACCESS_ALLOWED_ACE_TYPE;
    ace->flags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
    ace->size = const_cpu_to_le16(sizeof(ACCESS_ALLOWED_ACE));
    ace->mask = const_cpu_to_le32(0x1f01ff); /* FIXME */
    ace->sid.revision = SID_REVISION;
    ace->sid.sub_authority_count = 1;
    ace->sid.sub_authority[0] = const_cpu_to_le32(0);
    ace->sid.identifier_authority.value[5] = 1;

    ret = ntfs_attr_add(ni, AT_SECURITY_DESCRIPTOR, AT_UNNAMED, 0, (u8*)sd,
        sd_len);
    if (ret)
        ntfs_log_perror("Failed to add initial SECURITY_DESCRIPTOR");

    free(sd);
#else
    const u8* sd;
    const s64 sd_len;
    int ret;
    ret = create_sd_sddl("O:WDG:SYD:PAI(A;;0x1200a9;;;WD)", &sd, &sd_len);
    if (!ret)
        ret = ntfs_attr_add(ni, AT_SECURITY_DESCRIPTOR, AT_UNNAMED, 0, sd, sd_len);

    free_sd_sddl(sd);
#endif
    return ret;
}




/*
 *	Open the volume's security descriptor index ($Secure)
 *
 *	returns  0 if it succeeds
 *		-1 with errno set if it fails and the volume is NTFS v3.0+
 */
int ntfs_open_secure(ntfs_volume* vol)
{
    ntfs_inode* ni;
    ntfs_index_context* sii;
    ntfs_index_context* sdh;

    if (vol->secure_ni) /* Already open? */
        return 0;

    ni = ntfs_pathname_to_inode(vol, NULL, "$Secure");
    if (!ni)
        goto err;

    if (ni->mft_no != FILE_Secure) {
        ntfs_log_error("$Secure does not have expected inode number!");
        errno = EINVAL;
        goto err_close_ni;
    }

    /* Allocate the needed index contexts. */
    sii = ntfs_index_ctx_get(ni, sii_stream, 4);
    if (!sii)
        goto err_close_ni;

    sdh = ntfs_index_ctx_get(ni, sdh_stream, 4);
    if (!sdh)
        goto err_close_sii;

    vol->secure_xsdh = sdh;
    vol->secure_xsii = sii;
    vol->secure_ni = ni;
    return 0;

err_close_sii:
    ntfs_index_ctx_put(sii);
err_close_ni:
    ntfs_inode_close(ni);
err:
    /* Failing on NTFS pre-v3.0 is expected. */
    if (vol->major_ver < 3)
        return 0;
    ntfs_log_perror("Failed to open $Secure");
    return -1;
}

/*
 *  Close the volume's security descriptor index ($Secure)
 *
 *  returns  0 if it succeeds
 *      -1 with errno set if it fails
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
