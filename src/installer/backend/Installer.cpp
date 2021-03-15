#include "Installer.h"

#include "../../utils/streams/FileStream.h"

#include <stdio.h>

namespace EGL3::Installer::Backend {
    Installer::Installer(const std::filesystem::path& TargetDirectory) :
        Stream(L"epic.gl", L"/installdata.eglu"),
        Reader(Stream, TargetDirectory),
        OnProgressUpdate(Reader.OnProgressUpdate),
        OnProgressError(Reader.OnProgressError)
    {
        
    }

    void Installer::Run()
    {
        Reader.Run();
    }

    void Installer::GetProgress(float& Value, Unpacker::State& State) const
    {
        Reader.GetProgress(Value, State);
    }

    const std::filesystem::path& Installer::GetLaunchPath() const
    {
        return Reader.GetLaunchPath();
    }
}