#include "../ntfs/egl3interface.h"
#include "../utils/assert.h"
#include "../utils/align.h"
#include "../utils/open_browser.h"
#include "../web/epic/auth/device_code.h"
#include "../web/epic/auth/token_to_token.h"
#include "../web/epic/epic_client.h"

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
#undef ERROR_SUCCESS

	static const cpr::Authentication AuthClientSwitch{ "5229dcd3ac3845208b496649092f251b", "e3bd2d3e-bf8c-4857-9e7d-f3d947d220c7" };
	static const cpr::Authentication AuthClientLauncher{ "34a02cf8f4414e29b15921876da36f9a", "daafbccc737745039dffe53d94fc76cf" };

	AuthDeviceCode Login(AuthClientSwitch);
	auto bRet = Login.GetBrowserUrlFuture().get();
	if (bRet != AuthDeviceCode::ERROR_SUCCESS) {
		// couldn't get the browser url
		return 0;
	}
	OpenInBrowser(Login.GetBrowserUrl());

	bRet = Login.GetOAuthResponseFuture().get();
	if (bRet != AuthDeviceCode::ERROR_SUCCESS) {
		// couldn't get oauth data, probably expired
		return 0;
	}

	AuthTokenToToken Login2(AuthClientLauncher, Login.GetOAuthResponse()["access_token"].GetString());

	auto bRet2 = Login2.GetOAuthResponseFuture().get();
	if (bRet != AuthTokenToToken::ERROR_SUCCESS) {
		// couldn't get new launcher token
		return 0;
	}

	EpicClient client(Login2.GetOAuthResponse(), AuthClientLauncher);

	auto accData = client.GetAccount();
	printf("Hello %s (%s %s)!\n", accData->DisplayName.c_str(), accData->Name.c_str(), accData->LastName.c_str());
	auto assets = client.GetAssets("Windows", "Live");
	std::vector<std::future<void>> Futures;
	for (auto& asset : assets->Assets) {
		Futures.emplace_back(std::async([&, asset]() {
			auto catalogInfo = client.GetCatalogItems(asset.Namespace, { asset.CatalogItemId }, "US", "en");
			auto& catalogMap = catalogInfo->Items;
			auto catalogItr = catalogMap.find(asset.CatalogItemId);
			if (catalogItr != catalogMap.end()) {
				auto& catalogItem = catalogItr->second;
				printf("%s by %s\n", catalogItem.Title.c_str(), catalogItem.Developer.c_str());
			}
			else {
				printf("COULD NOT FIND %s\n", asset.CatalogItemId.c_str());
			}
		}));
	}

	for (auto& fut : Futures) {
		fut.wait();
	}
	//auto response3 = client.GetCatalogItems(response2->Assets[0].Namespace, { response2->Assets[0].CatalogItemId }, "US", "en");

	//printf("Hello %s (%s %s)!\n", response->DisplayName.c_str(), response->Name.c_str(), response->LastName.c_str());
	
	std::this_thread::sleep_for(std::chrono::hours(99));

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
		{
			uint8_t* p = (uint8_t*)&Params.Guid;
			srand(time(NULL));
			for (int i = 0; i < sizeof(GUID); i++) {
				p[i] = (uint8_t)(rand() & 0xFF);
				if (i == 7)
					p[7] = (p[7] & 0x0F) | 0x40;
				if (i == 8)
					p[8] = (p[8] & 0x3F) | 0x80;
			}
		}
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