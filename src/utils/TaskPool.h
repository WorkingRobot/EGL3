#pragma once

#include "Assert.h"
#include "Callback.h"

#include <condition_variable>
#include <future>
#include <shared_mutex>
#include <vector>

namespace EGL3::Utils {
    class TaskPool {
        enum class PoolState : uint8_t {
            Stopped,
            Running,
            Disposing
        };

    public:
        TaskPool(size_t Capacity) :
            State(PoolState::Stopped),
            Task([]() { return false; })
        {
            Workers.reserve(Capacity);
            for (auto i = 0; i < Capacity; ++i) {
                Workers.emplace_back(std::async(std::launch::async, &TaskPool::Worker, this));
            }
        }

        ~TaskPool() {
            UpdateState(PoolState::Disposing);

            for (auto& Worker : Workers) {
                if (Worker.valid()) {
                    Worker.get();
                }
            }
        }

        void Begin() {
            UpdateState(PoolState::Running);
        }

        void Pause() {
            UpdateState(PoolState::Stopped);
        }

        Utils::Callback<bool()> Task;

    private:
        void UpdateState(PoolState NewState) {
            {
                std::lock_guard Lock(StateMutex);
                State = NewState;
            }
            StateCV.notify_all();
        }

        void Worker() {
            do {
                std::shared_lock Lock(StateMutex);
                StateCV.wait(Lock, [this]() {
                    return State != PoolState::Stopped;
                });
                if (State == PoolState::Disposing) {
                    return;
                }
            } while (Task());
        }

        std::shared_mutex StateMutex;
        std::condition_variable_any StateCV;
        PoolState State;

        std::vector<std::future<void>> Workers;
    };
}