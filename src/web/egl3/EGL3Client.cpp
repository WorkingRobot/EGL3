#include "EGL3Client.h"

#include "../RunningFunctionGuard.h"

namespace EGL3::Web::EGL3 {
    Response<Responses::VersionInfo> EGL3Client::GetLatestVersion()
    {
        return Call<Responses::VersionInfo, 200>(
            [&]() {
                return Http::Get(
                    Http::FormatUrl<Host::EGL3>("installdata.json")
                );
            }
        );
    }

    Response<std::string> EGL3Client::GetInstaller()
    {
        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        auto Response = Http::Get(
            Http::FormatUrl<Host::EGL3>("installer.exe")
        );

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != 200) {
            return Response.status_code;
        }

        return std::move(Response.text);
    }

    template<typename ResponseType, int SuccessStatusCode, class CallFunctorType>
    Response<ResponseType> EGL3Client::Call(CallFunctorType&& CallFunctor) {
        RunningFunctionGuard Guard(Lock);

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        auto Response = std::invoke(std::move(CallFunctor));

        if (GetCancelled()) { return ErrorData::Status::Cancelled; }

        if (Response.status_code != SuccessStatusCode) {
            return Response.status_code;
        }

        if constexpr (std::is_void_v<ResponseType>) {
            return ErrorData::Status::Success;
        }
        else {
            auto RespJson = Http::ParseJson(Response);

            if (RespJson.HasParseError()) {
                return ErrorData::Status::NotJson;
            }

            ResponseType Resp;
            if (!ResponseType::Parse(RespJson, Resp)) {
                return ErrorData::Status::BadJson;
            }

            return Resp;
        }
    }
}