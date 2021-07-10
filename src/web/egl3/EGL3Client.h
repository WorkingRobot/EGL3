#pragma once

#include "../BaseClient.h"
#include "../Http.h"
#include "../Response.h"
#include "responses/VersionInfo.h"

namespace EGL3::Web::EGL3 {
    class EGL3Client : public BaseClient {
    public:
        Response<Responses::VersionInfo> GetLatestVersion();

        Response<std::string> GetInstaller();

    private:
        template<typename ResponseType, int SuccessStatusCode, class CallFunctorType>
        Response<ResponseType> Call(CallFunctorType&& CallFunctor);
    };
}