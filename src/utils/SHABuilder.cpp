#include "SHABuilder.h"

namespace EGL3::Utils {
    SHA1Builder::SHA1Builder() :
        ErrorCode(0)
    {
        mbedtls_sha1_init(&Ctx);
        CheckError(mbedtls_sha1_starts_ret(&Ctx));
    }

    SHA1Builder::~SHA1Builder()
    {
        mbedtls_sha1_free(&Ctx);
    }

    void SHA1Builder::Update(const char* Input, size_t InputSize)
    {
        CheckError(mbedtls_sha1_update_ret(&Ctx, (unsigned char*)Input, InputSize));
    }

    void SHA1Builder::Finish(char Out[20])
    {
        CheckError(mbedtls_sha1_finish_ret(&Ctx, (unsigned char*)Out));
    }

    bool SHA1Builder::HasError() const
    {
        return ErrorCode;
    }

    int SHA1Builder::GetError() const
    {
        return ErrorCode;
    }

    void SHA1Builder::CheckError(int ErrorCode)
    {
        // Set error code if there was an error and it's not overwriting an old error
        if (!this->ErrorCode && ErrorCode) {
            this->ErrorCode = ErrorCode;
        }
    }
}
