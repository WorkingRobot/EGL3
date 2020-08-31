#ifndef _EGL3_INTERFACE_H_
#define _EGL3_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct _EGL3Runlist EGL3Runlist;
typedef struct _EGL3File EGL3File;

struct _EGL3Runlist {/* In memory vcn to lcn mapping structure element. */
	int64_t vcn;	/* vcn = Starting virtual cluster number. */
	int64_t lcn;	/* lcn = Starting logical cluster number. */
	int64_t length;	/* Run length in clusters. */
};

struct _EGL3File {
	const char* name;
	int64_t size;
	uint8_t is_directory;
	int64_t parent_index;
	EGL3Runlist* o_runlist;
	void* o_index_block;
};

// defined at the end of mkntfs.c
// 0: success, 1: error
int EGL3CreateDisk(uint64_t sector_size, const char* label, const EGL3File files[], uint32_t file_count, void** o_data);

#ifdef __cplusplus
}

// C++ code gets some more data
#include <unordered_map>

struct AppendingFile {
	std::unordered_map<uint64_t, uint8_t*> data;
	uint32_t operation_count;
	uint64_t written_bytes;
	int64_t position;
};

#endif

#endif