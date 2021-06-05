#include "MountedDisk.h"

#include "../disk/interface.h"
#include "../utils/Align.h"
#include "../utils/Log.h"
#include "../utils/Random.h"
#include "../utils/mmio/MmioFile.h"
#include "xorfilter/xorfilter.h"

#include <memory>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winspd/winspd.h>

namespace EGL3::Service {
    static constexpr uint32_t DiskSizeMegabytes = DISK_SIZE / (1 << 20);

    static constexpr uint32_t SectorsPerMegabyte = (1 << 20) / 4096;

    // I use sector and cluster somewhat interchangably, even though technically
    // a sector would be 512 bytes and a cluster 4096 bytes, thanks wikipedia
    // Sorry not sorry, I'll fix that eventually, I guess
    struct MountedData {
        uint8_t MBRData[4096];
        uint8_t BlankData[4096];
        uint32_t DiskSignature;
        std::vector<EGL3File> Files;
        AppendingFile* Disk;
        SPD_STORAGE_UNIT* Unit;
        SPD_GUARD CloseGuard;
        const decltype(MountedDisk::HandleFileCluster)& FileClusterCallback;
        XorFilter DiskFilter;

        MountedData(const decltype(MountedDisk::HandleFileCluster)& Callback) :
            MBRData{},
            BlankData{},
            DiskSignature(),
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
            EGL3_VERIFY(File.Path.size() < 256, "File path too long");

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
        Unmount();

        auto Data = (MountedData*)PrivateData;

        std::destroy_at(Data);
        DataAllocator.deallocate(Data, 1);
    }

    char MountedDisk::GetDriveLetter() const
    {
        auto Data = (const MountedData*)PrivateData;

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
                            *(uint64_t*)(ValueData + sizeof(uint32_t)) == 4096 // Next 8 bytes are the offset at which the partition starts (sector 1)
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
            .BlockAddress = 1,
            .BlockCount = DiskSizeMegabytes * SectorsPerMegabyte - 1
        };
        EGL3_VERIFY(SpdDefinePartitionTable(&Partition, 1, Data->MBRData) == ERROR_SUCCESS, "Could not create MBR data");

        *(uint32_t*)(Data->MBRData + 440) = Data->DiskSignature;

        EGL3_VERIFY(EGL3CreateDisk("EGL3 Game", Data->Files.data(), Data->Files.size(), (void**)&Data->Disk), "Could not create disk");

        auto& DiskData = Data->Disk->get_data();
        std::vector<uint64_t> DiskKeys;
        DiskKeys.reserve(1 + DiskData.size());
        DiskKeys.emplace_back(0); // MBR cluster
        for (auto& Key : DiskData) {
            DiskKeys.emplace_back(Key.first + 1);
        }

        EGL3_VERIFY(Data->DiskFilter.Initialize(DiskKeys.data(), DiskKeys.size()), "Could not create xor filter");
    }

    static UINT8 ClusterCache[4096];

    static UINT8* HandleCluster(MountedData* Data, UINT64 Idx) {
        if (Data->DiskFilter.Contains(Idx)) {
            if (Idx == 0) [[unlikely]] {
                return Data->MBRData;
            }

            if (auto ClusterPtr = Data->Disk->try_get_cluster(Idx - 1)) {
                return ClusterPtr;
            }
        }
        
        --Idx;

        for (auto& File : Data->Files) {
            auto CurrentRun = File.runs;
            uint64_t lIdx = 0;
            while (CurrentRun->count) {
                if (CurrentRun->idx <= Idx && uint64_t(CurrentRun->idx) + CurrentRun->count > Idx) {
                    Data->FileClusterCallback(File.user_context, Idx - CurrentRun->idx + lIdx, ClusterCache);
                    return ClusterCache;
                }
                lIdx += CurrentRun->count;
                CurrentRun++;
            }
        }

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
        UINT_PTR ExceptionDataAddress;
        UINT64 Information, * PInformation;

        //printf("read %llu %u\n", BlockAddress, BlockCount);
        __try {
            auto Data = (MountedData*)StorageUnit->UserContext;

            while (BlockCount) {
                memcpy(Buffer, HandleCluster(Data, BlockAddress), 4096);

                ++BlockAddress;
                --BlockCount;
                Buffer = (UINT8*)Buffer + 4096;
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

    static BOOLEAN WriteCallback(SPD_STORAGE_UNIT* StorageUnit,
        PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, BOOLEAN FlushFlag,
        SPD_STORAGE_UNIT_STATUS* Status)
    {
        UINT_PTR ExceptionDataAddress;
        UINT64 Information, * PInformation;

        //printf("write %llu %u\n", BlockAddress, BlockCount);
        __try {

            auto Data = (MountedData*)StorageUnit->UserContext;

            while (BlockCount) {
                UINT8* ClusBuf;
                if (BlockAddress == 0) {
                    ClusBuf = Data->MBRData;
                }
                else {
                    ClusBuf = Data->Disk->get_cluster(BlockAddress - 1);
                }
                memcpy(ClusBuf, Buffer, 4096);

                ++BlockAddress;
                --BlockCount;
                Buffer = (UINT8*)Buffer + 4096;
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
        WriteCallback,
        0,
        0
    };

    void MountedDisk::Create() {
        auto Data = (MountedData*)PrivateData;

        SpdDebugLogSetHandle((HANDLE)STD_ERROR_HANDLE);
        {
            SPD_STORAGE_UNIT_PARAMS Params{
                .BlockCount = DiskSizeMegabytes * SectorsPerMegabyte,
                .BlockLength = 4096,
                .DeviceType = 0,
                .WriteProtected = 1,
                .CacheSupported = 0,
                .UnmapSupported = 0,
                .EjectDisabled = 1, // disables eject ui
                .MaxTransferLength = 2 * 1024 * 1024
            };

            static_assert(sizeof(Params.Guid) == 16, "Guid is not 16 bytes (?)");
            Utils::GenerateRandomGuid((char*)&Params.Guid);

            strcpy_s((char*)Params.ProductId, sizeof(Params.ProductId), "EGL3");
            strncpy_s((char*)Params.ProductRevisionLevel, sizeof(Params.ProductRevisionLevel), CONFIG_VERSION_SHORT, _TRUNCATE);

            // This can fail with error code 5
            // https://github.com/billziss-gh/winspd/blob/master/doc/WinSpd-Tutorial.asciidoc#testing-the-integration-with-the-operating-system
            // "This happens because mounting a storage unit requires administrator privileges."
            EGL3_VERIFY(SpdStorageUnitCreate(NULL, &Params, &DiskInterface, &Data->Unit) == ERROR_SUCCESS, "Could not create storage unit");

            Data->Unit->UserContext = Data;
        }
    }

    void MountedDisk::Mount(uint32_t LogFlags) {
        auto Data = (MountedData*)PrivateData;

        SpdStorageUnitSetDebugLog(Data->Unit, LogFlags);
        EGL3_VERIFY(SpdStorageUnitStartDispatcher(Data->Unit, 1) == ERROR_SUCCESS, "Could not mount storage unit");
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