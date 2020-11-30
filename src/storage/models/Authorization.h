#pragma once

#include "../../utils/Encrypt.h"
#include "../../utils/streams/BufferStream.h"
#include "../../utils/streams/MemoryStream.h"

namespace EGL3::Storage::Models {
    struct Authorization {
        std::string AccountId;
        std::string DeviceId;
        std::string Secret;

        Authorization() = default;

        Authorization(const std::string& AccountId, const std::string& DeviceId, const std::string& Secret) :
            AccountId(AccountId),
            DeviceId(DeviceId),
            Secret(Secret)
        { }

        friend Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, Authorization& Val) {
            uint32_t DataSize;
            Stream >> DataSize;
            auto Data = std::unique_ptr<char[]>(new char[DataSize]);
            Stream.read(Data.get(), DataSize);

            size_t DecryptedSize;
            auto DecryptedData = Utils::Decrypt(Data.get(), DataSize, DecryptedSize);

            Utils::Streams::BufferStream DecryptedStream(DecryptedData.get(), DecryptedSize);

            DecryptedStream >> Val.AccountId;
            DecryptedStream >> Val.DeviceId;
            DecryptedStream >> Val.Secret;

            return Stream;
        }

        friend Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const Authorization& Val) {
            Utils::Streams::MemoryStream DecryptedStream;

            DecryptedStream << Val.AccountId;
            DecryptedStream << Val.DeviceId;
            DecryptedStream << Val.Secret;

            size_t EncryptedSize;
            auto EncryptedData = Utils::Encrypt(DecryptedStream.get(), DecryptedStream.size(), EncryptedSize);

            Stream << (uint32_t)EncryptedSize;
            Stream.write(EncryptedData.get(), EncryptedSize);

            return Stream;
        }
    };
}