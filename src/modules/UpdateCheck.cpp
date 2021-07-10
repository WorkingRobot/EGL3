#include "UpdateCheck.h"

#include "../utils/streams/FileStream.h"
#include "../utils/Humanize.h"
#include "../utils/Version.h"
#include "../utils/SlotHolder.h"

#include <shellapi.h>

namespace EGL3::Modules {
    constexpr auto UpdateFrequency = std::chrono::minutes(30);

    UpdateCheckModule::UpdateCheckModule(ModuleList& Ctx) :
        Dialog(Ctx.GetWidget<Gtk::Dialog>("UpdateDialog")),
        DialogPatchNotes(Ctx.GetWidget<Gtk::Label>("UpdateDialogPatchNotes")),
        DialogDate(Ctx.GetWidget<Gtk::Label>("UpdateDialogDate")),
        DialogVersion(Ctx.GetWidget<Gtk::Label>("UpdateDialogVersion")),
        DialogVersionHR(Ctx.GetWidget<Gtk::Label>("UpdateDialogVersionHR")),
        DialogAccept(Ctx.GetWidget<Gtk::Button>("UpdateDialogAcceptButton")),
        DialogReject(Ctx.GetWidget<Gtk::Button>("UpdateDialogRejectButton")),
        Auth(Ctx.GetModule<Login::AuthModule>()),
        SysTray(Ctx.GetModule<SysTrayModule>()),
        LatestVersionNum(Utils::Version::GetVersionNum()),
        Cancelled(false),
        Future(std::async(std::launch::async, &UpdateCheckModule::BackgroundTask, this))
    {
        
    }

    UpdateCheckModule::~UpdateCheckModule()
    {
        {
            std::unique_lock Lock(Mutex);
            Cancelled = true;
        }
        CV.notify_all();
        Future.get();
    }

    void UpdateCheckModule::DisplayUpdate(const Web::EGL3::Responses::VersionInfo& VersionData)
    {
        auto CurrentTime = Web::TimePoint::clock::now();

        DialogPatchNotes.set_markup(VersionData.PatchNotes);
        DialogDate.set_text(Utils::Humanize(VersionData.ReleaseDate, CurrentTime));
        DialogDate.set_tooltip_text(std::format(std::locale(""), "{:%Ec}", std::chrono::current_zone()->to_local(VersionData.ReleaseDate)));
        DialogVersion.set_text(VersionData.Version);
        DialogVersionHR.set_text(VersionData.VersionHR);
        Utils::SlotHolder SlotAccept = DialogAccept.signal_clicked().connect([this]() {
            if (!Auth.LogOutPreflight()) {
                return;
            }
            Dialog.response(Gtk::RESPONSE_ACCEPT);
        });

        int Resp = Dialog.run();
        Dialog.hide();
        if (Resp == Gtk::RESPONSE_ACCEPT) {
            std::error_code Code;
            auto TempPath = std::filesystem::temp_directory_path(Code);
            if (TempPath.empty()) {
                return;
            }

            TempPath /= "EGL3Installer.exe";

            auto Resp = Client.GetInstaller();
            if (Resp.HasError()) {
                return;
            }

            Utils::Streams::FileStream Stream;
            if (!Stream.open(TempPath, "wb")) {
                return;
            }

            Stream.write(Resp->c_str(), Resp->size());

            Stream.close();
            
            if ((uint64_t)ShellExecuteW(NULL, L"runas", TempPath.c_str(), NULL, NULL, SW_SHOW) <= 32) {
                return;
            }

            SysTray.Quit();
        }
    }

    void UpdateCheckModule::OnUpdateAvailable(const Web::EGL3::Responses::VersionInfo& VersionData)
    {
        SysTray.ShowToast(Utils::ToastTemplate{
            .Type = Utils::ToastType::Text02,
            .TextFields = {"New EGL3 Update", "A new version of EGL3 is available!"},
            .Actions = { std::format("Update to {}", VersionData.VersionHR) }
        }, {
            .OnClicked = [this, VersionData](int ActionIdx) {
                DisplayUpdate(VersionData);
            }
        });
    }

    void UpdateCheckModule::CheckForUpdate()
    {
        auto DataResp = Client.GetLatestVersion();
        if (DataResp.HasError()) {
            return;
        }
        if (DataResp->VersionNum > LatestVersionNum) {
            LatestVersionNum = DataResp->VersionNum;
            OnUpdateAvailable(DataResp.Get());
        }
    }

    void UpdateCheckModule::BackgroundTask()
    {
        std::unique_lock Lock(Mutex);
        do {
            CheckForUpdate();
        } while (!CV.wait_for(Lock, UpdateFrequency, [this]() { return Cancelled; }));
    }
}