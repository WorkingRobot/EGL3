#include "WhatsNew.h"

#include "../web/epic/EpicClient.h"

namespace EGL3::Modules {
    WhatsNewModule::WhatsNewModule(ModuleList& Modules, Storage::Persistent::Store& Storage, const Utils::GladeBuilder& Builder) :
        ImageCache(Modules.GetModule<ImageCacheModule>()),
        Storage(Storage),
        Box(Builder.GetWidget<Gtk::Box>("PlayWhatsNewBox")),
        RefreshBtn(Builder.GetWidget<Gtk::Button>("PlayWhatsNewRefreshBtn")),
        CheckBR(Builder.GetWidget<Gtk::CheckMenuItem>("WhatsNewBR")),
        CheckBlog(Builder.GetWidget<Gtk::CheckMenuItem>("WhatsNewBlog")),
        CheckCreative(Builder.GetWidget<Gtk::CheckMenuItem>("WhatsNewCreative")),
        CheckNotice(Builder.GetWidget<Gtk::CheckMenuItem>("WhatsNewNotice")),
        CheckSTW(Builder.GetWidget<Gtk::CheckMenuItem>("WhatsNewSTW")),
        Selection(Storage.Get(Storage::Persistent::Key::WhatsNewSelection))
    {
        Dispatcher.connect([this]() { UpdateBox(); });
        RefreshBtn.signal_clicked().connect([this]() { Refresh(); });

        CheckBR.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::BR>());
        CheckBlog.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::BLOG>());
        CheckCreative.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::CREATIVE>());
        CheckNotice.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::NOTICE>());
        CheckSTW.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::STW>());

        CheckBR.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckBlog.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckCreative.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckNotice.signal_toggled().connect([this]() { UpdateSelection(); });
        CheckSTW.signal_toggled().connect([this]() { UpdateSelection(); });

        Refresh();
    }

    WhatsNewModule::~WhatsNewModule() {
        std::lock_guard Guard(ItemDataMutex);
    }

    void WhatsNewModule::Refresh() {
        if (!RefreshBtn.get_sensitive()) {
            return;
        }

        RefreshBtn.set_sensitive(false);

        RefreshTask = std::async(std::launch::async, [this]() {
            std::unique_lock ItemDataGuard(ItemDataMutex);

            ItemData.clear();
            Web::Epic::EpicClient Client;

            auto Blogs = Client.GetBlogPosts("en-US");
            auto News = Client.GetPageInfo("en-US");

            if (Blogs.HasError()) {
                ItemDataError = Blogs.GetErrorCode();
            }
            else if (News.HasError()) {
                ItemDataError = News.GetErrorCode();
            }
            else {
                ItemDataError = Web::ErrorCode::Success;

                auto& Store = Storage.Get(Storage::Persistent::Key::WhatsNewTimestamps);

                // Blogs

                if (SourceEnabled<Storage::Models::WhatsNew::ItemSource::BLOG>()) {
                    for (auto& Post : Blogs->BlogList) {
                        ItemData.emplace_back(Post, GetTime(Store, Post), Storage::Models::WhatsNew::ItemSource::BLOG);
                    }
                }


                // Emergency Notices

                if (SourceEnabled<Storage::Models::WhatsNew::ItemSource::NOTICE>()) {
                    if (News->EmergencyNotice.News.Messages.has_value()) {
                        for (auto& Post : News->EmergencyNotice.News.Messages.value()) {
                            if (Post.Hidden) {
                                continue;
                            }
                            // Emergency notices should be at the top (plus, they're not unique between downtimes so it's hard to put a uuid on them anyway)
                            ItemData.emplace_back(Post, std::chrono::system_clock::time_point::max(), Storage::Models::WhatsNew::ItemSource::NOTICE);
                        }
                    }

                    if (News->EmergencyNotice.News.RegionMessages.has_value()) {
                        for (auto& Post : News->EmergencyNotice.News.RegionMessages.value()) {
                            if (Post.Region != "NA" || Post.Message.Hidden) {
                                continue;
                            }
                            ItemData.emplace_back(Post, std::chrono::system_clock::time_point::max(), Storage::Models::WhatsNew::ItemSource::NOTICE);
                        }
                    }
                }


                // BR News

                if (SourceEnabled<Storage::Models::WhatsNew::ItemSource::BR>()) {
                    if (News->BRNewsV2.News.Motds.has_value()) {
                        for (auto& Post : News->BRNewsV2.News.Motds.value()) {
                            if (Post.Hidden) {
                                continue;
                            }
                            // If the id does not show up, use the current system time and store it as the first time we saw it
                            ItemData.emplace_back(Post, GetTime(Store, Post), Storage::Models::WhatsNew::ItemSource::BR);
                        }
                    }

                    if (News->BRNewsV2.News.PlatformMotds.has_value()) {
                        for (auto& Post : News->BRNewsV2.News.PlatformMotds.value()) {
                            if (Post.Hidden || Post.Message.Hidden || Post.Platform != "windows") {
                                continue;
                            }
                            ItemData.emplace_back(Post, GetTime(Store, Post.Message), Storage::Models::WhatsNew::ItemSource::BR);
                        }
                    }
                }


                // Creative News

                if (SourceEnabled<Storage::Models::WhatsNew::ItemSource::CREATIVE>()) {
                    if (News->CreativeNewsV2.News.Motds.has_value()) {
                        for (auto& Post : News->CreativeNewsV2.News.Motds.value()) {
                            if (Post.Hidden) {
                                continue;
                            }
                            ItemData.emplace_back(Post, GetTime(Store, Post), Storage::Models::WhatsNew::ItemSource::CREATIVE);
                        }
                    }
                }


                // STW News

                if (SourceEnabled<Storage::Models::WhatsNew::ItemSource::STW>()) {
                    if (News->SaveTheWorldNews.News.Messages.has_value()) {
                        for (auto& Post : News->SaveTheWorldNews.News.Messages.value()) {
                            if (Post.Hidden) {
                                continue;
                            }
                            ItemData.emplace_back(Post, GetTime(Store, Post), Storage::Models::WhatsNew::ItemSource::STW);
                        }
                    }
                }
            }

            std::sort(ItemData.begin(), ItemData.end(), [](const decltype(ItemData)::value_type& A, const decltype(ItemData)::value_type& B) {
                return A.Date > B.Date;
            });

            ItemData.erase(std::unique(ItemData.begin(), ItemData.end(), [](const decltype(ItemData)::value_type& A, const decltype(ItemData)::value_type& B) {
                if (A.Item.index() != B.Item.index()) {
                    return false;
                }

                return std::visit([&](auto&& AItem) {
                    using T = std::decay_t<decltype(AItem)>;
                    if constexpr (std::is_same_v<Web::Epic::Responses::GetPageInfo::GenericMotd, T>) {
                        return AItem.Id == std::get<Web::Epic::Responses::GetPageInfo::GenericMotd>(B.Item).Id;
                    }
                    if constexpr (std::is_same_v<Web::Epic::Responses::GetPageInfo::GenericPlatformMotd, T>) {
                        return AItem.Message.Id == std::get<Web::Epic::Responses::GetPageInfo::GenericPlatformMotd>(B.Item).Message.Id;
                    }
                    return false;
                }, A.Item);
            }), ItemData.end());

            ItemDataGuard.unlock();
            Dispatcher.emit();
        });
    }

    void WhatsNewModule::UpdateBox() {
        std::lock_guard ItemDataGuard(ItemDataMutex);

        Widgets.clear();
        Box.foreach([this](Gtk::Widget& Widget) { Box.remove(Widget); });

        for (auto& Pair : ItemData) {
            std::visit([&, this](auto&& Item) {
                Box.pack_start(*Widgets.emplace_back(std::make_unique<Widgets::WhatsNewItem>(Item, Pair.Date, Pair.Source, ImageCache)));
            }, Pair.Item);
        }
        Box.show_all_children();
        RefreshBtn.set_sensitive(true);
    }

    void WhatsNewModule::UpdateSelection() {
        Selection =
            (CheckBlog.get_active() ? 0 : (uint8_t)Storage::Models::WhatsNew::ItemSource::BLOG) |
            (CheckBR.get_active() ? 0 : (uint8_t)Storage::Models::WhatsNew::ItemSource::BR) |
            (CheckSTW.get_active() ? 0 : (uint8_t)Storage::Models::WhatsNew::ItemSource::STW) |
            (CheckCreative.get_active() ? 0 : (uint8_t)Storage::Models::WhatsNew::ItemSource::CREATIVE) |
            (CheckNotice.get_active() ? 0 : (uint8_t)Storage::Models::WhatsNew::ItemSource::NOTICE);
        Refresh();
    }

    template<Storage::Models::WhatsNew::ItemSource Source>
    bool WhatsNewModule::SourceEnabled() {
        return !(Selection & (uint8_t)Source);
    }

    template<class T>
    std::chrono::system_clock::time_point WhatsNewModule::GetTime(decltype(Storage::Persistent::Key::WhatsNewTimestamps)::ValueType& Storage, const T& Value) {
        return std::chrono::system_clock::time_point::min();
    }

    template<>
    std::chrono::system_clock::time_point WhatsNewModule::GetTime<Web::Epic::Responses::GetBlogPosts::BlogItem>(decltype(Storage::Persistent::Key::WhatsNewTimestamps)::ValueType& Storage, const Web::Epic::Responses::GetBlogPosts::BlogItem& Value) {
        return Value.Date;
    }

    template<>
    std::chrono::system_clock::time_point WhatsNewModule::GetTime<Web::Epic::Responses::GetPageInfo::GenericMotd>(decltype(Storage::Persistent::Key::WhatsNewTimestamps)::ValueType& Storage, const Web::Epic::Responses::GetPageInfo::GenericMotd& Value) {
        // Just hash the id, it isn't used anywhere and I assume it's unique anyway
        return Storage.try_emplace(std::hash<std::string>{}(Value.Id), std::chrono::system_clock::now()).first->second;
    }

    template<>
    std::chrono::system_clock::time_point WhatsNewModule::GetTime<Web::Epic::Responses::GetPageInfo::GenericNewsPost>(decltype(Storage::Persistent::Key::WhatsNewTimestamps)::ValueType& Storage, const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Value) {
        // Only used for stw at this point, if any of these change, it's going to look different visually
        return Storage.try_emplace(Utils::HashCombine(Value.AdSpace, Value.Body, Value.Image, Value.Title), std::chrono::system_clock::now()).first->second;
    }
}