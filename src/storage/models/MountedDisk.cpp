#include "MountedDisk.h"

#include "../../disk/interface.h"
#include "../../utils/Align.h"
#include "../../utils/Assert.h"
#include "../../utils/Random.h"

#include <memory>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winspd/winspd.h>

namespace EGL3::Storage::Models {
    static constexpr uint32_t SectorSize = 512;
    static constexpr uint32_t DiskSize = DISK_SIZE / (1 << 20);
    static constexpr uint32_t DiskSignature = 0x334C4745; // "EGL3"

    static constexpr uint32_t SectorsPerMegabyte = (1 << 20) / SectorSize;

    struct MountedData {
        uint8_t MBRData[512];
        std::vector<EGL3File> Files;
        AppendingFile* Disk;
        SPD_STORAGE_UNIT* Unit;
        SPD_GUARD CloseGuard;
        const decltype(MountedDisk::HandleFileCluster)& FileClusterCallback;

        MountedData(const decltype(MountedDisk::HandleFileCluster)& Callback) :
            MBRData{},
            Disk(nullptr),
            Unit(nullptr),
            CloseGuard(SPD_GUARD_INIT),
            FileClusterCallback(Callback)
        {

        }
    };

    static std::allocator<MountedData> DataAllocator;

    MountedDisk::MountedDisk(const std::vector<MountedFile>& Files) :
        HandleFileCluster([](void* Ctx, uint64_t LCN, uint8_t Buffer[4096]) {
            memset(Buffer, 'A', 1024);
            memset(Buffer + 1024, 'B', 1024);
            memset(Buffer + 2048, 'C', 1024);
            memset(Buffer + 3072, 'D', 1024);
            *(uint64_t*)Buffer = LCN;
        }),
        PrivateData(DataAllocator.allocate(1))
    {
        auto Data = (MountedData*)PrivateData;

        std::construct_at(Data, HandleFileCluster);

        Data->Files.reserve(Files.size());
        for (auto& File : Files) {
            EGL3_CONDITIONAL_LOG(File.Path.size() < 256, LogLevel::Critical, "File path too long");

            auto& DataFile = Data->Files.emplace_back(EGL3File{
                .size = File.FileSize,
                .user_context = File.UserContext,
                .runs = {}
            });

            strcpy_s(DataFile.path, File.Path.c_str());
        }
    }

    MountedDisk::~MountedDisk()
    {
        auto Data = (MountedData*)PrivateData;

        std::destroy_at(Data);
        DataAllocator.deallocate(Data, 1);
    }

    void MountedDisk::Initialize() {
        auto Data = (MountedData*)PrivateData;

        SPD_PARTITION Partition{
            .Type = 7,
            .BlockAddress = 8,
            .BlockCount = DiskSize * SectorsPerMegabyte - 8
        };
        EGL3_CONDITIONAL_LOG(SpdDefinePartitionTable(&Partition, 1, Data->MBRData) == ERROR_SUCCESS, LogLevel::Critical, "Could not create MBR data");

        *(uint32_t*)(Data->MBRData + 440) = Utils::Random();// DiskSignature;

        EGL3_CONDITIONAL_LOG(EGL3CreateDisk(Partition.BlockCount, "EGL3 Game", Data->Files.data(), Data->Files.size(), (void**)&Data->Disk), LogLevel::Critical, "Could not create NTFS disk");
    }

    static __forceinline void HandleCluster(MountedData* Data, UINT64 LCN, UINT8 Buffer[4096]) {
        if (!LCN) { // MBR cluster
            memcpy(Buffer, Data->MBRData, 512);
            return;
        }

        // NTFS ignores the MBR cluster
        --LCN;

        auto search = Data->Disk->data.find(LCN);
        if (search != Data->Disk->data.end()) {
            memcpy(Buffer, search->second, 4096);
            return;
        }

        for (auto& File : Data->Files) {
            auto CurrentRun = File.runs;
            uint64_t lIdx = 0;
            while (CurrentRun->count) {
                if (CurrentRun->idx <= LCN && uint64_t(CurrentRun->idx) + CurrentRun->count > LCN) {
                    Data->FileClusterCallback(File.user_context, LCN - CurrentRun->idx + lIdx, Buffer);
                    return;
                }
                lIdx += CurrentRun->count;
                CurrentRun++;
            }
        }
    }

    static inline BOOLEAN ExceptionFilter(ULONG Code, PEXCEPTION_POINTERS Pointers,
        PUINT_PTR PDataAddress)
    {
        if (EXCEPTION_IN_PAGE_ERROR != Code)
            return EXCEPTION_CONTINUE_SEARCH;

        *PDataAddress = 2 <= Pointers->ExceptionRecord->NumberParameters ?
            Pointers->ExceptionRecord->ExceptionInformation[1] : 0;
        return EXCEPTION_EXECUTE_HANDLER;
    }

