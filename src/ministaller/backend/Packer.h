#pragma once

#include "Backend.h"

namespace EGL3::Installer::Backend {
    class Packer {
    public:
        Packer(const char* Path, int CompressionLevel);

        ~Packer();

        bool IsValid() const;

        void Close();

        bool Initialize();

        bool AddCommand(const char* Command);

        bool AddRegistry(
            const char* DisplayName,
            const char* Publisher,
            const char* HelpLink,
            const char* URLUpdateInfo,
            const char* URLInfoAbout,
            const char* DisplayVersion
        );
        //bool AddCleanMsiInstall(const char* Path);

        bool AddDirectory(const char* Folder);

    private:
        size_t Tell();

        size_t PWriteLazy(const void* In, size_t Size, size_t Offset);
        size_t WriteLazy(const void* In, size_t Size);

        void PWrite(const void* In, size_t Size, size_t Offset)
        {
            auto BytesWritten = PWriteLazy(In, Size, Offset);
            if (BytesWritten != Size && IsValid()) {
                Close();
            }
        }

        void Write(const void* In, size_t Size)
        {
            auto BytesWritten = WriteLazy(In, Size);
            if (BytesWritten != Size && IsValid()) {
                Close();
            }
        }

        template<class T>
        void PWrite(const T& Out, size_t Offset)
        {
            PWrite(&Out, sizeof(Out), Offset);
        }

        template<class T>
        void Write(const T& Out)
        {
            Write(&Out, sizeof(Out));
        }

        void* Handle;
        void* CCtx;
    };
}