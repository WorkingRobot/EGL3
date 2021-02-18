#include "MountedDisk.h"

#include "../../disk/interface.h"
#include "../../utils/Align.h"
#include "../../utils/Assert.h"
#include "../../utils/Random.h"
#include "../../utils/mmio/MmioFile.h"

#include <memory>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winspd/winspd.h>

namespace EGL3::Storage::Models {
    static constexpr uint32_t SectorSize = 512;
    static constexpr uint32_t DiskSize = DISK_SIZE / (1 << 20);
    static constexpr uint32_t DiskSignature = 0x334C4745; // "EGL3"

    static constexpr uint32_t SectorsPerMegabyte = (1 << 20) / SectorSize;

    // I use sector and cluster interchangably, even though technically a
    // sector would be 512 bytes and a cluster 4096 bytes, thanks wikipedia
    // Sorry not sorry, I'll fix that eventually, I guess
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

    static inline BOOLEAN ExceptionFilter(ULONG Code, PEXCEPTION_POINTERS Pointers,
        PUINT_PTR PDataAddress)
    {
        if (EXCEPTION_IN_PAGE_ERROR != Code)
            return EXCEPTION_CONTINUE_SEARCH;

        *PDataAddress = 2 <= Pointers->ExceptionRecord->NumberParameters ?
            Pointers->ExceptionRecord->ExceptionInformation[1] : 0;
        return EXCEPTION_EXECUTE_HANDLER;
    }

    template<class T>
    static VOID OperationCallback(const T& Execute, UINT8 ASC, SPD_STORAGE_UNIT_STATUS* Status)
    {
        UINT_PTR ExceptionDataAddress;
        UINT64 Information, * PInformation;

        __try {
            Execute();
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

    static void HandleCluster(MountedData* Data, UINT64 Idx, UINT8 Buffer[4096]) {
        if (!Idx) { // MBR cluster
            memcpy(Buffer, Data->MBRData, 512);
            return;
        }

        // Filesystem ignores the MBR cluster
        --Idx;

        auto search = Data->Disk->try_get_cluster(Idx);
        if (search) {
            memcpy(Buffer, search, 4096);
            return;
        }

        for (auto& File : Data->Files) {
            auto CurrentRun = File.runs;
            uint64_t lIdx = 0;
            while (CurrentRun->count) {
                if (CurrentRun->idx <= Idx && uint64_t(CurrentRun->idx) + CurrentRun->count > Idx) {
                    Data->FileClusterCallback(File.user_context, Idx - CurrentRun->idx + lIdx, Buffer);
                    return;
                }
                lIdx += CurrentRun->count;
                CurrentRun++;
            }
        }
    }

    static BOOLEAN ReadCallback(SPD_STORAGE_UNIT* StorageUnit,
        PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, BOOLEAN FlushFlag,
        SPD_STORAGE_UNIT_STATUS* Status)
    {
        OperationCallback([&]() {
            auto Data = (MountedData*)StorageUnit->UserContext;
            
            if (BlockAddress & 7) {
                auto SectorsToSkip = BlockAddress & 7;
                auto SectorsToRead = std::min((UINT64)BlockCount, 8 - SectorsToSkip);

                uint8_t ClusBuf[4096];
                HandleCluster(Data, BlockAddress / 8, ClusBuf);
                memcpy(Buffer, ClusBuf + (SectorsToSkip * 512), SectorsToRead * 512);

                BlockAddress += SectorsToRead;
                BlockCount -= SectorsToRead;
                Buffer = (uint8_t*)Buffer + (SectorsToRead * 512);
            }

            EGL3_CONDITIONAL_LOG(BlockCount == 0 || (BlockAddress & 7) == 0, LogLevel::Critical, "Block address must be a multiple of 8 at this point");
            while (BlockCount) {
                if (BlockCount >= 8) {
                    HandleCluster(Data, BlockAddress / 8, (uint8_t*)Buffer);

                    BlockAddress += 8;
                    BlockCount -= 8;
                    Buffer = (uint8_t*)Buffer + 4096;
                }
                else {
                    uint8_t ClusBuf[4096];
                    HandleCluster(Data, BlockAddress / 8, ClusBuf);
                    memcpy(Buffer, ClusBuf, (UINT64)BlockCount * 512);
                    return;
                }
            }
        }, SCSI_ADSENSE_UNRECOVERED_ERROR, Status);

        return TRUE;
    }

    static BOOLEAN WriteCallback(SPD_STORAGE_UNIT* StorageUnit,
        PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, BOOLEAN FlushFlag,
        SPD_STORAGE_UNIT_STATUS* Status)
    {
        OperationCallback([&]() {

        }, SCSI_ADSENSE_WRITE_ERROR, Status);

        return TRUE;
    }

    static BOOLEAN FlushCallback(SPD_STORAGE_UNIT* StorageUnit,
        UINT64 BlockAddress, UINT32 BlockCount,
        SPD_STORAGE_UNIT_STATUS* Status)
    {
        OperationCallback([&]() {

        }, SCSI_ADSENSE_WRITE_ERROR, Status);

        return TRUE;
    }

    static BOOLEAN UnmapCallback(SPD_STORAGE_UNIT* StorageUnit,
        SPD_UNMAP_DESCRIPTOR Descriptors[], UINT32 Count,
        SPD_STORAGE_UNIT_STATUS* Status)
    {
        OperationCallback([&]() {

        }, SCSI_ADSENSE_NO_SENSE, Status);

        return TRUE;
    }

    constexpr SPD_STORAGE_UNIT_INTERFACE DiskInterface =
    {
        ReadCallback,
        WriteCallback,
        FlushCallback,
        UnmapCallback
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