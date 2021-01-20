#pragma once

#include "Header.h"
#include "RunlistElement.h"
#include "RunlistId.h"

#include <array>
#include <numeric>

namespace EGL3::Storage::Game {
    template<uint32_t MaxRunCount>
    class Runlist {
    public:
        Runlist(RunlistId Id) :
            AllocationId(Id),
            RunCount(0),
            ValidSize(0)
        {

        }

        RunlistId GetId() const {
            return AllocationId;
        }

        uint32_t GetRunCount() const {
            return RunCount;
        }

        uint64_t GetValidSize() const {
            return ValidSize;
        }

        void SetValidSize(uint64_t NewSize) {
            ValidSize = NewSize;
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
            Runs[RunCount] = RunlistElement{ .SectorOffset = SectorOffset, .SectorCount = SectorCount };
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
        bool GetRunIndex(uint64_t ByteOffset, uint32_t& RunIndex, uint32_t& RunByteOffset) const {
            if (ValidSize <= ByteOffset) {
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

    private:
        RunlistId AllocationId;                         // Basically the id of the runlist (unique), used in the RunIndex
        uint32_t RunCount;                              // Number of runs in the list below
        uint64_t ValidSize;                             // Number of bytes that are allocated to the runlist data
        std::array<RunlistElement, MaxRunCount> Runs;   // List of runs showing where the data can be found
    };
}