#include "Internal.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

const void* memchr(const void* s, int c, size_t n)
{
    auto p = (const unsigned char*)s;
    while (n-- != 0) {
        if ((unsigned char)c == *p++) {
            return (const void*)(p - 1);
        }
    }
    return NULL;
}

void* memmove(void* dest, const void* src, size_t count)
{
    char* tmp;
    const char* s;

    if (dest <= src) {
        tmp = (char*)dest;
        s = (const char*)src;
        while (count--)
            *tmp++ = *s++;
    }
    else {
        tmp = (char*)dest;
        tmp += count;
        s = (const char*)src;
        s += count;
        while (count--)
            *--tmp = *--s;
    }
    return dest;
}

int memcmp(const void* cs, const void* ct, size_t count)
{
    int res = 0;

    for (auto su1 = (const unsigned char*)cs, su2 = (const unsigned char*)ct; 0 < count; ++su1, ++su2, count--)
        if ((res = *su1 - *su2) != 0)
            break;
    return res;
}

void* memset(void* s, int c, size_t count)
{
    auto xs = (char*)s;

    while (count--)
        *xs++ = c;
    return s;
}

void* memcpy(void* dest, const void* src, size_t count)
{
    auto tmp = (char*)dest;
    auto s = (const char*)src;

    while (count--)
        *tmp++ = *s++;
    return dest;
}

namespace std {
    [[noreturn]] void __cdecl _Xlength_error(const char*) {}
}

namespace EGL3::Installer::Backend {
    HANDLE Heap = INVALID_HANDLE_VALUE;
    void* Alloc(size_t Count)
    {
        if (Heap == INVALID_HANDLE_VALUE) {
            Heap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
        }
        return HeapAlloc(Heap, HEAP_NO_SERIALIZE, Count);
    }

    void Free(void* Ptr)
    {
        HeapFree(Heap, HEAP_NO_SERIALIZE, Ptr);
    }

    void Log(const char* Format, ...)
    {
        char Buffer[1024]{};

        va_list Args;
        va_start(Args, Format);
        auto Size = wvsprintfA(Buffer, Format, Args);
        va_end(Args);

        WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), Buffer, Size, NULL, NULL);
    }

    void Log(const wchar_t* Format, ...)
    {
        wchar_t Buffer[1024]{};

        va_list Args;
        va_start(Args, Format);
        auto Size = wvsprintfW(Buffer, Format, Args);
        va_end(Args);

        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), Buffer, Size * sizeof(wchar_t), NULL, NULL);
    }
}