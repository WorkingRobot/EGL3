#pragma once

#include "BaseModule.h"

#include <gtkmm.h>
#include <variant>

#include "../storage/models/WhatsNew.h"
#include "../storage/persistent/Store.h"
#include "../utils/GladeBuilder.h"
#include "../utils/HashCombine.h"
#include "../web/epic/EpicClient.h"
#include "../widgets/WhatsNewItem.h"

namespace EGL3::Modules {
    class WhatsNewModule : public BaseModule {
    public:
        WhatsNewModule(const Glib::RefPtr<Gtk::Application>& App, Utils::GladeBuilder& Builder) : App(App), Box(Builder.GetWidget<Gtk::Box>("PlayWhatsNewBox")) {
            Dispatcher.connect([this]() { UpdateBox(); });
            Refresh();
        }

        void Refresh() {
            // Just don't run this while another refresh task is also running, might add something extra here for that or something
            RefreshTask = std::async(std::launch::async, [this]() {
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
                    auto Storage = (Storage::Persistent::Store*)App.get()->get_data("EGL3Storage");
                    auto& Store = Storage->Get(Storage::Persistent::Key::WhatsNew);

                    // Blogs

                    for (auto& Post : Blogs->BlogList) {
                        ItemData.emplace_back(Post, GetTime(Store, Post), Storage::Models::WhatsNew::ItemSource::BLOG);
                    }


                    // Emergency Notices

                    for (auto& Post : News->EmergencyNotice.News.Messages) {
                        if (Post.Hidden) {
                            continue;
                        }
                        // Emergency notices should be at the top (plus, they're not unique between downtimes so it's hard to put a uuid on them anyway)
                        ItemData.emplace_back(Post, std::chrono::system_clock::time_point::max(), Storage::Models::WhatsNew::ItemSource::NOTICE);
                    }

                    if (News->EmergencyNotice.News.RegionMessages.has_value()) {
                        for (auto& Post : News->EmergencyNotice.News.RegionMessages.value()) {
                            if (Post.Region != "NA" || Post.Message.Hidden) {
                                continue;
                            }
                            ItemData.emplace_back(Post, std::chrono::system_clock::time_point::max(), Storage::Models::WhatsNew::ItemSource::NOTICE);
                        }
                    }


                    // STW News

                    for (auto& Post : News->SaveTheWorldNews.News.Messages) {
                        if (Post.Hidden) {
                            continue;
                        }
                        ItemData.emplace_back(Post, GetTime(Store, Post), Storage::Models::WhatsNew::ItemSource::STW);
                    }


                    // Creative News

                    if (News->CreativeNewsV2.News.Motds.has_value()) {
                        for (auto& Post : News->CreativeNewsV2.News.Motds.value()) {
                            if (Post.Hidden) {
                                continue;
                            }
                            ItemData.emplace_back(Post, GetTime(Store, Post), Storage::Models::WhatsNew::ItemSource::CREATIVE);
                        }
                    }


                    // BR News

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

                Dispatcher.emit();
            });
        }

    private:
        void UpdateBox() {
            for (auto& Pair : ItemData) {
                std::visit([&, this](auto&& Item) {
                    Box.get().pack_start(*Widgets.emplace_back(std::make_unique<Widgets::WhatsNewItem>(Item, Pair.Date, Pair.Source)));
                }, Pair.Item);
            }
            Box.get().show_all_children();
        }

        template<class T>
        std::chrono::system_clock::time_point GetTime(decltype(Storage::Persistent::Key::WhatsNew)::ValueType& Storage, const T& Value) {
            return std::chrono::system_clock::time_point::min();
        }

        template<>
        std::chrono::system_clock::time_point GetTime<Web::Epic::Responses::GetBlogPosts::BlogItem>(decltype(Storage::Persistent::Key::WhatsNew)::ValueType& Storage, const Web::Epic::Responses::GetBlogPosts::BlogItem& Value) {
            return Value.Date;
        }

        template<>
        std::chrono::system_clock::time_point GetTime<Web::Epic::Responses::GetPageInfo::GenericMotd>(decltype(Storage::Persistent::Key::WhatsNew)::ValueType& Storage, const Web::Epic::Responses::GetPageInfo::GenericMotd& Value) {
            // Just hash the id, it isn't used anywhere and I assume it's unique anyway
            return Storage.try_emplace(std::hash<std::string>{}(Value.Id), std::chrono::system_clock::now()).first->second;
        }

        template<>
        std::chrono::system_clock::time_point GetTime<Web::Epic::Responses::GetPageInfo::GenericNewsPost>(decltype(Storage::Persistent::Key::WhatsNew)::ValueType& Storage, const Web::Epic::Responses::GetPageInfo::GenericNewsPost& Value) {
            // Only used for stw at this point, if any of these change, it's going to look different visually
            return Storage.try_emplace(Utils::HashCombine(Value.AdSpace, Value.Body, Value.Image, Value.Title), std::chrono::system_clock::now()).first->second;
        }

        std::reference_wrapper<const Glib::RefPtr<Gtk::Application>> App;
        std::reference_wrapper<Gtk::Box> Box;

        std::future<void> RefreshTask;
        Glib::Dispatcher Dispatcher;
        Web::BaseClient::ErrorCode ItemDataError;
        std::vector<Storage::Models::WhatsNew> ItemData;
        std::vector<std::unique_ptr<Widgets::WhatsNewItem>> Widgets;
    };
}