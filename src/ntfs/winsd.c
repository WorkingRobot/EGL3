#include <windows.h>
#include <sddl.h>
#include <errno.h>

#include "winsd.h"
#include "debug.h"

int create_sd_sddl(const char* sddl, u8** sd, s64* sd_len) {
    ULONG sdlen;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(sddl, SDDL_REVISION_1, sd, &sdlen)) {
        errno = EINVAL;
        return -errno;
    }
    *sd_len = sdlen;
    return 0;
}

int free_sd_sddl(const u8* sd) {
    LocalFree(sd);
}