#include "MountedDisk.h"

#include "../disk/interface.h"
#include "../utils/mmio/MmioFile.h"
#include "../utils/Align.h"
#include "../utils/Log.h"
#include "../utils/Random.h"
#include "../utils/Version.h"
#include "IntervalTree.h"

#include <memory>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winspd/winspd.h>

namespace EGL3::Service {
    static constexpr uint32_t DiskSizeMegabytes = DISK_SIZE / (1 << 20);

    static constexpr uint32_t SectorsPerMegabyte = (1 << 20) / 4096;

    struct IntervalContext {
        enum class Type : uint8_t {
            File,
            Filesystem,
            MBR,
            Empty
        };

        IntervalContext(Type Type, void* Data = nullptr) :
            ContextType(Type),
            Data(Data)
        {

        }

        IntervalContext(const IntervalContext&) = default;

        Type ContextType = Type::Empty;

        void* Data = nullptr;
    };

    // I use sector and cluster somewhat interchangably, both a cluster 
    // and a sector would be 4096 bytes in this instance, thanks wikipedia
    // Sorry not sorry, I'll fix that eventually, I guess
    struct MountedData {
        uint8_t MBRData[4096];
        uint32_t DiskSignature;
        std::vector<EGL3File> Files;
        AppendingFile* Disk;
        SPD_STORAGE_UNIT* Unit;
        SPD_GUARD CloseGuard;
        MountedDisk::ClusterCallback FileClusterCallback;
        IntervalTree<uint32_t, IntervalContext> DiskIntervals;

        MountedData() :
            MBRData{},
            DiskSignature(),
            Disk(nullptr),
            Unit(nullptr),
            CloseGuard(SPD_GUARD_INIT),
            FileClusterCallback(nullptr),
            DiskIntervals()
        {

        }
    };

    static std::allocator<MountedData> DataAllocator;

    MountedDisk::MountedDisk(const std::vector<MountedFile>& Files, uint32_t DiskSignature, ClusterCallback Callback) :
        PrivateData(DataAllocator.allocate(1))
    {
        auto Data = (MountedData*)PrivateData;
        std::construct_at(Data);

        Data->Files.reserve(Files.size());
        for (auto& File : Files) {
            EGL3_VERIFY(File.Path.size() < 256, "File path too long");

            auto& DataFile = Data->Files.emplace_back(EGL3File{
                .size = File.FileSize,
                .user_context = File.UserContext,
                .run_size = 0
            });

            strcpy_s(DataFile.path, File.Path.c_str());
        }

        Data->DiskSignature = DiskSignature;
        Data->FileClusterCallback = Callback;
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

        // Intervals are [start, stop], so you need to subtract 1 from the end to make it inclusive
        decltype(MountedData::DiskIntervals)::IntervalList Intervals;

        Intervals.push_back({ 0, 0, IntervalContext::Type::MBR });

        auto& DiskData = Data->Disk->get_data();
        std::vector<uint32_t> DiskKeys;
        DiskKeys.reserve(1 + DiskData.size());
        for (auto& Key : DiskData) {
            DiskKeys.emplace_back(Key.first + 1);
        }
        std::sort(DiskKeys.begin(), DiskKeys.end());

        {
            uint32_t First, Prev;
            First = Prev = DiskKeys.front();
            for (auto Itr = DiskKeys.begin() + 1; Itr != DiskKeys.end(); ++Itr) {
                if (*Itr - Prev == 1) {
                    Prev = *Itr;
                }
                else {
                    Intervals.push_back({ First, Prev + 1, IntervalContext::Type::Filesystem });
                    First = Prev = *Itr;
                }
            }
            Intervals.push_back({ First, Prev + 1, IntervalContext::Type::Filesystem });
        }

        {
            for (auto& File : Data->Files) {
                if (File.run_size != 0) {
                    Intervals.push_back({ File.run_idx + 1, File.run_idx + File.run_size, {IntervalContext::Type::File, File.user_context} });
                }
            }
        }

        Data->DiskIntervals = std::move(Intervals);
    }

    static void HandleInterval(const MountedData& Data, const Interval<uint32_t, IntervalContext>& Interval, UINT32 Offset, UINT32 Count, PUINT8 Buffer) {
        switch (Interval.value.ContextType)
        {
        case IntervalContext::Type::File:
            Data.FileClusterCallback((void*)Interval.value.Data, Offset, Count, Buffer);
            break;
        case IntervalContext::Type::Filesystem:
            while (Count) {
                if (auto ClusterPtr = Data.Disk->try_get_cluster(Interval.start + Offset - 1)) {
                    memcpy(Buffer, ClusterPtr, 4096);
                }
                Buffer += 4096;
                ++Offset;
                --Count;
            }
            break;
        case IntervalContext::Type::MBR:
            memcpy(Buffer, Data.MBRData, 4096);
            break;
        }
    }

    static BOOLEAN ReadCallback(SPD_STORAGE_UNIT* StorageUnit,
        PVOID Buffer, UINT64 BlockAddress, UINT32 BlockCount, BOOLEAN FlushFlag,
        SPD_STORAGE_UNIT_STATUS* Status) noexcept
    {
        memset(Buffer, 0, BlockCount * 4096);

        auto& Data = *(MountedData*)StorageUnit->UserContext;

        Data.DiskIntervals.VisitOverlapping(BlockAddress, BlockAddress + BlockCount - 1,
            [&](const auto& Interval) {
                HandleInterval(Data, Interval,
                    BlockAddress - Interval.start,
                    std::min<uint32_t>(BlockAddress + BlockCount, Interval.stop + 1) - BlockAddress,
                    (PUINT8)Buffer + 4096 * (std::max<uint32_t>(BlockAddress, Interval.start) - BlockAddress));
            });

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

        SpdDebugLogSetHandle(GetStdHandle(STD_ERROR_HANDLE));
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
            strncpy_s((char*)Params.ProductRevisionLevel, sizeof(Params.ProductRevisionLevel), Utils::Version::GetShortAppVersion(), _TRUNCATE);

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
        EGL3_VERIFY(SpdStorageUnitStartDispatcher(Data->Unit, 0) == ERROR_SUCCESS, "Could not mount storage unit");
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