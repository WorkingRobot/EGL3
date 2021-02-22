#include "MountedDisk.h"

#include "../../disk/interface.h"
#include "../../utils/Align.h"
#include "../../utils/Assert.h"
#include "../../utils/Random.h"
#include "../../utils/mmio/MmioFile.h"

#include <bitset>
#include <memory>
#include <mutex>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winspd/winspd.h>

namespace EGL3::Storage::Models {
    static constexpr uint32_t SectorSize = 512;
    static constexpr uint32_t DiskSize = DISK_SIZE / (1 << 20);

    static constexpr uint32_t SectorsPerMegabyte = (1 << 20) / SectorSize;

    // I use sector and cluster interchangably, even though technically a
    // sector would be 512 bytes and a cluster 4096 bytes, thanks wikipedia
    // Sorry not sorry, I'll fix that eventually, I guess
    struct MountedData {
        uint8_t MBRData[4096];
        uint8_t BlankData[4096];
        uint32_t DiskSignature; // 0x334C4745 = "EGL3"
        std::vector<EGL3File> Files;
        AppendingFile* Disk;
        SPD_STORAGE_UNIT* Unit;
        SPD_GUARD CloseGuard;
        const decltype(MountedDisk::HandleFileCluster)& FileClusterCallback;

        MountedData(const decltype(MountedDisk::HandleFileCluster)& Callback) :
            MBRData{},
            BlankData{},
            Disk(nullptr),
            Unit(nullptr),
            CloseGuard(SPD_GUARD_INIT),
            FileClusterCallback(Callback)
        {

        }
    };

    static std::allocator<MountedData> DataAllocator;

    MountedDisk::MountedDisk(const std::vector<MountedFile>& Files, uint32_t DiskSignature) :
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

