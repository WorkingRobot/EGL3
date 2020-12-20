#pragma once

#include "Http.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace EGL3::Web {
    class BaseClient {
    public:
        enum ErrorCode {
            SUCCESS,
            CANCELLED,
            INVALID_TOKEN,
            CODE_NOT_200,
            CODE_NOT_JSON,
            CODE_BAD_JSON
        };

        template<class T>
        class Response {
        public:
            bool HasError() const {
                return Error != SUCCESS;
            }

            ErrorCode GetErrorCode() const {
                return Error;
            }

            T* operator->() const {
                return Data.get();
            }

            Response() : Response(SUCCESS) {}
            Response(ErrorCode Error) : Error(Error), Data(nullptr) {}
            Response(T&& Data) : Error(SUCCESS), Data(std::make_unique<T>(std::forward<T&&>(Data))) {}

        private:
            ErrorCode Error;
            std::unique_ptr<T> Data;
        };

        // For 204 responses, or update requests (no useful data is returned)
        template<>
        class Response<void> {
        public:
            bool HasError() const {
                return Error != SUCCESS;
            }

            ErrorCode GetErrorCode() const {
                return Error;
            }

            Response() : Response(SUCCESS) {}
            Response(ErrorCode Error) : Error(Error) {}

        private:
            ErrorCode Error;
        };

        static cpr::Url FormatUrl(const char* Input) {
            return Input;
        }

        template<typename... Args>
        static cpr::Url FormatUrl(const char* Input, Args&&... FormatArgs) {
            auto BufSize = snprintf(nullptr, 0, Input, FormatArgs...) + 1;
            auto Buf = std::make_unique<char[]>(BufSize);
            snprintf(Buf.get(), BufSize, Input, FormatArgs...);
            return cpr::Url(Buf.get(), BufSize - 1);
        }

        ~BaseClient();

    protected:
        // Implementation is offered to derived classes
        // This is given as an alternative to a destructor
        // The base client's destructor handles the concurrency,
        // so we don't want the derived client's destructor to
        // invalidate its token before we know all of its requests
        // have ended.
        virtual void KillAuthentication() {}

        __forceinline const std::atomic_bool& GetCancelled() const {
            return Cancelled;
        }

        class RunningFunctionGuard {
        public:
            RunningFunctionGuard(BaseClient& Client) : Client(Client) {
                {
                    std::lock_guard lock(Client.RunningFunctionMutex);
                    Client.RunningFunctionCount++;
                }
                Client.RunningFunctionCV.notify_all();
            }

            ~RunningFunctionGuard() {
                {
                    std::lock_guard lock(Client.RunningFunctionMutex);
                    Client.RunningFunctionCount--;
                }
                Client.RunningFunctionCV.notify_all();
            }

        private:
            BaseClient& Client;
        };

    private:
        std::mutex RunningFunctionMutex;
        std::condition_variable RunningFunctionCV;
        int32_t RunningFunctionCount = 0;

        std::atomic_bool Cancelled;
    };
}
