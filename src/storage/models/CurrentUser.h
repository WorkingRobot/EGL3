#pragma once

#include "Friend.h"

namespace EGL3::Storage::Models {
    using namespace Web::Xmpp::Json;

    struct CurrentUser : public Friend {
    private:
        std::string KairosAvatar;

        std::string KairosBackground;

        std::optional<std::string> DisplayStatus;

        std::optional<ShowStatus> OnlineStatus;

    public:
        ShowStatus GetShowStatus() const override {
            if (OnlineStatus.has_value()) {
                return OnlineStatus.value();
            }
            return Friend::GetShowStatus();
        }

        const std::string& GetStatus() const override {
            if (DisplayStatus.has_value()) {
                return DisplayStatus.value();
            }
            return Friend::GetStatus();
        }

        const std::string& GetKairosAvatar() const override {
            if (KairosAvatar.empty()) {
                return Friend::GetKairosAvatar();
            }
            return KairosAvatar;
        }

        const std::string& GetKairosBackground() const override {
            if (KairosBackground.empty()) {
                return Friend::GetKairosBackground();
            }
            return KairosBackground;
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

        void SetShowStatus(ShowStatus NewStatus) {
            OnlineStatus = NewStatus;
        }

        Presence BuildPresence() const {
            Presence Ret;
            Ret.ShowStatus = GetShowStatus();
            Ret.Status.SetStatus(GetStatus());
            PresenceKairosProfile NewKairosProfile(GetKairosAvatar(), GetKairosBackground());
            Ret.Status.SetKairosProfile(NewKairosProfile);
            return Ret;
        }
    };
}