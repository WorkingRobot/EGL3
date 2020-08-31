#include "egl3interface.h"

extern "C" uint8_t* ntfs_device_win32_get_sector(void* ctx, int64_t sector_addr);

extern "C" int64_t* ntfs_device_win32_get_position(void* ctx);
extern "C" uint64_t* ntfs_device_win32_get_written_bytes(void* ctx);

extern "C" void* ntfs_device_win32_create_ctx();

uint8_t* ntfs_device_win32_get_sector(void* c, int64_t sector_addr) {
	auto ctx = (AppendingFile*)c;
	auto search = ctx->data.find(sector_addr);
	if (search != ctx->data.end()) {
		return search->second;
	}
	auto ret = (uint8_t*)calloc(1, 512);
	ctx->data.emplace(sector_addr, ret);

	return ret;
}

int64_t* ntfs_device_win32_get_position(void* ctx) {
	return &((AppendingFile*)ctx)->position;
}

uint64_t* ntfs_device_win32_get_written_bytes(void* ctx) {
	return &((AppendingFile*)ctx)->written_bytes;
}

void* ntfs_device_win32_create_ctx() {
	return new AppendingFile;
}