#pragma once

#include "ArchiveRef.h"
#include "Runlist.h"

namespace EGL3::Storage::Game {
    // Must be ABI compatible with WIN32_MEMORY_RANGE_ENTRY
    struct PrefetchTableSrcEntry {
        void* VirtualAddress;
        size_t NumberOfBytes;
    };

    template<RunlistId Id>
    class ArchiveListIteratorReadonly {
    public:
        using T = typename RunlistTraits<Id>::Data;
        using List = typename RunlistTraits<Id>::Runlist;

        using ConstRef = const T&;
        using ConstPtr = const T*;

        using value_type = T;

        ArchiveListIteratorReadonly() = default;

        ArchiveListIteratorReadonly(const ArchiveRef<List>& Runlist, size_t Position) :
            Runlist(Runlist)
        {
            if (!this->Runlist->GetRunIndex(Position * sizeof(T), CurrentRunIdx, CurrentRunOffset)) {
                CurrentRunIdx = this->Runlist->GetRunCount();
                CurrentRunOffset = 0;
            }
        }

        ConstRef operator*() const noexcept {
            return *(ConstPtr)(Runlist.GetBase() + Runlist->GetPosition(CurrentRunIdx, CurrentRunOffset));
        }

        ConstPtr operator->() const noexcept {
            return (ConstPtr)(Runlist.GetBase() + Runlist->GetPosition(CurrentRunIdx, CurrentRunOffset));
        }

        ArchiveListIteratorReadonly& operator++() noexcept {
            CurrentRunOffset += sizeof(T);
            Runlist->IncrementRunState(CurrentRunIdx, CurrentRunOffset); // make sure it's true

            return *this;
        }

        ArchiveListIteratorReadonly operator++(int) noexcept {
            ArchiveListIteratorReadonly Tmp = *this;
            ++*this;
            return Tmp;
        }

        ArchiveListIteratorReadonly& operator+=(const size_t Off) noexcept {
            CurrentRunOffset += Off * sizeof(T);
            Runlist->IncrementRunState(CurrentRunIdx, CurrentRunOffset); // make sure it's true

           return *this;
        }

        ArchiveListIteratorReadonly operator+(const size_t Off) const noexcept {
            ArchiveListIteratorReadonly Tmp = *this;
            return Tmp += Off;
        }

        /*size_t operator-(const ArchiveListIteratorReadonly& Right) const noexcept {
            return Position - Right.Position;
        }*/

        ConstRef operator[](const size_t Off) const noexcept {
            return *(*this + Off);
        }

        std::strong_ordering operator<=>(const ArchiveListIteratorReadonly& that) const {
            if (auto cmp = CurrentRunIdx <=> that.CurrentRunIdx; cmp != 0)
                return cmp;
            if (auto cmp = CurrentRunOffset <=> that.CurrentRunOffset; cmp != 0)
                return cmp;
            return Runlist <=> that.Runlist;
        }

        bool operator==(const ArchiveListIteratorReadonly& that) const noexcept {
            return std::is_eq(*this <=> that);
        }

        bool operator!=(const ArchiveListIteratorReadonly& that) const noexcept {
            return std::is_neq(*this <=> that);
        }

        bool operator<(const ArchiveListIteratorReadonly& that) const noexcept {
            return std::is_lt(*this <=> that);
        }

        bool operator>(const ArchiveListIteratorReadonly& that) const noexcept {
            return std::is_gt(*this <=> that);
        }

        bool operator<=(const ArchiveListIteratorReadonly& that) const noexcept {
            return std::is_lteq(*this <=> that);
        }

        bool operator>=(const ArchiveListIteratorReadonly& that) const noexcept {
            return std::is_gteq(*this <=> that);
        }

        size_t pos() const noexcept {
            return Runlist->GetPosition(CurrentRunIdx, CurrentRunOffset);
        }

        template<std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
        void FastRead(T* Dest, size_t Count) const noexcept {
            Count *= sizeof(typename ArchiveListIteratorReadonly<Id>::T);

            auto Base = Runlist.GetBase();

            size_t RunIdx = CurrentRunIdx;
            size_t RunOff = CurrentRunOffset;
            while (Count) {
                size_t ReadAmt = std::min(Runlist->GetRunSize(RunIdx) - RunOff, Count);

                void* Ptr = Base + Runlist->GetPosition(RunIdx, 0) + RunOff;
                memcpy(Dest, Ptr, ReadAmt);

                RunOff = 0;
                RunIdx++;
                Count -= ReadAmt;
                Dest = (T*)((uint8_t*)Dest + ReadAmt);
            }
        }

        template<std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
        size_t QueueFastPrefetch(T* Dest, size_t Count, PrefetchTableSrcEntry* SrcTable, void** DstTable) const noexcept {
            Count *= sizeof(typename ArchiveListIteratorReadonly<Id>::T);

            auto Base = Runlist.GetBase();

            size_t RunIdx = CurrentRunIdx;
            size_t RunOff = CurrentRunOffset;
            size_t Idx = 0;
            while (Count) {
                size_t ReadAmt = std::min(Runlist->GetRunSize(RunIdx) - RunOff, Count);

                void* Ptr = Base + Runlist->GetPosition(RunIdx, 0) + RunOff;
                *SrcTable = { Ptr, ReadAmt };
                *DstTable = Dest;
                SrcTable++;
                DstTable++;
                Idx++;

                RunOff = 0;
                RunIdx++;
                Count -= ReadAmt;
                Dest = (T*)((uint8_t*)Dest + ReadAmt);
            }

            return Idx;
        }

    protected:
        ArchiveRef<List> Runlist;

        uint32_t CurrentRunIdx;
        uint64_t CurrentRunOffset;
    };
}