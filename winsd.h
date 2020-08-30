#ifndef _NTFS_WINSD_H_
#define _NTFS_WINSD_H_

#include "debug.h"

int create_sd_sddl(const char* sddl, const u8** sd, const s64* sd_len);

int free_sd_sddl(const u8* sd);

#endif