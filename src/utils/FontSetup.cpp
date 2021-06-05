#include "FontSetup.h"

#include "Log.h"
#include "Config.h"

#include <fontconfig/fontconfig.h>
#include <gtkmm.h>
#include <pango/pangofc-fontmap.h>
#include <string>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShlObj_core.h>

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

        CHAR Path[MAX_PATH];
        if (SHGetFolderPath(NULL, CSIDL_FONTS, NULL, SHGFP_TYPE_CURRENT, Path) == S_OK) {
            FcConfigAppFontAddDir(Config, (FcChar8*)Path);
        }

        auto FontMap = (PangoFcFontMap*)pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
        pango_fc_font_map_set_config(FontMap, Config);
        pango_cairo_font_map_set_default((PangoCairoFontMap*)FontMap);
    }
}