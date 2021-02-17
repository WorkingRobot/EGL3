extern "C" {
    #include <stdint.h>
    #include <stdio.h>
    #include <stdbool.h>

    void* win32_helper_create_ctx(void* input);
    void win32_helper_flush_ctx(void* ctx);
    int64_t win32_helper_seek(void* ctx, int64_t offset, int whence);
    int64_t win32_helper_tell(void* ctx);
    uint8_t* win32_helper_get_cluster(void* ctx, int64_t cluster_addr);
}

#include "../../interface.h"

void* win32_helper_create_ctx(void* input)
{
    if (input) {
        return (void*)input;
    }
    return new AppendingFile;
}

void win32_helper_flush_ctx(void* ctx) {
    auto Ctx = (AppendingFile*)ctx;

}

int64_t win32_helper_seek(void* ctx, int64_t offset, int whence)
{
    auto Ctx = (AppendingFile*)ctx;

    if (whence == SEEK_SET) {
        return Ctx->position = offset;
    }
    else if (whence == SEEK_CUR) {
        return Ctx->position += offset;
    }
    else if (whence == SEEK_END) {
        // TODO: disk is 100 gb
        return Ctx->position = (DISK_SIZE - 4096) + offset;
    }

    return Ctx->position;
}

int64_t win32_helper_tell(void* ctx)
{
    auto Ctx = (AppendingFile*)ctx;

    return Ctx->position;
}

uint8_t* win32_helper_get_cluster(void* ctx, int64_t cluster_addr)
{
    auto Ctx = (AppendingFile*)ctx;

    auto search = Ctx->data.find(cluster_addr);
    if (search != Ctx->data.end()) {
        return search->second;
    }
    auto ret = (uint8_t*)calloc(1, 4096);
    Ctx->data.emplace(cluster_addr, ret);

    return ret;
}
