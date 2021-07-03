#pragma once

#include "../../utils/Callback.h"
#include "../BaseClient.h"
#include "../Http.h"
#include "../Response.h"
#include "friends/UserPresence.h"
#include "responses/OAuthToken.h"

namespace EGL3::Web::Epic {
    class EpicClientEOS : public BaseClient {
    public:
        EpicClientEOS(const Responses::OAuthToken& AuthData, const cpr::Authentication& AuthClient, const std::string& DeploymentId);
        EpicClientEOS(EpicClientEOS&&);

        ~EpicClientEOS();

        // Presence service

        Response<void> SetPresence(const std::string& ConnectionId, const Friends::UserPresence& Presence);


        const Responses::OAuthToken& GetAuthData() const;

        void SetKillTokenOnDestruct(bool Value = true);

        Utils::Callback<void(const Responses::OAuthToken&)> OnRefresh;

    private:
        bool EnsureTokenValidity();

        template<typename ResponseType, int SuccessStatusCode, bool RequiresAccount, class CallFunctorType>
        Response<ResponseType> Call(CallFunctorType&& CallFunctor);

        std::mutex TokenValidityMutex;
        
        Responses::OAuthToken AuthData;
        std::string AuthHeader;
        cpr::Authentication AuthClient; // used for refresh token auth
        std::string DeploymentId;
        bool KillTokenOnDestruct;
    };
}
