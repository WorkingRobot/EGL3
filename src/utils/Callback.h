#pragma once

#include "Assert.h"

#include <functional>
#include <optional>

namespace EGL3::Utils {
    template<class Func, class Result = typename std::function<Func>::result_type>
    struct Callback {
        Callback() = default;

        template<class... ArgsT>
        Callback(ArgsT&&... Args) :
            Function(std::forward<ArgsT>(Args)...)
        {

        }

        template<typename... ArgsT>
        void Set(ArgsT&&... Args) {
            Function.emplace(std::forward<ArgsT>(Args)...);
        }

        void Clear() {
            Function.reset();
        }

        template<class... ArgsT>
        constexpr Result operator()(ArgsT&&... Args) const {
            if (Function.has_value()) {
                return Function.value()(std::forward<ArgsT>(Args)...);
            }
            // If we expect a non-void return value, the callback must not be empty
            if constexpr (!std::is_void_v<Result>) {
                EGL3_LOG(LogLevel::Critical, "Required callback is empty");
            }
        }

    private:
        std::optional<std::function<Func>> Function;
    };
}