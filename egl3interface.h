#ifndef _EGL3_INTERFACE_H_
#define _EGL3_INTERFACE_H_

#include "runlist.h"

typedef struct _AppendingFileOperation AppendingFileOperation;
typedef struct _AppendingFile AppendingFile;
typedef struct _EGL3File EGL3File;

struct _EGL3File {
    const char* name;
    s64 size;
    u8 is_directory;
    s64 parent_index;
    runlist* o_runlist;
    INDEX_ALLOCATION* o_index_block;
};

struct _AppendingFileOperation {
	u64 offset;
	u64 size;
	u8* data;

	AppendingFileOperation* next;
};

struct _AppendingFile {
	AppendingFileOperation* first;
	AppendingFileOperation* last;
	u32 count;
	u64 written_bytes;
	s64 position;
};

// defined at the end of mkntfs.c
void EGL3CreateDisk(u64 sector_size, const EGL3File files[], u32 file_count, AppendingFile** o_data);

#endif