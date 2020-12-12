#pragma once

#include "Friend.h"

namespace EGL3::Storage::Models {
    using namespace Web::Xmpp::Json;

    struct CurrentUser : public Friend {
    private:
        std::string KairosAvatar;

        std::string KairosBackground;

        std::string DisplayStatus;

        ShowStatus OnlineStatus;

    public:
        CurrentUser() : OnlineStatus(ShowStatus::Online) {}

        const std::string& GetKairosAvatar() const {
            if (KairosAvatar.empty()) {
                return Friend::GetKairosAvatar();
            }
            return KairosAvatar;
        }

        const std::string& GetKairosBackground() const {
            if (KairosBackground.empty()) {
                return Friend::GetKairosBackground();
            }
            return KairosBackground;
        }

        const std::string& GetStatus() const {
            if (DisplayStatus.empty()) {
                return Friend::GetStatus();
            }
            return DisplayStatus;
        }

        void SetKairosAvatar(const std::string& NewAvatar) {
            KairosAvatar = NewAvatar;
        }

        void SetKairosBackground(const std::string& NewBackground) {
            KairosBackground = NewBackground;
        }

        void SetDisplayStatus(const std::string& NewStatus) {
            DisplayStatus = NewStatus;
        }

        void SetOnlineStatus(ShowStatus NewStatus) {
            OnlineStatus = NewStatus;
        }

        Presence BuildPresence() const {
            Presence Ret;
            Ret.ShowStatus = OnlineStatus;
            Ret.Status.SetStatus(GetStatus());
            PresenceKairosProfile NewKairosProfile;
            NewKairosProfile.Avatar = GetKairosAvatar();
            NewKairosProfile.Background = GetKairosBackground();
            Ret.Status.SetKairosProfile(NewKairosProfile);
            return Ret;
        }
    };
}