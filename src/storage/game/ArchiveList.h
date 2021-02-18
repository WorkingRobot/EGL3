#pragma once

#include "Archive.h"
#include "ArchiveListIterator.h"
#include "ArchiveListIteratorReadonly.h"

namespace EGL3::Storage::Game {
    template<RunlistId Id>
    class ArchiveList {
    public:
        using T = typename RunlistTraits<Id>::Data;
        using List = typename RunlistTraits<Id>::Runlist;

        using ConstRef = const T&;
        using ConstPtr = const T*;
        using ConstItr = ArchiveListIteratorReadonly<Id>;

        using Ref = T&;
        using Ptr = T*;
        using Itr = ArchiveListIterator<Id>;

        static_assert((Header::GetSectorSize() % sizeof(T)) == 0, "Value type must be a multiple of the sector. This is done to prevent some elements being partially between sectors (and potentially being fragmented)");

        ArchiveList(Archive& Archive) :
            Runlist(Archive.GetRunlist<Id>()),
            Archive(Archive)
        {

        }

        ConstRef operator[](size_t pos) const {
            return *(ConstPtr)(Runlist.GetBase() + Runlist->GetPosition(pos * sizeof(T)));
        }

        Ref operator[](size_t pos) {
            return *(Ptr)(Runlist.GetBase() + Runlist->GetPosition(pos * sizeof(T)));
        }

        ConstItr begin() const noexcept {
            return ConstItr(Runlist, 0);
        }
        
        Itr begin() noexcept {
            return Itr(Runlist, 0);
        }

        ConstItr cbegin() const noexcept {
            return begin();
        }

        ConstItr end() const noexcept {
            return ConstItr(Runlist, size());
        }

        Itr end() noexcept {
            return Itr(Runlist, size());
        }

        ConstItr cend() const noexcept {
            return end();
        }

        bool empty() const noexcept {
            return begin() == end();
        }

        size_t size() const noexcept {
            return Runlist->GetSize() / sizeof(T);
        }

        void reserve(size_t new_cap) {
            if (capacity() < new_cap) {
                reserve_internal(new_cap);
            }
        }

        size_t capacity() const noexcept {
            return Runlist->GetAllocatedSize() / sizeof(T);
        }

        void shrink_to_fit() {
            if (capacity() != size()) {
                reserve_internal(size());
            }
        }

        void clear() noexcept {
            resize(0);
        }

        template<class... Args>
        Ref emplace_back(Args&&... args) {
            resize_internal(size() + 1);
            auto pos = begin() + (size() - 1);
            std::construct_at(&*pos, std::forward<Args>(args)...);
            return *pos;
        }

        void resize(size_t count) {
            auto OldSize = size();
            if (count == OldSize) {
                return;
            }
            else if (count < OldSize) {
                std::destroy(begin() + count, end());
                resize_internal(count);
            }
            else if (count > OldSize) {
                resize_internal(count);
                std::uninitialized_default_construct(begin() + OldSize, end());
            }
        }

        void resize(size_t count, const T& value) {
            auto OldSize = size();
            if (count < OldSize) {
                std::destroy(begin() + count, end());
                resize_internal(count);
            }
            else if (count > OldSize) {
                resize_internal(count);
                std::uninitialized_fill(begin() + OldSize, end(), value);
            }
        }

        void flush(Itr begin, size_t count) {
            size_t BeginPos = begin.pos();
            Archive.FlushRunlist(*Runlist, BeginPos, count * sizeof(T));
        }

    private:
        void resize_internal(size_t count) {
            Archive.Resize(Runlist, count * sizeof(T));
        }

        void reserve_internal(size_t count) {
            Archive.Reserve(Runlist, count * sizeof(T));
        }

        ArchiveRef<List> Runlist;
        Archive& Archive;
    };
}