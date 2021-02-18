#pragma once

#include <stdint.h>

struct io_ops {
    // return NULL or a pointer
    void* (*open_ro)(void* input);
    void* (*open_rw)(void* input);

    // return 0 for success, anything else otherwise
    int (*close)(void* ctx);

    // works like standard posix functions
    int64_t(*lseek)(void* ctx, int64_t offset, int whence);
    size_t(*read)(void* ctx, void* buf, size_t size);
    size_t(*write)(void* ctx, const void* buf, size_t size);
    size_t(*pread)(void* ctx, void* buf, size_t size, int64_t offset);
    size_t(*pwrite)(void* ctx, const void* buf, size_t size, int64_t offset);
};