#pragma once

#include "../../storage/game/Archive.h"
#include "../MountedArchive.h"

namespace EGL3::Service::Pipe {
    static constexpr const char* PipeName = R"(\\.\pipe\EGL3Connection)";
    static constexpr uint32_t ProtocolVersion = 0x02;

    enum class MessageType : uint32_t {
        OpenArchive     = 0x00, // Open archive
        ReadArchive     = 0x01, // Read archive file metadata (and create MountedDisk)
        InitializeDisk  = 0x02, // Create MBR and filesystem
        CreateLUT       = 0x03, // Create LUT for archive files
        CreateDisk      = 0x04, // Create WinSpd disk
        MountDisk       = 0x05, // Mount WinSpd disk to system

        Destruct        = 0x10,

        QueryPath       = 0x20,
        QueryLetter     = 0x21,
        QueryStats      = 0x22,
        QueryServer     = 0x23,

        Error           = 0xFF
    };

    enum class ResponseStatus : uint8_t {
        Success,
        Failure,
        InvalidPath
    };

    template<MessageType TypeT>
    struct BaseMessage {
        MessageType Type = TypeT;
    };

    template<MessageType Type>
    struct Request : public BaseMessage<Type> {
        void* Context;
    };

    template<MessageType Type>
    struct Response : public BaseMessage<Type> {
        ResponseStatus Status;
    };

    template<>
    struct Request<MessageType::Error> : public BaseMessage<MessageType::Error> {

    };

    template<>
    struct Response<MessageType::Error> : public BaseMessage<MessageType::Error> {

    };

    template<>
    struct Request<MessageType::OpenArchive> : public BaseMessage<MessageType::OpenArchive> {
        wchar_t FilePath[384];
    };

    template<>
    struct Response<MessageType::OpenArchive> : public BaseMessage<MessageType::OpenArchive> {
        ResponseStatus Status;
        void* Context;
    };

    template<>
    struct Response<MessageType::QueryPath> : public BaseMessage<MessageType::QueryPath> {
        wchar_t FilePath[384];
    };

    template<>
    struct Response<MessageType::QueryLetter> : public BaseMessage<MessageType::QueryLetter> {
        char Letter;
    };

    template<>
    struct Response<MessageType::QueryStats> : public BaseMessage<MessageType::QueryStats> {
        MountedArchive::Stats Stats;
    };

    template<>
    struct Request<MessageType::QueryServer> : public BaseMessage<MessageType::QueryServer> {

    };

    template<>
    struct Response<MessageType::QueryServer> : public BaseMessage<MessageType::QueryServer> {
        uint32_t ProtocolVersion;
        uint32_t MountedDiskCount;
    };
}