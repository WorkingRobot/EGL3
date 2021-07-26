#include "WhatsNew.h"

#include "../web/epic/EpicClient.h"

namespace EGL3::Modules {
    WhatsNewModule::WhatsNewModule(ModuleList& Ctx) :
        ImageCache(Ctx.GetModule<ImageCacheModule>()),
        Box(Ctx.GetWidget<Gtk::Box>("PlayWhatsNewBox")),
        RefreshBtn(Ctx.GetWidget<Gtk::Button>("PlayWhatsNewRefreshBtn")),
        CheckBR(Ctx.GetWidget<Gtk::CheckMenuItem>("WhatsNewBR")),
        CheckBlog(Ctx.GetWidget<Gtk::CheckMenuItem>("WhatsNewBlog")),
        CheckCreative(Ctx.GetWidget<Gtk::CheckMenuItem>("WhatsNewCreative")),
        CheckNotice(Ctx.GetWidget<Gtk::CheckMenuItem>("WhatsNewNotice")),
        CheckSTW(Ctx.GetWidget<Gtk::CheckMenuItem>("WhatsNewSTW")),
        Timestamps(Ctx.Get<TimestampsSetting>()),
        Selection(Ctx.Get<SelectionSetting>())
    {
        Dispatcher.connect([this](Web::ErrorData::Status Error, const std::vector<Storage::Models::WhatsNew>& Data) { UpdateBox(Error, Data); });
        SlotRefresh = RefreshBtn.signal_clicked().connect([this]() { Refresh(); });

        CheckBR.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::BR>());
        CheckBlog.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::BLOG>());
        CheckCreative.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::CREATIVE>());
        CheckNotice.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::NOTICE>());
        CheckSTW.set_active(SourceEnabled<Storage::Models::WhatsNew::ItemSource::STW>());

        SlotBR = CheckBR.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotBlog = CheckBlog.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotCreative = CheckCreative.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotNotice = CheckNotice.signal_toggled().connect([this]() { UpdateSelection(); });
        SlotSTW = CheckSTW.signal_toggled().connect([this]() { UpdateSelection(); });

        Refresh();
    }

    WhatsNewModule::~WhatsNewModule() {
        
    }

    void WhatsNewModule::Refresh() {
        if (!RefreshBtn.get_sensitive()) {
            return;
        }

        RefreshBtn.set_sensitive(false);

        RefreshTask = std::async(std::launch::async, [this]() {
            std::vector<Storage::Models::WhatsNew> Data;

            Web::Epic::EpicClient Client;

            auto Blogs = Client.GetBlogPosts(Utils::Config::GetLanguage());
            auto News = Client.GetPageInfo(Utils::Config::GetLanguage());

            if (Blogs.HasError()) {
                Dispatcher.emit(Blogs.GetErrorCode(), Data);
                return;
            }
            else if (News.HasError()) {
                Dispatcher.emit(News.GetErrorCode(), Data);
                return;
            }
            else {
                // Blogs

                if (SourceEnabled<Storage::Models::WhatsNew::ItemSource::BLOG>()) {
                    for (auto& Post : Blogs->BlogList) {
                        Data.emplace_back(Post, GetTime(Post), Storage::Models::WhatsNew::ItemSource::BLOG);
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
                            Data.emplace_back(Post, Web::TimePoint::max(), Storage::Models::WhatsNew::ItemSource::NOTICE);
                        }
                    }

                    if (News->EmergencyNotice.News.RegionMessages.has_value()) {
                        for (auto& Post : News->EmergencyNotice.News.RegionMessages.value()) {
                            if (Post.Region != "NA" || Post.Message.Hidden) {
                                continue;
                            }
                            Data.emplace_back(Post, Web::TimePoint::max(), Storage::Models::WhatsNew::ItemSource::NOTICE);
                        }
                    }

                    for (auto& Post : News->EmergencyNoticeV2.EmergencyNotices.EmergencyNotices) {
                        if (Post.Hidden) {
                            continue;
                        }
                        Data.emplace_back(Post, Web::TimePoint::max(), Storage::Models::WhatsNew::ItemSource::NOTICE);
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
                            Data.emplace_back(Post, GetTime(Post), Storage::Models::WhatsNew::ItemSource::BR);
                        }
                    }

                    if (News->BRNewsV2.News.PlatformMotds.has_value()) {
                        for (auto& Post : News->BRNewsV2.News.PlatformMotds.value()) {
                            if (Post.Hidden || Post.Message.Hidden || Post.Platform != "windows") {
                                continue;
                            }
                            Data.emplace_back(Post, GetTime(Post.Message), Storage::Models::WhatsNew::ItemSource::BR);
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
                            Data.emplace_back(Post, GetTime(Post), Storage::Models::WhatsNew::ItemSource::CREATIVE);
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
                            Data.emplace_back(Post, GetTime(Post), Storage::Models::WhatsNew::ItemSource::STW);
                        }
                    }
                }
            }

            std::sort(Data.begin(), Data.end(), [](const decltype(Data)::value_type& A, const decltype(Data)::value_type& B) {
                return A.Date > B.Date;
            });

            Data.erase(std::unique(Data.begin(), Data.end(), [](const decltype(Data)::value_type& A, const decltype(Data)::value_type& B) {
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
            }), Data.end());

            Timestamps.Flush();

            Dispatcher.emit(Web::ErrorData::Status::Success, Data);
        });
    }

    void WhatsNewModule::UpdateBox(Web::ErrorData::Status Error, const std::vector<Storage::Models::WhatsNew>& Data) {
        if (Error != Web::ErrorData::Status::Success) {
            return;
        }

        Widgets.clear();
        Box.foreach([this](Gtk::Widget& Widget) { Box.remove(Widget); });

        for (auto& Pair : Data) {
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
        Selection.Flush();
        Refresh();
    }

    template<Storage::Models::WhatsNew::ItemSource Source>
    bool WhatsNewModule::SourceEnabled() {
        return !(*Selection & (uint8_t)Source);
    }

    template<class T>
    Web::TimePoint WhatsNewModule::GetTime(const T& Value) {
        return Web::TimePoint::min();
    }

    template<>
    Web::TimePoint WhatsNewModule::GetTime<Web::Epic::Responses::GetBlogPosts::BlogItem>(const Web::Epic::Responses::GetBlogPosts::BlogItem& Value) {
        return Value.Date;
    }

    template<>
    Web::TimePoint WhatsNewModule::GetTime<Web::Epic::Responses::GetPageInfo::GenericMotd>(const Web::Epic::Responses::GetPageInfo::GenericMotd& Value) {
        // Just hash the id, it isn't used anywhere and I assume it's unique anyway
        return Timestamps->try_emplace(std::hash<std::string>{}(Value.Id), Web::TimePoint::clock::now()).first->second;
    }

    template<>
    Web::TimePoint WhatsNewModule::GetTime<Web::Epic::Responses::GetPageInfo::GenericNewsPost>(const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Value) {
        // Only used for stw at this point, if any of these change, it's going to look different visually
        return Timestamps->try_emplace(Utils::HashCombine(Value.AdSpace, Value.Body, Value.Image, Value.Title), Web::TimePoint::clock::now()).first->second;
    }
}