#pragma once

#include "ArchiveRef.h"
#include "Runlist.h"

namespace EGL3::Storage::Game {
    template<RunlistId Id>
    class ArchiveListIteratorReadonly {
    public:
        using T = typename RunlistTraits<Id>::Data;
        using List = typename RunlistTraits<Id>::Runlist;

        using ConstRef = const T&;
        using ConstPtr = const T*;

        using value_type = T;

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

    protected:
        ArchiveRef<List> Runlist;

        uint32_t CurrentRunIdx;
        uint32_t CurrentRunOffset;
    };
}