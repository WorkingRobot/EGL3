#pragma once

#include "../../utils/mmio/MmioFile.h"

#include <compare>

namespace EGL3::Storage::Game {
    template<class T>
    class ArchiveRefConst {
    public:
        ArchiveRefConst() noexcept :
            Archive(nullptr),
            Offset()
        {

        }

        explicit ArchiveRefConst(Utils::Mmio::MmioFile& Archive, size_t Offset) noexcept :
            Archive(&Archive),
            Offset(Offset)
        {

        }

        const T& operator*() const noexcept {
            return *(T*)(Archive->Get() + Offset);
        }

        const T* operator->() const noexcept {
            return (T*)(Archive->Get() + Offset);
        }

        const T* Get() const noexcept {
            return (T*)(Archive->Get() + Offset);
        }

        const char* GetBase() const noexcept {
            return Archive->Get();
        }

        size_t GetOffset() const noexcept {
            return Offset;
        }

        std::strong_ordering operator<=>(const ArchiveRefConst& that) const noexcept {
            if (auto cmp = Offset <=> that.Offset; cmp != 0)
                return cmp;
            return Archive <=> that.Archive;
        }

    protected:
        Utils::Mmio::MmioFile* Archive;
        size_t Offset;
    };


    template<class T>
    class ArchiveRef : public ArchiveRefConst<T> {
    public:
        using Base = ArchiveRefConst<T>;

        using Base::Base;

        ArchiveRef(const Base& that) noexcept :
            Base(that)
        {

        }

        using Base::operator*;

        T& operator*() const noexcept {
            return *(T*)(Base::Archive->Get() + Base::Offset);
        }

        using Base::operator->;

        T* operator->() const noexcept {
            return (T*)(Base::Archive->Get() + Base::Offset);
        }

        using Base::Get;

        T* Get() const noexcept {
            return (T*)(Base::Archive->Get() + Base::Offset);
        }

        using Base::GetBase;

        char* GetBase() const noexcept {
            return Base::Archive->Get();
        }

        std::strong_ordering operator<=>(const ArchiveRef& that) const noexcept {
            if (auto cmp = Base::Offset <=> that.Offset; cmp != 0)
                return cmp;
            return Base::Archive <=> that.Archive;
        }
    };
}