    static VOID ReadCallbackWrapped(SPD_STORAGE_UNIT* StorageUnit, PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, UINT8 ASC, SPD_STORAGE_UNIT_STATUS* Status) {
        UINT_PTR ExceptionDataAddress;
        UINT64 Information, * PInformation;

        __try {
            ZeroMemory(Buffer, BlockCount * 512llu);

            UINT8 ClusterBuffer[4096];
            auto StartCluster = BlockAddress >> 3; // 8 sectors = 1 cluster, we apply alignment downwards
            auto ThrowawaySectorCount = BlockAddress & 7; // number of sectors to throw away from the beginning
            auto TotalClusterCount = Utils::Align<8>(ThrowawaySectorCount + BlockCount) >> 3;
            for (UINT64 i = 0; i < TotalClusterCount; ++i) {
                memset(ClusterBuffer, 0, 4096);
                HandleCluster((MountedData*)StorageUnit->UserContext, StartCluster + i, ClusterBuffer);
                if (i == 0) {
                    memcpy(Buffer, ClusterBuffer + ThrowawaySectorCount * 512llu, std::min(BlockCount * 512llu, 4096 - ThrowawaySectorCount * 512llu));
                }
                else {
                    memcpy(((UINT8*)Buffer) + i * 4096llu - ThrowawaySectorCount * 512llu, ClusterBuffer, std::min(BlockCount * 512llu, 4096llu));
                }
                BlockCount -= 8;
            }
        }
        __except (ExceptionFilter(GetExceptionCode(), GetExceptionInformation(), &ExceptionDataAddress)) {
            if (0 != Status)
            {
                PInformation = 0;
                if (0 != ExceptionDataAddress)
                {
                    Information = 4055; //(UINT64)(ExceptionDataAddress - (UINT_PTR)RawDisk->Pointer) / RawDisk->BlockLength;
                    PInformation = &Information;
                }

                SpdStorageUnitStatusSetSense(Status, SCSI_SENSE_MEDIUM_ERROR, ASC, PInformation);
            }
        }
    }

    static BOOLEAN ReadCallback(SPD_STORAGE_UNIT* StorageUnit,
        PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, BOOLEAN FlushFlag,
        SPD_STORAGE_UNIT_STATUS* Status)
    {
        ReadCallbackWrapped(StorageUnit, Buffer, BlockAddress, BlockCount, SCSI_ADSENSE_UNRECOVERED_ERROR, Status);

        return TRUE;
    }

    constexpr SPD_STORAGE_UNIT_INTERFACE DiskInterface =
    {
        ReadCallback,
        0, // write
        0, // flush
        0, // unmap
    };

    void MountedDisk::Create() {
        auto Data = (MountedData*)PrivateData;

        SpdDebugLogSetHandle((HANDLE)STD_ERROR_HANDLE);
        {
            SPD_STORAGE_UNIT_PARAMS Params{
                .BlockCount = DiskSize * SectorsPerMegabyte,
                .BlockLength = SectorSize,
                .WriteProtected = 1,
                .CacheSupported = 0,
                .UnmapSupported = 0,
                .EjectDisabled = 1, // disables eject ui
                // https://social.msdn.microsoft.com/Forums/WINDOWS/en-US/6223c501-f55a-4df3-a148-df12d8032c7b#ec8f32d4-44ea-4523-9401-e7c8c1f19fed
                // Since its inception USBStor.sys specifies 0x10000 for MaximumTransferLength
                // Best to stay at 64kb, but some have gone to 128kb
                .MaxTransferLength = 0x10000 // 64 * 1024
            };

            static_assert(sizeof(Params.Guid) == 16, "Guid is not 16 bytes (?)");
            Utils::GenerateRandomGuid((char*)&Params.Guid);

            strcpy_s((char*)Params.ProductId, sizeof(Params.ProductId), "EGL3");
            strcpy_s((char*)Params.ProductRevisionLevel, sizeof(Params.ProductRevisionLevel), "1.0");

            // This can fail with error code 5
            // https://github.com/billziss-gh/winspd/blob/master/doc/WinSpd-Tutorial.asciidoc#testing-the-integration-with-the-operating-system
            // "This happens because mounting a storage unit requires administrator privileges."
            EGL3_CONDITIONAL_LOG(SpdStorageUnitCreate(NULL, &Params, &DiskInterface, &Data->Unit) == ERROR_SUCCESS, LogLevel::Critical, "Could not create storage unit");

            Data->Unit->UserContext = Data;
        }
    }

    void MountedDisk::Mount(uint32_t LogFlags) {
        auto Data = (MountedData*)PrivateData;

        SpdStorageUnitSetDebugLog(Data->Unit, LogFlags);
        EGL3_CONDITIONAL_LOG(SpdStorageUnitStartDispatcher(Data->Unit, 0) == ERROR_SUCCESS, LogLevel::Critical, "Could not mount storage unit");
        SpdGuardSet(&Data->CloseGuard, Data->Unit);
    }

    void ExecuteClose(PVOID Unit) {
        SpdStorageUnitShutdown((SPD_STORAGE_UNIT*)Unit);
    }

    void MountedDisk::Unmount()
    {
        auto Data = (MountedData*)PrivateData;

        SpdGuardExecute(&Data->CloseGuard, ExecuteClose);
    }
}