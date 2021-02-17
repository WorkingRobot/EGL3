#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct _EGL3Run EGL3Run;
typedef struct _EGL3File EGL3File;

struct _EGL3Run {
    uint32_t idx;
    uint32_t count;
};

struct _EGL3File {
    char path[256]; // Must be null terminated
    uint64_t size;
    void* user_context;
    EGL3Run runs[16];
};

#define DISK_SIZE 1024ull * 1024 * 1024 * 100

// true = success
// false = failed
bool EGL3CreateDisk(uint64_t sector_count, const char* label, const EGL3File files[], uint32_t file_count, void** o_data);

#ifdef __cplusplus
}

#include <unordered_map>
#include <unordered_set>

// o_data
struct AppendingFile {
    std::unordered_map<uint64_t, uint8_t*> data;
    uint32_t operation_count;
    uint64_t written_bytes;
    int64_t position;

    AppendingFile() :
        operation_count(),
        written_bytes(),
        position()
    {

    }
};

#endif