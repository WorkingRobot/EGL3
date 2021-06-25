#include "AsyncFF.h"

namespace EGL3::Modules {
    constexpr size_t Capacity = 32;

    AsyncFFModule::AsyncFFModule(ModuleList& Ctx) {
        Futures.reserve(Capacity);
    }

    void AsyncFFModule::Enqueue(std::future<void>&& Future) {
        std::lock_guard Guard(FutureMutex);

        Futures.emplace_back(std::move(Future));

        if (Futures.size() == Futures.capacity()) {
            std::vector<std::future<void>> OldFutures;

            Futures.swap(OldFutures);
            Futures.reserve(OldFutures.capacity());

            Futures.emplace_back(std::async(std::launch::async, [Olds = std::move(OldFutures)]{ ; }));
        }
    }
}