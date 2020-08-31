#include "../ntfs/egl3interface.h"

#include <winspd/winspd.h>

static uint8_t* MBRData;
static EGL3File EGL3Files[] = {
	{ "test folder", 0, 1, -1, NULL, NULL },
	{ "testfile.txt", 1024*1024*1024*4llu, 0, 0, NULL, NULL },
	{ "test.folder", 0, 1, 0, NULL, NULL },
};
static constexpr int EGL3FileCount = 3;

__forceinline void HandleFileBlock(EGL3File* File, UINT64 BlockAddress, PVOID Buffer) {
	*(uint64_t*)Buffer = BlockAddress;
}

__forceinline void HandleBlock(AppendingFile* Data, UINT64 BlockAddress, PVOID Buffer) {
	if (BlockAddress < 8) { // MBR cluster
		if (BlockAddress == 0) {
			memcpy(Buffer, MBRData, 512);
		}
		else {
			memset(Buffer, 0, 512);
		}
		return;
	}

	// NTFS filesystem ignores the MBR cluster
	BlockAddress -= 8;
	int64_t lcn = BlockAddress >> 3; // divide by 8
	for (int n = 0; n < EGL3FileCount; ++n) {
		auto& file = EGL3Files[n];
		if (file.is_directory) {
			continue;
		}
		if (file.o_runlist->lcn <= lcn && file.o_runlist->lcn + file.o_runlist->length > lcn) {
			HandleFileBlock(&file, lcn - file.o_runlist->lcn, Buffer);
			return;
		}
	}

	auto search = Data->data.find(BlockAddress);
	if (search != Data->data.end()) {
		memcpy(Buffer, search->second, 512);
		return;
	}
	if (Data->data_ff.contains(BlockAddress)) {
		memset(Buffer, 255, 512);
		return;
	}
}

static BOOLEAN Read(SPD_STORAGE_UNIT* StorageUnit,
	PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, BOOLEAN FlushFlag,
	SPD_STORAGE_UNIT_STATUS* Status)
{
	memset(Buffer, 0, BlockCount * 512llu);
	for (UINT64 i = 0; i < BlockCount; ++i) {
		HandleBlock((AppendingFile*)StorageUnit->UserContext, BlockAddress + i, ((UINT8*)Buffer) + i * 512llu);
	}
	return TRUE;
}

static SPD_STORAGE_UNIT_INTERFACE DiskInterface =
{
	Read,
	0, // write
	0, // flush
	0, // unmap
};

int main(int argc, char* argv[])
{
	constexpr uint64_t disk_size_mb = 1024 * 1024 - 1;
	static_assert(disk_size_mb * 2048 < INT_MAX, "MBR and basically everything else requires disk size to be below 2**31 clusters");

	// Create MBR
	MBRData = (uint8_t*)malloc(512);
	SPD_PARTITION Partition = {
		.Type = 7,
		.BlockAddress = 8,
		.BlockCount = disk_size_mb * 2048ull - 8 // 2048 sectors per mb, 8 taken for mbr record
	};
	if (SpdDefinePartitionTable(&Partition, 1, MBRData)) {
		printf("MBR error lol\n");
		return 0;
	}
	*(uint32_t*)(MBRData + 440) = 0x334C4745;

	// Create NTFS data
	AppendingFile* Disk;
	if (EGL3CreateDisk(Partition.BlockCount, NULL, EGL3Files, EGL3FileCount, (void**)&Disk)) {
		printf("create ntfs error\n");
		return 0;
	}

	// Mount disk
	SpdDebugLogSetHandle((HANDLE)STD_ERROR_HANDLE);
	SPD_STORAGE_UNIT* Unit;
	DWORD Error;
	{
		SPD_STORAGE_UNIT_PARAMS Params = { 0 };
		UuidCreate(&Params.Guid); // windows function
		Params.BlockCount = disk_size_mb * 2048; // 2048 * 512 bytes = 1 mb
		Params.BlockLength = 512; // should stay at this
		strcpy((char*)Params.ProductId, "EGL3");
		strcpy((char*)Params.ProductRevisionLevel, "0.1");
		Params.WriteProtected = 1;
		Params.CacheSupported = 1;
		Params.UnmapSupported = 1;
		Params.EjectDisabled = 1; // disables eject ui :)
		// https://social.msdn.microsoft.com/Forums/WINDOWS/en-US/6223c501-f55a-4df3-a148-df12d8032c7b#ec8f32d4-44ea-4523-9401-e7c8c1f19fed
		// Since its inception USBStor.sys specifies 0x10000 for MaximumTransferLength
		// Best to stay at 64kb, but some have gone to 128kb
		Params.MaxTransferLength = 0x10000; // 64 * 1024

		if (Error = SpdStorageUnitCreate(NULL, &Params, &DiskInterface, &Unit)) {
			printf("create disk error: %d\n", Error);
			return 0;
		}
		Unit->UserContext = Disk;
	}

	//SpdStorageUnitSetDebugLog(Unit, -1); // print everything

	if (Error = SpdStorageUnitStartDispatcher(Unit, 0)) {
		printf("start dispatcher error: %d\n", Error);
		return 0;
	}

	printf("started\n");
	Sleep(INFINITE);

	return 0;
}