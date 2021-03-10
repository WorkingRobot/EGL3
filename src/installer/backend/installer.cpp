#include "installer.h"

#include "use_http.h"
#include "streams.h"
#include "registry.h"

#include <shlobj.h>

#define UNUSED_RETURN(x) __pragma(warning(suppress:6031)) x

namespace EGL3::Installer::Backend {
    InstallManager::InstallManager(const std::filesystem::path& InstallPath, const std::function<void()>& UpdateCallback, const std::function<void(const std::string&)>& ErrorCallback) :
        InstallLocation(InstallPath),
        CurrentState(InstallState::INIT_PINNING),
        Progress(0),
        UpdateCallback(UpdateCallback),
        ErrorCallback(ErrorCallback)
    {

    }

    void InstallManager::Run() {
        RunTask = std::async(std::launch::async, [this]() {
            UpdateState(InstallState::INIT_PINNING, 0, "");

            auto pinner = SSLPinner();
            if (!pinner.valid) {
                ErrorCallback("Could not create a secure connection.");
                return;
            }

            UpdateState(InstallState::INIT_STREAMS, .02f, "");

            Header header;
            bool error = false;
            ArchiveFileStream stream(InstallLocation, &header, [&error, this](const char* err) {
                error = true;
                ErrorCallback(err);
            });
            LZ4_DecompressionStream decompStream(&stream);

            httplib::SSLClient cli(DOWNLOAD_HOST);
            cli.set_compress(true);
            cli.set_ca_cert_path(pinner.filename);
            cli.enable_server_certificate_verification(true);
            cli.set_follow_location(true);

            UpdateState(InstallState::INIT_DOWNLOAD, .03f, "");
            bool started = false; // the html buffers a bit with the github html response, wait till the file magic comes through
            auto resp = cli.Get(DOWNLOAD_PATH, [&decompStream, &started, &header, &error, this](const char* data, uint64_t data_length) {
                if (!started) {
                    if (*((int*)data) == FILE_MAGIC) {
                        started = true;
                        header.InstallSize = *(((int*)data) + 1);
                        decompStream.recieve(data + 8, data_length - 8);
                    }
                    return true;
                }
                decompStream.recieve(data, data_length);
                if (error) {
                    return false;
                }
                return true;
                }, [&stream, &started, this](uint64_t len, uint64_t total) {
                    static int val = 1;
                    --val;
                    if (started && !val) {
                        UpdateState(InstallState::DOWNLOAD, .05f + (((double)len / total) * .9f), stream.filename);
                        val = 5;
                    }
                    return true;
                }
            );
            if (error) {
                return;
            }
            UpdateState(InstallState::REGISTRY, .95f, "");
            registry_uninstall();

            ProgramRegistryInfo uninstall_info;
            StartExe = InstallLocation / header.StartupExe;
            auto StartExeString = StartExe.string();
            uninstall_info.DisplayIcon = StartExeString;
            uninstall_info.DisplayName = DOWNLOAD_REPO;
            uninstall_info.DisplayVersion = DOWNLOAD_VERSION;
            uninstall_info.Publisher = DOWNLOAD_AUTHOR;
            uninstall_info.HelpLink = header.HelpLink;
            uninstall_info.InstallDate = registry_datetime();
            uninstall_info.InstallLocation = InstallLocation.string();
            uninstall_info.URLInfoAbout = header.AboutLink;
            uninstall_info.URLUpdateInfo = header.PatchNotesLink;
            uninstall_info.EstimatedSize = header.InstallSize;// / 1024;
            uninstall_info.NoModify = 1;
            uninstall_info.NoRepair = 0;
            uninstall_info.ModifyPath = "\"" + StartExeString + "\" " + header.ModifyPath;
            uninstall_info.UninstallString = "\"" + StartExeString + "\" " + header.UninstallString;
            registry_install(&uninstall_info);

            UpdateState(InstallState::SHORTCUT, .98f, "");
            fs::path shortcut_path = fs::path(getenv("PROGRAMDATA")) / "Microsoft\\Windows\\Start Menu\\Programs\\" DOWNLOAD_REPO ".lnk";
            fs::remove(shortcut_path); // remove if the file exists
            CreateShortcut(StartExeString, std::string(), shortcut_path.native(), std::string(), 1, InstallLocation.string(), StartExeString, 0);

            UpdateState(InstallState::DONE, 1, "");
        });
    }

    void InstallManager::UpdateState(InstallState State, float Progress, const std::string& StateInfo) {
        this->CurrentState = State;
        this->Progress = Progress;
        this->StateInfo = StateInfo;
        UpdateCallback();
    }

    void InstallManager::CreateShortcut(const std::string& Target, const std::string& TargetArgs, const std::wstring& Output, const std::string& Description, int ShowMode, const std::string& CurrentDirectory, const std::string& IconFile, int IconIndex)
    {
        IShellLink* pShellLink;
        IPersistFile* pPersistFile;
        UNUSED_RETURN(CoInitialize(NULL));
        if (ShowMode >= 0 && IconIndex >= 0) {
            if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink))) {
                pShellLink->SetPath(Target.c_str());
                pShellLink->SetArguments(TargetArgs.c_str());
                if (Description.size() > 0) {
                    pShellLink->SetDescription(Description.c_str());
                }
                if (ShowMode > 0) {
                    pShellLink->SetShowCmd(ShowMode);
                }
                if (CurrentDirectory.size() > 0) {
                    pShellLink->SetWorkingDirectory(CurrentDirectory.c_str());
                }
                if (IconFile.size() > 0 && IconIndex >= 0) {
                    pShellLink->SetIconLocation(IconFile.c_str(), IconIndex);
                }
                if (SUCCEEDED(pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile))) {
                    pPersistFile->Save(Output.c_str(), TRUE);
                    pPersistFile->Release();
                }
                pShellLink->Release();
            }
        }
        CoUninitialize();
    }
}