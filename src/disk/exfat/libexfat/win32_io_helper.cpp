#include "../../interface.h"

void* win32_open_ro(void* input) {
    if (input) {
        return input;
    }
    return new AppendingFile;
}

void* win32_open_rw(void* input) {
    if (input) {
        return input;
    }
    return new AppendingFile;
}

int win32_close(void* ctx) {
    return 0;
}

int64_t win32_lseek(void* ctx, int64_t offset, int whence) {
    return ((AppendingFile*)ctx)->seek(offset, whence);
}

size_t win32_pread(void* ctx, void* buf, size_t size, int64_t offset) {
    return ((AppendingFile*)ctx)->pread((uint8_t*)buf, size, offset);
}

size_t win32_pwrite(void* ctx, const void* buf, size_t size, int64_t offset) {
    return ((AppendingFile*)ctx)->pwrite((const uint8_t*)buf, size, offset);
}

size_t win32_read(void* ctx, void* buf, size_t size) {
    return ((AppendingFile*)ctx)->read((uint8_t*)buf, size);
}

size_t win32_write(void* ctx, const void* buf, size_t size) {
    return ((AppendingFile*)ctx)->write((const uint8_t*)buf, size);
}

extern "C" {

#include "device.h"

    struct io_ops win32_io_ops = {
        .open_ro = win32_open_ro,
        .open_rw = win32_open_rw,
        .close = win32_close,
        .lseek = win32_lseek,
        .read = win32_read,
        .write = win32_write,
        .pread = win32_pread,
        .pwrite = win32_pwrite
    };
}