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
