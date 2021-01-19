#include "SHA.h"

#include <mbedtls/sha1.h>
#include <string.h>

namespace EGL3::Utils {
    void SHA1(const char* Buffer, size_t BufferSize, char Dst[20]) {
        mbedtls_sha1_ret((unsigned char*)Buffer, BufferSize, (unsigned char*)Dst);
    }

    bool SHA1Verify(const char* Buffer, size_t BufferSize, const char ExpectedHash[20]) {
        char BufferHash[20];
        SHA1(Buffer, BufferSize, BufferHash);
        return !memcmp(BufferHash, ExpectedHash, 20);
    }
}
