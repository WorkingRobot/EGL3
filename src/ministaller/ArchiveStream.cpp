#include "ArchiveStream.h"

namespace EGL3::Installer {
    bool ArchiveStream::HasError() const noexcept
    {
        return CurrentError != Error::Success;
    }

    ArchiveStream::Error ArchiveStream::GetError() const noexcept
    {
        return CurrentError;
    }

    void ArchiveStream::SetError(Error NewError) noexcept
    {
        OnError(ErrorCtx, CurrentError = NewError);
    }
}