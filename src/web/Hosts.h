#pragma once

#include <stdint.h>

namespace EGL3::Web {
    enum class Host : uint8_t {
        BaseFortnite,
        BaseEpicGames,
        Account,
        Launcher,
        Catalog,
        Entitlement,
        Friends,
        Lightswitch,
        XMPP,
        EOSStomp,
        LauncherStomp,
        EOS,
        FortniteContent,
        LauncherGql,
        Statuspage,
        UnrealEngineCdn1,
        UnrealEngineCdn2,
        EGL3Assets,
        EGL3,

        StatuspageNonApi
    };

    template<Host SelectedHost>
    constexpr const char* GetHostUrl() {
        switch (SelectedHost)
        {
        case Host::BaseEpicGames:
            return "https://www.epicgames.com/id/api/";
        case Host::BaseFortnite:
            return "https://www.epicgames.com/fortnite/api/";
        case Host::Account:
            return "https://account-public-service-prod.ol.epicgames.com/account/api/";
        case Host::Launcher:
            return "https://launcher-public-service-prod.ol.epicgames.com/launcher/api/";
        case Host::Catalog:
            return "https://catalog-public-service-prod.ol.epicgames.com/catalog/api/";
        case Host::Entitlement:
            return "https://entitlement-public-service-prod.ol.epicgames.com/entitlement/api/";
        case Host::Friends:
            return "https://friends-public-service-prod.ol.epicgames.com/friends/api/";
        case Host::Lightswitch:
            return "https://lightswitch-public-service-prod.ol.epicgames.com/lightswitch/api/";
        case Host::XMPP:
            return "wss://xmpp-service-prod.ol.epicgames.com//";
        case Host::EOSStomp:
            return "wss://connect.epicgames.dev/";
        case Host::LauncherStomp:
            return "wss://connect.ol.epicgames.com/";
        case Host::EOS:
            return "https://api.epicgames.dev/epic/";
        case Host::FortniteContent:
            return "https://fortnitecontent-website-prod07.ol.epicgames.com/content/api/";
        case Host::LauncherGql:
            return "https://store-launcher.epicgames.com/graphql";
        case Host::Statuspage:
            return "https://status.epicgames.com/api/";
        case Host::UnrealEngineCdn1:
            return "https://cdn1.unrealengine.com/";
        case Host::UnrealEngineCdn2:
            return "https://cdn2.unrealengine.com/";
        case Host::EGL3Assets:
            return "https://epic.gl/assets/";
        case Host::EGL3:
            return "https://epic.gl/";

        case Host::StatuspageNonApi:
            return "https://status.epicgames.com";
        default:
            return "";
        }
    }
}