#pragma once

#include "../http.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace EGL3::Web {
    class BaseClient {
    public:
        enum ErrorCode {
            ERROR_SUCCESS,
            ERROR_CANCELLED,
            ERROR_INVALID_TOKEN,
            ERROR_CODE_NOT_200,
            ERROR_CODE_NOT_JSON,
            ERROR_CODE_BAD_JSON
        };

        template<class T>
        class Response {
        public:
            bool HasError() const {
                return Error != ERROR_SUCCESS;
            }

            ErrorCode GetErrorCode() const {
                return Error;
            }

            T* operator->() const {
                return Data.get();
            }

            Response(ErrorCode Error) : Error(Error), Data(nullptr) {}
            Response(T&& Data) : Error(ERROR_SUCCESS), Data(std::make_unique<T>(std::forward<T&&>(Data))) {}

        private:
            ErrorCode Error;
            std::unique_ptr<T> Data;
        };

        ~BaseClient();

    protected:
        virtual void KillAuthentication() {
            // Implementation is offered to derived class
            // This is kind given as an alternative to a destructor
            // The base client's destructor handles the concurrency,
            // so we don't want the derived client's destructor to
            // invalidate its token before we know all of its requests
            // have ended.
        }

        __forceinline const std::atomic_bool& GetCancelled() const {
            return Cancelled;
        }

        class RunningFunctionGuard {
        public:
            RunningFunctionGuard(BaseClient& Client) : Client(Client) {
                {
                    std::lock_guard<std::mutex> lock(Client.RunningFunctionMutex);
                    Client.RunningFunctionCount++;
                }
                Client.RunningFunctionCV.notify_all();
            }

            ~RunningFunctionGuard() {
                {
                    std::lock_guard<std::mutex> lock(Client.RunningFunctionMutex);
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
        int32_t RunningFunctionCount;

        std::atomic_bool Cancelled;
    };
}