        Data->DiskSignature = DiskSignature;
    }

    MountedDisk::~MountedDisk()
    {
        auto Data = (MountedData*)PrivateData;

        std::destroy_at(Data);
        DataAllocator.deallocate(Data, 1);
    }

    char MountedDisk::GetDriveLetter()
    {
        auto Data = (MountedData*)PrivateData;

        HKEY MountedDevicesKey;
        char Letter = '\0';
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\MountedDevices", 0, KEY_READ, &MountedDevicesKey) == ERROR_SUCCESS) {
            DWORD ValueCount = 0;

            DWORD Ret = RegQueryInfoKey(MountedDevicesKey, NULL, NULL, NULL, NULL, NULL, NULL, &ValueCount, NULL, NULL, NULL, NULL);

            if (Ret == ERROR_SUCCESS) {
                DWORD ValueNameSize = 255;
                TCHAR ValueName[255];
                DWORD ValueType;
                DWORD ValueDataSize = 255;
                BYTE ValueData[255];
                for (DWORD i = 0; i < ValueCount; ++i) {
                    ValueNameSize = 255;
                    ValueDataSize = 255;
                    Ret = RegEnumValue(MountedDevicesKey, i, ValueName, &ValueNameSize, NULL, &ValueType, ValueData, &ValueDataSize);
                    if (Ret == ERROR_SUCCESS) {
                        if (ValueType == REG_BINARY && // Data must be binary
                            ValueDataSize == 12 && // Data is exactly 12 bytes
                            *(uint32_t*)ValueData == Data->DiskSignature && // First 4 bytes are the MBR signature
                            *(uint64_t*)(ValueData + sizeof(uint32_t)) == 8 * SectorSize // Next 8 bytes are the offset at which the partition starts (sector 8)
                            ) {
                            if (ValueNameSize == 14 && // Name is exactly 14 bytes (matches '\DosDevices\D:')
                                strncmp(ValueName, "\\DosDevices\\", 12) == 0 && // Starts with '\DosDevices\'
                                ValueName[13] == ':' && // Last character is :
                                ValueName[12] >= 'A' && ValueName[12] <= 'Z') // Drive letter must be uppercase
                            {
                                Letter = ValueName[12];
                                break;
                            }
                            else {
                                printf("name did not match, but data did match\n");
                            }
                        }
                    }
                }
            }
        }
        RegCloseKey(MountedDevicesKey);
        return Letter;
    }

    void MountedDisk::Initialize() {
        auto Data = (MountedData*)PrivateData;

        SPD_PARTITION Partition{
            .Type = 7,
            .BlockAddress = 8,
            .BlockCount = DiskSize * SectorsPerMegabyte - 8
        };
        EGL3_CONDITIONAL_LOG(SpdDefinePartitionTable(&Partition, 1, Data->MBRData) == ERROR_SUCCESS, LogLevel::Critical, "Could not create MBR data");

        *(uint32_t*)(Data->MBRData + 440) = Data->DiskSignature;

        EGL3_CONDITIONAL_LOG(EGL3CreateDisk(Partition.BlockCount, "EGL3 Game", Data->Files.data(), Data->Files.size(), (void**)&Data->Disk), LogLevel::Critical, "Could not create NTFS disk");
    }

    static constexpr int ClusterPoolCount = 64;
    static UINT8 ClusterPool[ClusterPoolCount][4096];
    static std::mutex ClusterPoolMutex;
    static std::bitset<ClusterPoolCount> ClusterPoolAvailable(-1);

    static UINT8* AllocateCluster() {
        std::lock_guard Lock(ClusterPoolMutex);
        int Idx = 0;
        while (Idx < ClusterPoolAvailable.size() && !ClusterPoolAvailable.test(Idx)) {
            ++Idx;
        }
        if (Idx < ClusterPoolAvailable.size()) {
            ClusterPoolAvailable.set(Idx, false);
            //printf("> %d\n", Idx);
            EGL3_CONDITIONAL_LOG((ClusterPool[Idx] - ClusterPool[0]) / sizeof(ClusterPool[0]) == Idx, LogLevel::Critical, "Idx wouldn't work");
            return ClusterPool[Idx];
        }
        return nullptr;
    }

    static void FreeCluster(UINT8* Data, bool FreeAfterUse) {
        if (FreeAfterUse) {
            std::lock_guard Lock(ClusterPoolMutex);
            int Idx = (Data - ClusterPool[0]) / sizeof(ClusterPool[0]);
            //printf("< %d\n", Idx);
            ClusterPoolAvailable.set(Idx, true);
        }
    }

    static UINT8* HandleCluster(MountedData* Data, UINT64 Idx, bool& FreeAfterUse) {
        // MBR cluster
        if (Idx == 0) {
            FreeAfterUse = false;
            return Data->MBRData;
        }

        // Filesystem ignores the MBR cluster
        --Idx;

        if (auto ClusterPtr = Data->Disk->try_get_cluster(Idx)) {
            FreeAfterUse = false;
            return ClusterPtr;
        }

        for (auto& File : Data->Files) {
            auto CurrentRun = File.runs;
            uint64_t lIdx = 0;
            while (CurrentRun->count) {
                if (CurrentRun->idx <= Idx && uint64_t(CurrentRun->idx) + CurrentRun->count > Idx) {
                    auto ClusterBuffer = AllocateCluster();
                    Data->FileClusterCallback(File.user_context, Idx - CurrentRun->idx + lIdx, ClusterBuffer);
                    FreeAfterUse = true;
                    return ClusterBuffer;
                }
                lIdx += CurrentRun->count;
                CurrentRun++;
            }
        }

        FreeAfterUse = false;
        return Data->BlankData;
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

    static BOOLEAN ReadCallback(SPD_STORAGE_UNIT* StorageUnit,
        PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, BOOLEAN FlushFlag,
        SPD_STORAGE_UNIT_STATUS* Status)
    {
        //printf("%llu %u\n", BlockAddress, BlockCount);
        UINT_PTR ExceptionDataAddress;
        UINT64 Information, * PInformation;

        __try {
            auto Data = (MountedData*)StorageUnit->UserContext;
            
            if (BlockAddress & 7) {
                auto SectorsToSkip = BlockAddress & 7;
                auto SectorsToRead = std::min((UINT64)BlockCount, 8 - SectorsToSkip);

                bool FreeClusBuf;
                auto ClusBuf = HandleCluster(Data, BlockAddress / 8, FreeClusBuf);
                memcpy(Buffer, ClusBuf + (SectorsToSkip * 512), SectorsToRead * 512);
                FreeCluster(ClusBuf, FreeClusBuf);

                BlockAddress += SectorsToRead;
                BlockCount -= SectorsToRead;
                Buffer = (uint8_t*)Buffer + (SectorsToRead * 512);
            }

            EGL3_CONDITIONAL_LOG(BlockCount == 0 || (BlockAddress & 7) == 0, LogLevel::Critical, "Block address must be a multiple of 8 at this point");
            while (BlockCount) {
                if (BlockCount >= 8) {
                    bool FreeClusBuf;
                    auto ClusBuf = HandleCluster(Data, BlockAddress / 8, FreeClusBuf);
                    memcpy(Buffer, ClusBuf, 4096);
                    FreeCluster(ClusBuf, FreeClusBuf);

                    BlockAddress += 8;
                    BlockCount -= 8;
                    Buffer = (uint8_t*)Buffer + 4096;
                }
                else {
                    bool FreeClusBuf;
                    auto ClusBuf = HandleCluster(Data, BlockAddress / 8, FreeClusBuf);
                    memcpy(Buffer, ClusBuf, (UINT64)BlockCount * 512);
                    FreeCluster(ClusBuf, FreeClusBuf);
                    break;
                }
            }
        }
        __except (ExceptionFilter(GetExceptionCode(), GetExceptionInformation(), &ExceptionDataAddress)) {
            printf("error!\n");
            if (0 != Status)
            {
                PInformation = 0;
                if (0 != ExceptionDataAddress)
                {
                    Information = 4055; //(UINT64)(ExceptionDataAddress - (UINT_PTR)RawDisk->Pointer) / RawDisk->BlockLength;
                    PInformation = &Information;
                }

                SpdStorageUnitStatusSetSense(Status, SCSI_SENSE_MEDIUM_ERROR, SCSI_ADSENSE_UNRECOVERED_ERROR, PInformation);
            }
        }
        return TRUE;
    }

    constexpr SPD_STORAGE_UNIT_INTERFACE DiskInterface =
    {
        ReadCallback,
        0,
        0,
        0
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
                .MaxTransferLength = 1024 * 1024
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
        EGL3_CONDITIONAL_LOG(SpdStorageUnitStartDispatcher(Data->Unit, 1) == ERROR_SUCCESS, LogLevel::Critical, "Could not mount storage unit"); // std::thread::hardware_concurrency() * 4
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