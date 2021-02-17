#include "device.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

void* win32_helper_create_ctx(void* input);
void win32_helper_flush_ctx(void* ctx);
int64_t win32_helper_seek(void* ctx, int64_t offset, int whence);
int64_t win32_helper_tell(void* ctx);
uint8_t* win32_helper_get_cluster(void* ctx, int64_t cluster_addr);

static void* win32_open_ro(void* input) {
    return win32_helper_create_ctx(input);
}

static void* win32_open_rw(void* input) {
    return win32_helper_create_ctx(input);
}

static int win32_close(void* ctx) {
    win32_helper_flush_ctx(ctx);
    return 0;
}

static off_t win32_lseek(void* ctx, off_t offset, int whence) {
    return win32_helper_seek(ctx, offset, whence);
}

#define min(a, b) ((a) < (b)) ? (a) : (b)

static ssize_t win32_pread(void* ctx, void* buf, size_t size, off_t offset) {
    size_t bytes_left = size;

    size_t cluster_idx = offset >> 12; // divide by 4096
    size_t cluster_off = offset & 4095;
    while (bytes_left) {
        int read_amt = min(4096 - cluster_off, bytes_left);
        if (read_amt != 4096) {
            memcpy(buf, win32_helper_get_cluster(ctx, cluster_idx) + cluster_off, read_amt);
        }
        else {
            memcpy(buf, win32_helper_get_cluster(ctx, cluster_idx) + cluster_off, read_amt);
        }
        (uint8_t*)buf += read_amt;
        cluster_off = 0;
        bytes_left -= read_amt;
        cluster_idx++;
    }

    return size;
}

static bool win32_is_cluster_00(const uint8_t* b) {
    for (int n = 0; n < 4096; ++n) {
        if (b[n] != 0x00) {
            return false;
        }
    }
    return true;
}

static ssize_t win32_pwrite(void* ctx, const void* buf, size_t size, off_t offset) {
    size_t bytes_left = size;

    size_t cluster_idx = offset >> 12; // divide by 4096
    size_t cluster_off = offset & 4095;
    while (bytes_left) {
        int read_amt = min(4096 - cluster_off, bytes_left);
        if (read_amt != 4096) {
            memcpy(win32_helper_get_cluster(ctx, cluster_idx) + cluster_off, buf, read_amt);
        }
        else {
            // it's 0 by default, don't do anything
            if (!win32_is_cluster_00(buf)) {
                memcpy(win32_helper_get_cluster(ctx, cluster_idx) + cluster_off, buf, read_amt);
            }
        }
        (uint8_t*)buf += read_amt;
        cluster_off = 0;
        bytes_left -= read_amt;
        cluster_idx++;
    }

    return size;
}

static ssize_t win32_read(void* ctx, void* buf, size_t size) {
    ssize_t ret = win32_pread(ctx, buf, size, win32_helper_tell(ctx));
    if (ret != -1)
        win32_helper_seek(ctx, ret, SEEK_CUR);
    return ret;
}

static ssize_t win32_write(void* ctx, const void* buf, size_t size) {
    ssize_t ret = win32_pwrite(ctx, buf, size, win32_helper_tell(ctx));
    if (ret != -1)
        win32_helper_seek(ctx, ret, SEEK_CUR);
    return ret;
}

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