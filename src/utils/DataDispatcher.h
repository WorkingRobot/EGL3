#pragma once

#include <deque>
#include <mutex>

#include <glibmm/dispatcher.h>

namespace EGL3::Utils {
    template<typename... ArgsT>
    struct DataDispatcher {
    public:
        DataDispatcher() = default;

        template<class FuncT>
        DataDispatcher(FuncT&& Func)
        {
            Dispatcher.connect([this, Func]() {
                std::apply(Func, Args);
            });
        }

        template<class FuncT>
        sigc::connection connect(FuncT&& Func)
        {
            return Dispatcher.connect([this, Func]() {
                std::apply(Func, Args);
            });
        }

        template<class... TupleArgsT>
        void emit(TupleArgsT&&... TupleArgs)
        {
            Args = std::tuple<ArgsT...>(std::forward<TupleArgsT>(TupleArgs)...);
            Dispatcher.emit();
        }

    private:
        Glib::Dispatcher Dispatcher;
        std::tuple<ArgsT...> Args;
    };

    template<typename... ArgsT>
    struct DataQueueDispatcher {
    public:
        DataQueueDispatcher() = default;

        template<class FuncT>
        DataQueueDispatcher(FuncT&& Func)
        {
            Dispatcher.connect([this, Func]() {
                std::lock_guard Guard(Mutex);
                for (auto& Args : ArgsQueue) {
                    std::apply(Func, Args);
                }
                ArgsQueue.clear();
            });
        }

        template<class FuncT>
        sigc::connection connect(FuncT&& Func)
        {
            return Dispatcher.connect([this, Func]() {
                std::lock_guard Guard(Mutex);
                for (auto& Args : ArgsQueue) {
                    std::apply(Func, Args);
                }
                ArgsQueue.clear();
            });
        }

        template<class... TupleArgsT>
        void emit(TupleArgsT&&... TupleArgs)
        {
            std::unique_lock Lock(Mutex);
            ArgsQueue.emplace_back(std::forward<TupleArgsT>(TupleArgs)...);
            Lock.unlock();

            Dispatcher.emit();
        }

    private:
        Glib::Dispatcher Dispatcher;
        std::mutex Mutex;
        std::deque<std::tuple<ArgsT...>> ArgsQueue;
    };
}