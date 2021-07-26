#include "FriendBase.h"

#include "../../modules/Friends/KairosMenu.h"
#include "../../utils/Crc32.h"

namespace EGL3::Storage::Models {
    const std::string& FriendBase::GetDisplayName() const {
        if (GetNickname().empty()) {
            return GetUsername();
        }
        return GetNickname();
    }

    const std::string& FriendBase::GetSecondaryName() const {
        if (GetNickname().empty()) {
            return GetNickname(); // Returns ""
        }
        return GetUsername();
    }
}