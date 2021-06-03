#pragma once

namespace EGL3::Modules {
    class BaseModule {
    public:
        BaseModule() = default;
        BaseModule(BaseModule&) = delete;

        virtual ~BaseModule() {};
    };
}