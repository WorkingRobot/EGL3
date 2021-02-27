#pragma once

namespace EGL3::Service {
    class DriveCounter {
    public:
        DriveCounter();

        DriveCounter(char DriveLetter);

        ~DriveCounter();

        bool IsValid() const;

        struct Data {
            double PercentDiskTime;
            double TransfersPerSec;
            double BytesPerTransfer;
            double BytesPerSec;
            double AvgDiskQueueLength;
            long CurrentDiskQueueLength;
        };

        Data GetData();

    private:
        void* AddCounter(const char* Name);

        void CollectData();

        template<class T>
        T GetValue(void* Counter);

        bool Valid;
        void* QueryHandle;

        void* CurrentDiskQueueLength;
        void* PercentDiskTime;
        void* AvgDiskQueueLength;
        void* DiskTransfersPerSec;
        void* AvgDiskBytesPerRead;
        void* DiskReadBytesPerSec;
    };
}