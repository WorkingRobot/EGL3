#pragma once

#include "RunIndexElement.h"

#include <array>

namespace EGL3::Storage::Game {
    template<uint32_t MaxRunCount>
    class RunIndex {
    public:
        RunIndex() :
            NextAvailableSector(Header::GetFirstValidSector()),
            RunCount(0)
        {

        }

        uint32_t GetNextAvailableSector() const {
            return NextAvailableSector;
        }

        uint32_t GetRunCount() const {
            return RunCount;
        }

        const std::array<RunIndexElement, MaxRunCount>& GetRuns() const {
            return Runs;
        }

        void ExtendLastRun(uint32_t Amount) {
            Runs[RunCount - 1].SectorCount += Amount;
            NextAvailableSector += Amount;
        }

        bool EmplaceRun(uint32_t SectorCount, RunlistId RunlistId) {
            if (RunCount >= MaxRunCount) {
                return false;
            }
            Runs[RunCount] = RunIndexElement{ .SectorCount = SectorCount, .AllocationId = RunlistId };
            RunCount++;
            NextAvailableSector += SectorCount;
            return true;
        }

        static constexpr uint32_t GetMaxRunCount() {
            return MaxRunCount;
        }

    private:
        uint32_t NextAvailableSector;                   // Next available free sector
        uint32_t RunCount;                              // Number of elements in element array
        std::array<RunIndexElement, MaxRunCount> Runs;  // Accumulation of all run indexes in all runs
    };
}