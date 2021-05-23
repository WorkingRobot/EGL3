#pragma once

#include "../storage/models/WhatsNew.h"
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
        void UpdateBox();

        void UpdateSelection();

        template<Storage::Models::WhatsNew::ItemSource Source>
        bool SourceEnabled();

        template<class T>
        std::chrono::system_clock::time_point GetTime(decltype(Storage::Persistent::Key::WhatsNewTimestamps)::ValueType& Storage, const T& Value);

        template<>
        std::chrono::system_clock::time_point GetTime<Web::Epic::Responses::GetBlogPosts::BlogItem>(decltype(Storage::Persistent::Key::WhatsNewTimestamps)::ValueType& Storage, const Web::Epic::Responses::GetBlogPosts::BlogItem& Value);

        template<>
        std::chrono::system_clock::time_point GetTime<Web::Epic::Responses::GetPageInfo::GenericMotd>(decltype(Storage::Persistent::Key::WhatsNewTimestamps)::ValueType& Storage, const Web::Epic::Responses::GetPageInfo::GenericMotd& Value);

        template<>
        std::chrono::system_clock::time_point GetTime<Web::Epic::Responses::GetPageInfo::GenericNewsPost>(decltype(Storage::Persistent::Key::WhatsNewTimestamps)::ValueType& Storage, const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Value);

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

        std::future<void> RefreshTask;
        Glib::Dispatcher Dispatcher;
        Web::ErrorData::Status ItemDataError;
        std::vector<Storage::Models::WhatsNew> ItemData;
        std::mutex ItemDataMutex;
        std::vector<std::unique_ptr<Widgets::WhatsNewItem>> Widgets;
    };
}