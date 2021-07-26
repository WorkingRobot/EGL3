#pragma once

#include "../storage/models/WhatsNew.h"
#include "../utils/DataDispatcher.h"
#include "../utils/SlotHolder.h"
#include "../web/ErrorData.h"
#include "../widgets/WhatsNewItem.h"
#include "ModuleList.h"

#include <gtkmm.h>

namespace EGL3::Modules {
    class WhatsNewModule : public BaseModule {
    public:
        WhatsNewModule(ModuleList& Ctx);

        ~WhatsNewModule();

        void Refresh();

    private:
        void UpdateBox(Web::ErrorData::Status Error, const std::vector<Storage::Models::WhatsNew>& Data);

        void UpdateSelection();

        template<Storage::Models::WhatsNew::ItemSource Source>
        bool SourceEnabled();

        template<class T>
        Web::TimePoint GetTime(const T& Value);

        template<>
        Web::TimePoint GetTime<Web::Epic::Responses::GetBlogPosts::BlogItem>(const Web::Epic::Responses::GetBlogPosts::BlogItem& Value);

        template<>
        Web::TimePoint GetTime<Web::Epic::Responses::GetPageInfo::GenericMotd>(const Web::Epic::Responses::GetPageInfo::GenericMotd& Value);

        template<>
        Web::TimePoint GetTime<Web::Epic::Responses::GetPageInfo::GenericNewsPost>(const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Value);

        ImageCacheModule& ImageCache;
        Gtk::Box& Box;
        Gtk::Button& RefreshBtn;
        Gtk::CheckMenuItem& CheckBR;
        Gtk::CheckMenuItem& CheckBlog;
        Gtk::CheckMenuItem& CheckCreative;
        Gtk::CheckMenuItem& CheckNotice;
        Gtk::CheckMenuItem& CheckSTW;

        using TimestampsSetting = Storage::Persistent::Setting<Utils::Crc32("WhatsNewTimestamps"), std::unordered_map<size_t, Web::TimePoint>>;
        Storage::Persistent::SettingHolder<TimestampsSetting> Timestamps;
        using SelectionSetting = Storage::Persistent::Setting<Utils::Crc32("WhatsNewSelection"), uint8_t>;
        Storage::Persistent::SettingHolder<SelectionSetting> Selection;

        Utils::SlotHolder SlotRefresh;
        Utils::SlotHolder SlotBR;
        Utils::SlotHolder SlotBlog;
        Utils::SlotHolder SlotCreative;
        Utils::SlotHolder SlotNotice;
        Utils::SlotHolder SlotSTW;

        std::future<void> RefreshTask;
        Utils::DataDispatcher<Web::ErrorData::Status, std::vector<Storage::Models::WhatsNew>> Dispatcher;
        std::vector<std::unique_ptr<Widgets::WhatsNewItem>> Widgets;
    };
}