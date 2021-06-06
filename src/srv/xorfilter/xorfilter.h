#pragma once

// Taken from https://github.com/FastFilter/xor_singleheader
#include "fastfilter.h"

class XorFilter {
public:
    XorFilter() = default;

    XorFilter(XorFilter&) = delete;

    XorFilter(XorFilter&& Other) noexcept :
        Filter(Other.Filter)
    {
        Other.Filter.fingerprints = nullptr;
    }

    ~XorFilter() {
        xor16_free(&Filter);
    }

    bool Initialize(const uint64_t* Source, size_t Count) {
        if (!xor16_allocate(Count, &Filter)) {
            return false;
        }
        if (!xor16_buffered_populate(Source, Count, &Filter)) {
            return false;
        }
        return true;
    }

    __forceinline bool Contains(uint64_t Item) const noexcept {
        return xor16_contain(Item, &Filter);
    }

    __forceinline size_t Size() const noexcept {
        return xor16_size_in_bytes(&Filter);
    }

private:
    xor16_s Filter;
};
