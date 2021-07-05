#pragma once

#include "../storage/models/WhatsNew.h"
#include "../utils/SlotHolder.h"
#include "../web/ErrorData.h"
#include "../widgets/WhatsNewItem.h"
#include "ModuleList.h"

#include <gtkmm.h>

namespace EGL3::Modules {
    using WhatsNewTimestamps = Storage::Persistent::Setting<Utils::Crc32("WhatsNewTimestamps"), std::unordered_map<size_t, Web::TimePoint>>;

    class WhatsNewModule : public BaseModule {
    public:
        WhatsNewModule(ModuleList& Ctx);

        ~WhatsNewModule();

        void Refresh();

    private:
        void UpdateBox();

        void UpdateSelection();

        template<Storage::Models::WhatsNew::ItemSource Source>
        bool SourceEnabled();

        template<class T>
        Web::TimePoint GetTime(WhatsNewTimestamps::Type& Storage, const T& Value);

        template<>
        Web::TimePoint GetTime<Web::Epic::Responses::GetBlogPosts::BlogItem>(WhatsNewTimestamps::Type& Storage, const Web::Epic::Responses::GetBlogPosts::BlogItem& Value);

        template<>
        Web::TimePoint GetTime<Web::Epic::Responses::GetPageInfo::GenericMotd>(WhatsNewTimestamps::Type& Storage, const Web::Epic::Responses::GetPageInfo::GenericMotd& Value);

        template<>
        Web::TimePoint GetTime<Web::Epic::Responses::GetPageInfo::GenericNewsPost>(WhatsNewTimestamps::Type& Storage, const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Value);

        Storage::Persistent::Store& Storage;
        ImageCacheModule& ImageCache;
        Gtk::Box& Box;
        Gtk::Button& RefreshBtn;
        Gtk::CheckMenuItem& CheckBR;
        Gtk::CheckMenuItem& CheckBlog;
        Gtk::CheckMenuItem& CheckCreative;
        Gtk::CheckMenuItem& CheckNotice;
        Gtk::CheckMenuItem& CheckSTW;
        uint8_t& Selection;

        Utils::SlotHolder SlotRefresh;
        Utils::SlotHolder SlotBR;
        Utils::SlotHolder SlotBlog;
        Utils::SlotHolder SlotCreative;
        Utils::SlotHolder SlotNotice;
        Utils::SlotHolder SlotSTW;

        std::future<void> RefreshTask;
        Glib::Dispatcher Dispatcher;
        Web::ErrorData::Status ItemDataError;
        std::vector<Storage::Models::WhatsNew> ItemData;
        std::mutex ItemDataMutex;
        std::vector<std::unique_ptr<Widgets::WhatsNewItem>> Widgets;
    };
}