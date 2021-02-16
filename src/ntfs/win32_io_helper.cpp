#include "egl3interface.h"

#include "types.h"

extern "C" {
    uint8_t* ntfs_device_win32_get_cluster(void* ctx, int64_t cluster_addr);

    BOOL ntfs_device_win32_get_cluster_FF(void* ctx, int64_t cluster_addr);
    void ntfs_device_win32_set_cluster_FF(void* ctx, int64_t cluster_addr, BOOL is_ff);

    int64_t* ntfs_device_win32_get_position(void* ctx);
    uint64_t* ntfs_device_win32_get_written_bytes(void* ctx);

    void* ntfs_device_win32_create_ctx(const char* name);
}

uint8_t* ntfs_device_win32_get_cluster(void* c, int64_t cluster_addr) {
    auto ctx = (AppendingFile*)c;
    auto search = ctx->data.find(cluster_addr);
    if (search != ctx->data.end()) {
        return search->second;
    }
    auto ret = (uint8_t*)calloc(1, 4096);
    ctx->data.emplace(cluster_addr, ret);

    return ret;
}

BOOL ntfs_device_win32_get_cluster_FF(void* c, int64_t cluster_addr) {
    auto ctx = (AppendingFile*)c;
    return (BOOL)ctx->data_ff.contains(cluster_addr);
}

void ntfs_device_win32_set_cluster_FF(void* c, int64_t cluster_addr, BOOL is_ff) {
    auto ctx = (AppendingFile*)c;
    if (is_ff) {
        ctx->data.erase(cluster_addr);
        ctx->data_ff.emplace(cluster_addr);
    }
    else {
        ctx->data_ff.erase(cluster_addr);
    }
}

int64_t* ntfs_device_win32_get_position(void* ctx) {
    return &((AppendingFile*)ctx)->position;
}

uint64_t* ntfs_device_win32_get_written_bytes(void* ctx) {
    return &((AppendingFile*)ctx)->written_bytes;
}

void* ntfs_device_win32_create_ctx(const char* name) {
    if (name) {
        return (void*)name;
    }
    return new AppendingFile;
}