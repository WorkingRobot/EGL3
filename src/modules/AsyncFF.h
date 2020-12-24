#pragma once

#include "BaseModule.h"

#include <future>
#include <mutex>

namespace EGL3::Modules {
    // Inspiration from https://github.com/chris0x44/detached-task
    class AsyncFFModule : public BaseModule {
        std::mutex FutureMutex;
        std::vector<std::future<void>> Futures;

    public:
        AsyncFFModule(size_t Capacity = 30) {
            Futures.reserve(Capacity);
        }

        void Enqueue(std::future<void>&& Future) {
            std::lock_guard Guard(FutureMutex);

            Futures.emplace_back(std::move(Future));

            if (Futures.size() == Futures.capacity()) {
                std::vector<std::future<void>> OldFutures;

                Futures.swap(OldFutures);
                Futures.reserve(OldFutures.capacity());

                Futures.emplace_back(std::async(std::launch::async, [Olds = std::move(OldFutures)]{ ; }));
            }
        }

        template<class FunctorType, class... FunctorArgs>
        void Enqueue(FunctorType&& Functor, FunctorArgs&&... Args) {
            Enqueue(std::async(std::launch::async, std::move(Functor), std::move(Args)...));
        }

        template<class FunctorType, class CallbackType, class... FunctorArgs>
        void EnqueueCallback(FunctorType&& Functor, CallbackType&& Callback, FunctorArgs&&... Args) {
            Enqueue(std::async(std::launch::async, [](FunctorType&& Functor, CallbackType&& Callback, FunctorArgs&&... Args) {
                if constexpr (std::is_void_v<std::invoke_result_t<FunctorType, FunctorArgs...>>) {
                    Functor(std::move(Args)...);
                    Callback();
                }
                else {
                    Callback(Functor(std::move(Args)...));
                }
            }, std::move(Functor), std::move(Callback), std::move(Args)...));
        }
    };
}