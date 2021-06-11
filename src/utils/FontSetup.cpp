#include "FontSetup.h"

#include "Config.h"
#include "KnownFolders.h"
#include "Log.h"

#include <fontconfig/fontconfig.h>
#include <gtkmm.h>
#include <pango/pangofc-fontmap.h>
#include <string>
#include <fstream>

namespace EGL3::Utils {
    void SetupFonts() {
        auto FontsFolder = Config::GetExeFolder() / "fonts";
        
        {
            std::ifstream ConfStream(FontsFolder / "fonts.conf");
            std::ofstream ConfOutStream(Config::GetFolder() / "fonts.conf");
            EGL3_VERIFY(ConfStream.good(), "Can't find fonts.conf template. Try reinstalling.");
            EGL3_VERIFY(ConfOutStream.good(), "Can't find fonts.conf template. Try reinstalling.");

            std::string Line;
            while (std::getline(ConfStream, Line))
            {
                if (Line == "<!--(CONFD)-->") {
                    ConfOutStream << "<include ignore_missing=\"no\">" << (FontsFolder / "conf.d").string() << "</include>";
                }
                else {
                    ConfOutStream << Line;
                }
                ConfOutStream << '\n';
            }
        }

        auto Config = FcConfigCreate();
        FcConfigSetCurrent(Config);
        FcConfigParseAndLoad(Config, (FcChar8*)(Config::GetFolder() / "fonts.conf").string().c_str(), true);
        FcConfigAppFontAddDir(Config, (FcChar8*)FontsFolder.string().c_str());

        auto Path = Platform::GetKnownFolderPath(FOLDERID_Fonts);
        if (!Path.empty()) {
            FcConfigAppFontAddDir(Config, (FcChar8*)Path.string().c_str());
        }

        auto FontMap = (PangoFcFontMap*)pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
        pango_fc_font_map_set_config(FontMap, Config);
        pango_cairo_font_map_set_default((PangoCairoFontMap*)FontMap);
    }
}