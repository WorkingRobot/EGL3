#pragma once

#include "Header.h"
#include "RunlistElement.h"
#include "RunlistId.h"

#include "File.h"
#include "ChunkPart.h"
#include "ChunkInfo.h"

#include <array>
#include <numeric>

namespace EGL3::Storage::Game {
    template<uint32_t MaxRunCount>
    class Runlist {
    public:
        Runlist(RunlistId Id) :
            Id(Id),
            RunCount(0),
            AllocatedSize(0),
            Size(0),
            Runs()
        {

        }

        RunlistId GetId() const {
            return Id;
        }

        uint32_t GetRunCount() const {
            return RunCount;
        }

        uint64_t GetAllocatedSize() const {
            return AllocatedSize;
        }

        void SetAllocatedSize(uint64_t NewSize) {
            AllocatedSize = NewSize;
        }

        uint64_t GetSize() const {
            return Size;
        }

        void SetSize(uint64_t NewSize) {
            Size = NewSize;
        }

        const std::array<RunlistElement, MaxRunCount>& GetRuns() const {
            return Runs;
        }

        std::array<RunlistElement, MaxRunCount>& GetRuns() {
            return Runs;
        }

        static constexpr uint32_t GetMaxRunCount() {
            return MaxRunCount;
        }

        void ExtendLastRun(uint32_t Amount) {
            Runs[RunCount - 1].SectorCount += Amount;
        }

        bool EmplaceRun(uint32_t SectorOffset, uint32_t SectorCount) {
            if (RunCount >= MaxRunCount) {
                return false;
            }
            Runs[RunCount] = RunlistElement{
                .SectorOffset = SectorOffset,
                .SectorCount = SectorCount
            };
            RunCount++;
            return true;
        }

        // TODO: cache this value inside Archive somewhere to prevent slowdowns (profile first!)
        uint64_t GetAllocatedSectors() const {
            return std::accumulate(Runs.begin(), Runs.end(), 0ull, [](uint64_t Val, const RunlistElement& Run) {
                return Val + Run.SectorCount;
            });
        }

        // TODO: optimize to use some sort of binary search? I really want to make it fast
        // ByteOffset is the offset of the byte requested
        // RunIndex is the index inside the Runs array, RunByteOffset is the byte offset inside that run
        bool GetRunIndex(uint64_t ByteOffset, uint32_t& RunIndex, uint64_t& RunByteOffset) const {
            if (Size < ByteOffset) {
                return false;
            }

            for (RunIndex = 0; RunIndex < RunCount; ++RunIndex) {
                if (ByteOffset < Runs[RunIndex].SectorCount * Header::GetSectorSize()) {
                    RunByteOffset = ByteOffset;
                    return true;
                }
                ByteOffset -= Runs[RunIndex].SectorCount * Header::GetSectorSize();
            }
            return false;
        }

        bool IncrementRunState(uint32_t& RunIndex, uint64_t& RunByteOffset) const {
            for (; RunIndex < RunCount; ++RunIndex) {
                if (RunByteOffset < Runs[RunIndex].SectorCount * Header::GetSectorSize()) {
                    return true;
                }
                RunByteOffset -= Runs[RunIndex].SectorCount * Header::GetSectorSize();
            }
            return false;
        }

        size_t GetPosition(uint32_t RunIndex, uint64_t RunByteOffset) const {
            return Runs[RunIndex].SectorOffset * Header::GetSectorSize() + RunByteOffset;
        }

        size_t GetPosition(size_t Position) const {
            if (Position >= Size) {
                return -1;
            }

            uint32_t RunIndex;
            uint64_t RunByteOffset;
            if (GetRunIndex(Position, RunIndex, RunByteOffset)) {
                return GetPosition(RunIndex, RunByteOffset);
            }

            return -1;
        }

    private:
        RunlistId Id;                                   // Basically the id of the runlist (unique), used in the RunIndex
        uint32_t RunCount;                              // Number of runs in the list below
        uint64_t AllocatedSize;                         // Number of bytes that are allocated to the runlist data (like vector capacity)
        uint64_t Size;                                  // Number of bytes that are used in the runlist data (like vector size)
        std::array<RunlistElement, MaxRunCount> Runs;   // List of runs showing where the data can be found
    };

    template<RunlistId Id>
    struct RunlistTraits {

    };

    template<>
    struct RunlistTraits<RunlistId::File> {
        using Runlist = Runlist<1789>;
        using Data = File;

        static_assert(sizeof(Runlist) == 14336);
    };

    template<>
    struct RunlistTraits<RunlistId::ChunkPart> {
        using Runlist = Runlist<2045>;
        using Data = ChunkPart;

        static_assert(sizeof(Runlist) == 16384);
    };

    template<>
    struct RunlistTraits<RunlistId::ChunkInfo> {
        using Runlist = Runlist<2045>;
        using Data = ChunkInfo;

        static_assert(sizeof(Runlist) == 16384);
    };

    template<>
    struct RunlistTraits<RunlistId::ChunkData> {
        using Runlist = Runlist<2045>;
        using Data = char;

        static_assert(sizeof(Runlist) == 16384);
    };
}