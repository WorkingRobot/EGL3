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

#define DISK_SIZE 1024ull * 1024 * 1024 * 64
#define DISK_SIZE_PARTITION DISK_SIZE - 4096 // -1 cluster for the MBR

// true = success
// false = failed
bool EGL3CreateDisk(const char* label, const EGL3File files[], uint32_t file_count, void** o_data);

#ifdef __cplusplus
}

#include <unordered_map>

// o_data
class AppendingFile {
public:
    AppendingFile();

    int64_t seek(int64_t offset, int whence);

    size_t tell();

    size_t pread(uint8_t* buf, size_t size, int64_t offset);

    size_t pwrite(const uint8_t* buf, size_t size, int64_t offset);

    size_t read(uint8_t* buf, size_t size);

    size_t write(const uint8_t* buf, size_t size);

    uint8_t* try_get_cluster(int64_t cluster_idx);

    uint8_t* get_cluster(int64_t cluster_idx);

    const std::unordered_map<uint64_t, uint8_t*>& get_data() const;

private:
    std::unordered_map<uint64_t, uint8_t*> data;
    size_t position;
    static constexpr size_t size = DISK_SIZE_PARTITION;
};

#endif