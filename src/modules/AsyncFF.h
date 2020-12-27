#pragma once

#include "BaseModule.h"

#include <future>
#include <mutex>
#include <vector>

namespace EGL3::Modules {
    // Inspiration from https://github.com/chris0x44/detached-task
    class AsyncFFModule : public BaseModule {
        std::mutex FutureMutex;
        std::vector<std::future<void>> Futures;

    public:
        AsyncFFModule(size_t Capacity = 30);

        void Enqueue(std::future<void>&& Future);

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