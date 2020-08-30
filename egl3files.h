#ifndef _EGL3_FILES_H_
#define _EGL3_FILES_H_

#include "types.h"
#include "runlist.h"

struct EGL3File {
    const char* name;
    s64 size;
    u8 is_directory;
    s64 parent_index;
    runlist* o_runlist;
    INDEX_ALLOCATION* o_index_block;
};

static struct EGL3File EGL3Files[] = {
    { "test folder", 0, 1, -1, NULL, NULL },
	{ "testfile.txt", 500*1024*1024, 0, 0, NULL, NULL },
    { "test.folder", 0, 1, 0, NULL, NULL },
};

static const int EGL3FileCount = 3;

#endif