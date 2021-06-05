#pragma once

#include "Log.h"
#include "Callback.h"

#include <condition_variable>
#include <future>
#include <shared_mutex>
#include <vector>

namespace EGL3::Utils {
    class TaskPool {
    public:
        enum class PoolState : uint8_t {
            Stopped,
            Running,
            Completed,
            Disposing
        };

        TaskPool(size_t Capacity) :
            Task([]() { return false; }),
            State(PoolState::Stopped),
            WorkerCount(0)
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

        void SetRunning(bool Running = true) {
            UpdateState(Running ? PoolState::Running : PoolState::Stopped);
        }

        PoolState GetState() const {
            std::shared_lock Lock(StateMutex);
            return State;
        }

        void WaitUntilResumed() const {
            std::shared_lock Lock(StateMutex);
            StateCV.wait(Lock, [this]() {
                return State != PoolState::Stopped;
            });
        }

        // returns true if stopped before the timeout, returns false if timeout completed
        template <class Clock, class Duration>
        bool WaitUntilFinished(const std::chrono::time_point<Clock, Duration>& Timeout) const {
            std::unique_lock Lock(WorkerMutex);
            return WorkerCV.wait_until(Lock, Timeout, [this]() {
                return WorkerCount == 0;
            });
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
            {
                std::lock_guard Guard(WorkerMutex);
                ++WorkerCount;
            }
            WorkerCV.notify_all();

            do {
                std::shared_lock Lock(StateMutex);
                StateCV.wait(Lock, [this]() {
                    return State != PoolState::Stopped;
                });
                if (State == PoolState::Disposing) {
                    return;
                }
            } while (Task());
            UpdateState(PoolState::Completed);

            {
                std::lock_guard Guard(WorkerMutex);
                --WorkerCount;
            }
            WorkerCV.notify_all();
        }

        mutable std::shared_mutex StateMutex;
        mutable std::condition_variable_any StateCV;
        PoolState State;

        mutable std::condition_variable WorkerCV;
        mutable std::mutex WorkerMutex;
        size_t WorkerCount;

        std::vector<std::future<void>> Workers;
    };
}