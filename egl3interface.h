#ifndef _EGL3_INTERFACE_H_
#define _EGL3_INTERFACE_H_

#include "runlist.h"

struct EGL3File {
    const char* name;
    s64 size;
    u8 is_directory;
    s64 parent_index;
    runlist* o_runlist;
    INDEX_ALLOCATION* o_index_block;
};

struct EGL3WrittenDiskData {
    u64 offset;
    u64 size;
    u8* data;
};

void EGL3CreateDisk(u64 disk_size, const struct EGL3File files[], u32 file_count, struct EGL3WrittenDiskData** o_data, u32* o_data_count);

#endif