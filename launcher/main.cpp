#include "../ntfs/egl3interface.h"
#include "../utils/stringex/stringex.h"

#include <chrono>
namespace ch = std::chrono;
#include <winspd/winspd.h>

typedef ch::steady_clock timer;
#define START_TIMER(name) auto timer_##name = timer::now()
#define STOP_TIMER(name) \
{ \
	auto timer_stop_##name = timer::now(); \
	printf("Timer " #name ": %.02f ms\n", (timer_stop_##name - timer_##name).count() / 1000000.); \
}

static uint8_t* MBRData;
static EGL3File EGL3Files[] = {
	{ "test folder", 0, 1, -1, NULL, NULL },
	{ "testfile.txt", 1024*1024*1024*1023llu, 0, 0, NULL, NULL },
	{ "test.folder", 0, 1, 0, NULL, NULL },
};
static constexpr int EGL3FileCount = sizeof(EGL3Files) / sizeof(EGL3File);

__forceinline void HandleFileCluster(EGL3File* File, UINT64 LCN, UINT8 Buffer[4096]) {
	memset(Buffer, 'A', 1024);
	memset(Buffer + 1024, 'B', 1024);
	memset(Buffer + 2048, 'C', 1024);
	memset(Buffer + 3072, 'D', 1024);
	*(uint64_t*)Buffer = LCN;
}

__forceinline void HandleCluster(AppendingFile* Data, UINT64 LCN, UINT8 Buffer[4096]) {
	if (!LCN) { // MBR cluster
		memcpy(Buffer, MBRData, 512);
		return;
	}

	// NTFS filesystem ignores the MBR cluster
	LCN--;

	for (int n = 0; n < EGL3FileCount; ++n) {
		auto& file = EGL3Files[n];
		if (file.is_directory) {
			continue;
		}
		auto runlist = file.o_runlist;
		while (runlist->length) {
			if (runlist->lcn <= LCN && runlist->lcn + runlist->length > LCN) {
				HandleFileCluster(&file, LCN - runlist->lcn + runlist->vcn, Buffer);
				return;
			}
			runlist++;
		}
	}

	auto search = Data->data.find(LCN);
	if (search != Data->data.end()) {
		memcpy(Buffer, search->second, 4096);
		return;
	}
	if (Data->data_ff.contains(LCN)) {
		memset(Buffer, 255, 4096);
		return;
	}
}

template<int Alignment, class N>
static constexpr N Align(N Value) {
	return Value + (-Value & (Alignment - 1));
}

static BOOLEAN Read(SPD_STORAGE_UNIT* StorageUnit,
	PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, BOOLEAN FlushFlag,
	SPD_STORAGE_UNIT_STATUS* Status)
{
	memset(Buffer, 0, BlockCount * 512llu);
	UINT8 ClusterBuffer[4096];
	auto StartCluster = BlockAddress >> 3; // 8 sectors = 1 cluster, we apply alignment downwards
	auto ThrowawaySectorCount = BlockAddress & 7; // number of sectors to throw away from the beginning
	auto TotalClusterCount = Align<8>(ThrowawaySectorCount + BlockCount) >> 3;
	for (UINT64 i = 0; i < TotalClusterCount; ++i) {
		memset(ClusterBuffer, 0, 4096);
		HandleCluster((AppendingFile*)StorageUnit->UserContext, StartCluster + i, ClusterBuffer);
		if (i == 0) {
			memcpy(Buffer, ClusterBuffer + ThrowawaySectorCount * 512llu, min(BlockCount * 512llu, 4096 - ThrowawaySectorCount * 512llu));
		}
		else {
			memcpy(((UINT8*)Buffer) + i * 4096llu - ThrowawaySectorCount * 512llu, ClusterBuffer, min(BlockCount * 512llu, 4096));
		}
		BlockCount -= 8;
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
	for (int i = 0; i < 500; ++i) {
		StringEx::Evaluate("++Fortnite+Release-2.4.x-CL-3846605-Platform", "Regex(\\+\\+Fortnite\\+Release-(.*)-CL-(\\d+)-.*) && RegexGroupInt64(2) < 4016789 && ((RegexGroupString(1) == \"Prep\" && RegexGroupInt64(2) >= 3779789) || (RegexGroupString(1) == \"Next\" && RegexGroupInt64(2) >= 3779794) || (RegexGroupString(1) == \"Cert\" && RegexGroupInt64(2) >= 3785892) || (RegexGroupInt64(2) >= 3846604))");
	}
	START_TIMER(StringEx);
	for (int i = 0; i < 50000; ++i) {
		StringEx::Evaluate("++Fortnite+Release-2.4.x-CL-3846605-Platform", "Regex(\\+\\+Fortnite\\+Release-(.*)-CL-(\\d+)-.*) && RegexGroupInt64(2) < 4016789 && ((RegexGroupString(1) == \"Prep\" && RegexGroupInt64(2) >= 3779789) || (RegexGroupString(1) == \"Next\" && RegexGroupInt64(2) >= 3779794) || (RegexGroupString(1) == \"Cert\" && RegexGroupInt64(2) >= 3785892) || (RegexGroupInt64(2) >= 3846604))");
	}
	STOP_TIMER(StringEx);

	return 0;
	constexpr uint64_t disk_size_mb = 1024 * 1024 - 1; // max: 1024 * 1024 - 1
	static_assert(disk_size_mb * 2048 < INT_MAX, "MBR and basically everything else requires disk size to be below 2**31 clusters");

	// Create MBR
	START_TIMER(MBR);
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
	STOP_TIMER(MBR);

	// Create NTFS data
	START_TIMER(NTFS);
	AppendingFile* Disk;
	if (EGL3CreateDisk(Partition.BlockCount, "Fortnite Season 15 Leaks", EGL3Files, EGL3FileCount, (void**)&Disk)) {
		printf("create ntfs error\n");
		return 0;
	}
	STOP_TIMER(NTFS);

	// Mount disk
	START_TIMER(Disk);
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
		Params.EjectDisabled = 1; // disables eject ui
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
	STOP_TIMER(Disk);

	//SpdStorageUnitSetDebugLog(Unit, -1); // print everything

	START_TIMER(Dispatcher);
	if (Error = SpdStorageUnitStartDispatcher(Unit, 0)) {
		printf("start dispatcher error: %d\n", Error);
		return 0;
	}
	STOP_TIMER(Dispatcher);

	printf("started\n");
	Sleep(INFINITE);

	return 0;
}