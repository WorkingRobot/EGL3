#pragma once

#include <functional>
#include <optional>

namespace EGL3::Utils {
    // Callbacks that have a non void return type must have a default
    template<class Func, class Enable = void>
    struct Callback {
        using Result = typename std::function<Func>::result_type;

        template<class... ArgsT>
        Callback(ArgsT&&... Args) :
            Function(std::forward<ArgsT>(Args)...)
        {
            static_assert(sizeof...(ArgsT) != 0, "Callback with a return type must have a default return value");
        }

        template<class... ArgsT>
        void Set(ArgsT&&... Args) {
            static_assert(sizeof...(ArgsT) != 0, "Callback with a return type must have a default return value");

            Function = { std::forward<ArgsT>(Args)... };
        }

        template<class... ArgsT>
        constexpr Result operator()(ArgsT&&... Args) const {
            return Function(std::forward<ArgsT>(Args)...);
        }

    private:
        std::function<Func> Function;
    };

    template<class Func>
    struct Callback<Func, typename std::enable_if<std::is_void_v<typename std::function<Func>::result_type>>::type> {
        using Result = void;

        Callback()
        {

        }

        template<class... ArgsT>
        Callback(ArgsT&&... Args) :
            Function(std::forward<ArgsT>(Args)...)
        {

        }

        template<class... ArgsT>
        void Set(ArgsT&&... Args) {
            Function = { std::forward<ArgsT>(Args)... };
        }

        template<class... ArgsT>
        constexpr Result operator()(ArgsT&&... Args) const {
            if (Function) {
                Function(std::forward<ArgsT>(Args)...);
            }
        }

    private:
        std::function<Func> Function;
    };
}