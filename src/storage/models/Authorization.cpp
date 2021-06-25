#include "Authorization.h"

#include "../../utils/Log.h"
#include "../../utils/Encrypt.h"
#include "../../utils/streams/BufferStream.h"
#include "../../utils/streams/MemoryStream.h"

namespace EGL3::Storage::Models {
    enum class DataVersion : uint16_t {
        Legacy,
        Initial,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };

    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, AuthUserData& Val)
    {
        Stream >> Val.AccountId;
        Stream >> Val.DisplayName;
        Stream >> Val.KairosAvatar;
        Stream >> Val.KairosBackground;
        Stream >> Val.RefreshToken;
        Stream >> Val.RefreshExpireTime;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const AuthUserData& Val)
    {
        Stream << Val.AccountId;
        Stream << Val.DisplayName;
        Stream << Val.KairosAvatar;
        Stream << Val.KairosBackground;
        Stream << Val.RefreshToken;
        Stream << Val.RefreshExpireTime;

        return Stream;
    }

    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, Authorization& Val) {
        uint16_t LegacyDataSize;
        Stream >> LegacyDataSize;
        DataVersion Version;
        Stream >> Version;
        if (LegacyDataSize != 0 || Version == DataVersion::Legacy) {
            Stream.seek(LegacyDataSize, Utils::Streams::Stream::SeekPosition::Cur);
            return Stream;
        }
        if (Version > DataVersion::Initial) {
            EGL3_ABORTF("Data version ({:04x}) is too new, can't read. If you want to go back a version, don't use the same storage backend.", (uint16_t)Version);
            return Stream;
        }

        size_t EncDataSize;
        Stream >> EncDataSize;

        Stream >> Val.SelectedUserIdx;

        auto EncData = std::make_unique<char[]>(EncDataSize);
        Stream.read(EncData.get(), EncDataSize);

        size_t DecDataSize;
        auto DecData = Utils::Decrypt(EncData.get(), EncDataSize, DecDataSize);

        Utils::Streams::BufferStream DecStream(DecData.get(), DecDataSize);

        DecStream >> Val.UserData;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const Authorization& Val) {
        Utils::Streams::MemoryStream DecStream;

        DecStream << Val.UserData;

        size_t EncDataSize;
        auto EncData = Utils::Encrypt(DecStream.get(), DecStream.size(), EncDataSize);

        Stream << (uint16_t)0;
        Stream << DataVersion::Latest;
        Stream << EncDataSize;
        Stream << Val.SelectedUserIdx;
        Stream.write(EncData.get(), EncDataSize);

        return Stream;
    }

    bool Authorization::IsUserSelected() const
    {
        return SelectedUserIdx != InvalidUserIdx && SelectedUserIdx < UserData.size();
    }

    const AuthUserData& Authorization::GetSelectedUser() const
    {
        return UserData[SelectedUserIdx];
    }

    AuthUserData& Authorization::GetSelectedUser()
    {
        return UserData[SelectedUserIdx];
    }

    void Authorization::SetSelectedUser(uint32_t Idx)
    {
        SelectedUserIdx = Idx;
    }

    void Authorization::ClearSelectedUser()
    {
        SetSelectedUser(InvalidUserIdx);
    }

    const std::deque<AuthUserData>& Authorization::GetUsers() const
    {
        return UserData;
    }

    std::deque<AuthUserData>& Authorization::GetUsers()
    {
        return UserData;
    }
}