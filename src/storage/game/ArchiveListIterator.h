#pragma once

#include "../../utils/Assert.h"
#include "ArchiveListIteratorReadonly.h"

namespace EGL3::Storage::Game {
    template<RunlistId Id>
    class ArchiveListIterator : public ArchiveListIteratorReadonly<Id> {
    public:
        using Base = ArchiveListIteratorReadonly<Id>;

        using T = typename Base::T;
        using List = typename Base::List;

        using ConstRef = typename Base::ConstRef;
        using ConstPtr = typename Base::ConstPtr;

        using Ref = T&;
        using Ptr = T*;

        using value_type = T;

        using Base::Base;

        Ref operator*() noexcept {
            auto Pos = Base::Runlist->GetPosition(Base::CurrentRunIdx, Base::CurrentRunOffset);
            EGL3_CONDITIONAL_LOG(Pos, LogLevel::Critical, "Writing to index 0");
            return *(Ptr)(Base::Runlist.GetBase() + Pos);
        }

        Ptr operator->() noexcept {
            auto Pos = Base::Runlist->GetPosition(Base::CurrentRunIdx, Base::CurrentRunOffset);
            EGL3_CONDITIONAL_LOG(Pos, LogLevel::Critical, "Writing to index 0");
            return (Ptr)(Base::Runlist.GetBase() + Pos);
        }

        ArchiveListIterator& operator++() noexcept {
            Base::operator++();
            return *this;
        }

        ArchiveListIterator operator++(int) noexcept {
            ArchiveListIterator Tmp = *this;
            Base::operator++();
            return Tmp;
        }

        ArchiveListIterator& operator+=(const size_t Off) noexcept {
           Base::operator+=(Off);
           return *this;
        }

        ArchiveListIterator operator+(const size_t Off) const noexcept {
            ArchiveListIterator Tmp = *this;
            return Tmp += Off;
        }

        //using Base::operator-;

        Ref operator[](const size_t Off) const noexcept {
            return (Ref)(Base::operator[](Off));
        }

        std::strong_ordering operator<=>(const ArchiveListIterator& that) const {
            return (Base&)(*this) <=> (Base&)that;
        }

        bool operator==(const ArchiveListIterator& that) const noexcept {
            return std::is_eq(*this <=> that);
        }

        bool operator!=(const ArchiveListIterator& that) const noexcept {
            return std::is_neq(*this <=> that);
        }

        bool operator<(const ArchiveListIterator& that) const noexcept {
            return std::is_lt(*this <=> that);
        }

        bool operator>(const ArchiveListIterator& that) const noexcept {
            return std::is_gt(*this <=> that);
        }

        bool operator<=(const ArchiveListIterator& that) const noexcept {
            return std::is_lteq(*this <=> that);
        }

        bool operator>=(const ArchiveListIterator& that) const noexcept {
            return std::is_gteq(*this <=> that);
        }
    };
}