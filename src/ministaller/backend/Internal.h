#pragma once

#include "Backend.h"

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>

namespace EGL3::Installer::Backend {
    void* Alloc(size_t Count);
    void Free(void* Ptr);

    constexpr ZSTD_customMem ZstdAllocator{
        .customAlloc = [](void*, size_t Count) { return Alloc(Count); },
        .customFree = [](void*, void* Ptr) { return Free(Ptr); }
    };

    template <class T>
    struct Allocator {
        typedef T value_type;

        Allocator() = default;

        template <class U>
        constexpr Allocator(const Allocator<U>&) noexcept { }

        [[nodiscard]]
        T* allocate(size_t n) noexcept {
            return static_cast<T*>(Alloc(n * sizeof(T)));
        }

        void deallocate(T* p, size_t n) noexcept {
            Free(p);
        }
    };

    void Log(const char* Format, ...);
    void Log(const wchar_t* Format, ...);
}