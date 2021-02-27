#include "DriveCounter.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Pdh.h>
#include <string>

#pragma comment(lib, "Pdh.lib")

namespace EGL3::Service {
    DriveCounter::DriveCounter() :
        Valid(false),
        QueryHandle(nullptr)
    {
    }

    DriveCounter::DriveCounter(char DriveLetter) :
        Valid(false)
    {
        auto Error = PdhOpenQuery(NULL, NULL, &QueryHandle);
        if (Error != ERROR_SUCCESS) {
            printf("PdhOpenQuery failed, GLE=%d.\n", Error);
            return;
        }

        constexpr static const char* NameCurrentDiskQueueLength = "\\LogicalDisk(*:)\\Current Disk Queue Length";
        constexpr static const char* NamePercentDiskTime = "\\LogicalDisk(*:)\\% Disk Time";
        constexpr static const char* NameAvgDiskQueueLength = "\\LogicalDisk(*:)\\Avg. Disk Queue Length";
        constexpr static const char* NameDiskTransfersPerSec = "\\LogicalDisk(*:)\\Disk Transfers/sec";
        constexpr static const char* NameAvgDiskBytesPerRead = "\\LogicalDisk(*:)\\Avg. Disk Bytes/Write";
        constexpr static const char* NameDiskReadBytesPerSec = "\\LogicalDisk(*:)\\Disk Write Bytes/sec";

        std::string CounterName;

        CounterName = NameCurrentDiskQueueLength;
        CounterName[13] = DriveLetter;
        CurrentDiskQueueLength = AddCounter(CounterName.c_str());

        CounterName = NamePercentDiskTime;
        CounterName[13] = DriveLetter;
        PercentDiskTime = AddCounter(CounterName.c_str());

        CounterName = NameAvgDiskQueueLength;
        CounterName[13] = DriveLetter;
        AvgDiskQueueLength = AddCounter(CounterName.c_str());

        CounterName = NameDiskTransfersPerSec;
        CounterName[13] = DriveLetter;
        DiskTransfersPerSec = AddCounter(CounterName.c_str());

        CounterName = NameAvgDiskBytesPerRead;
        CounterName[13] = DriveLetter;
        AvgDiskBytesPerRead = AddCounter(CounterName.c_str());

        CounterName = NameDiskReadBytesPerSec;
        CounterName[13] = DriveLetter;
        DiskReadBytesPerSec = AddCounter(CounterName.c_str());

        Valid = CurrentDiskQueueLength && PercentDiskTime &&
            AvgDiskQueueLength && DiskTransfersPerSec &&
            AvgDiskBytesPerRead && DiskReadBytesPerSec;
    }

    DriveCounter::~DriveCounter()
    {
        if (QueryHandle) {
            PdhCloseQuery(QueryHandle);
        }
    }

    bool DriveCounter::IsValid() const
    {
        return Valid;
    }

    DriveCounter::Data DriveCounter::GetData()
    {
        CollectData();
        return DriveCounter::Data{
            .PercentDiskTime = GetValue<double>(PercentDiskTime),
            .TransfersPerSec = GetValue<double>(DiskTransfersPerSec),
            .BytesPerTransfer = GetValue<double>(AvgDiskBytesPerRead),
            .BytesPerSec = GetValue<double>(DiskReadBytesPerSec),
            .AvgDiskQueueLength = GetValue<double>(AvgDiskQueueLength),
            .CurrentDiskQueueLength = GetValue<long>(CurrentDiskQueueLength)
        };
    }

    void* DriveCounter::AddCounter(const char* Name)
    {
        PDH_HCOUNTER Counter;
        auto Error = PdhAddCounter(QueryHandle, Name, NULL, &Counter);
        if (Error != ERROR_SUCCESS) {
            printf("PdhAddCounter failed, GLE=%d.\n", Error);
            return nullptr;
        }
        return Counter;
    }

    void DriveCounter::CollectData()
    {
        auto Error = PdhCollectQueryData(QueryHandle);
        if (Error != ERROR_SUCCESS) {
            printf("PdhCollectQueryData failed, GLE=%d.\n", Error);
        }
    }

    template<class T>
    T DriveCounter::GetValue(void* Counter)
    {
        PDH_FMT_COUNTERVALUE CounterValue{};
        constexpr static DWORD ReqType =
            std::is_same_v<T, double> ? PDH_FMT_DOUBLE :
            (std::is_same_v<T, long long> ? PDH_FMT_LARGE :
            (std::is_same_v<T, long> ? PDH_FMT_LONG : 0));
        auto Error = PdhGetFormattedCounterValue(Counter, ReqType, NULL, &CounterValue);
        if (Error != ERROR_SUCCESS) {
            printf("PdhGetFormattedCounterValue failed, GLE=%x.\n", Error);
        }
        if (CounterValue.CStatus != ERROR_SUCCESS) {
            printf("PdhGetFormattedCounterValue failed, CStatus=%x.\n", CounterValue.CStatus);
        }
        if constexpr (std::is_same_v<T, double>) {
            return CounterValue.doubleValue;
        }
        if constexpr (std::is_same_v<T, long long>) {
            return CounterValue.largeValue;
        }
        if constexpr (std::is_same_v<T, long>) {
            return CounterValue.longValue;
        }
    }
}