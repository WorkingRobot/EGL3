#pragma once

#include "systypes.h"

struct io_ops {
    // return NULL or a pointer
    void* (*open_ro)(void* input);
    void* (*open_rw)(void* input);

    // return 0 for success, anything else otherwise
    int (*close)(void* ctx);

    // works like standard posix functions
    off_t(*lseek)(void* ctx, off_t offset, int whence);
    ssize_t(*read)(void* ctx, void* buf, size_t size);
    ssize_t(*write)(void* ctx, const void* buf, size_t size);
    ssize_t(*pread)(void* ctx, void* buf, size_t size, off_t offset);
    ssize_t(*pwrite)(void* ctx, const void* buf, size_t size, off_t offset);
};