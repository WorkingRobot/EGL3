#pragma once

// 0: Installer (downloaded from browser to install everything)
// 1: Packer (packs update files)
// 2: Updater (used in the install location)
#define RUN_TASK 0

// Packing config
#define PACK_DIR R"(J:\Code\Visual Studio 2017\Projects\fnbot.shop\bin\Release\netcoreapp3.0\win-x64)"
#define PACK_STARTUP_EXE "fnbot.shop.updater.exe" // is this project but with RUN_TASK = 3
#define PACK_UNINSTALL_EXE "uninstall"
#define PACK_MODIFY_EXE "reinstall"
#define PACK_HELP_LINK "https://fnbot.shop/help"
#define PACK_ABOUT_LINK "https://fnbot.shop"
#define PACK_PATCHNOTES_LINK "https://fnbot.shop/changelog"

// Installer config
#define CONSOLE_WIDTH 30
#define PROGBAR_UPDATE_SPEED 50
#define DOWNLOAD_FILE "4.3.fns"
#define DOWNLOAD_VERSION "4.3"
#define DOWNLOAD_AUTHOR "WorkingRobot"
#define DOWNLOAD_REPO "fnbot.shop"
#define DOWNLOAD_CHANNEL "PUBLIC"
#define DOWNLOAD_HOST "github.com"
#define PROGRAM_GUID "{DC6A20C9-BF6C-AF9B-E621-B34EB5605183C}"

#define DOWNLOAD_PATH "/" DOWNLOAD_AUTHOR "/" DOWNLOAD_REPO "/releases/download/" DOWNLOAD_VERSION "/" DOWNLOAD_FILE
#define REGISTRY_SUBKEY "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" PROGRAM_GUID

// Updater config
#define UPDATER_STARTUP_EXE L"fnbot.shop.exe" // has to be wchar_t
#define UPDATER_INSTALLER_EXE "installer.exe" // name of the file to be downloaded from github
#define UPDATER_VERSION_HOST "fnbot.shop"
#define UPDATER_VERSION_CHANNEL "PUBLIC"

#define UPDATER_VERSION_PATH "/api/version?c=" UPDATER_VERSION_CHANNEL


#define USE_LZ4_HC

#define FILE_MAGIC 0x21E6D863

#define FILE_BUF_SIZE 1024 * 128 // 128 kb
#define UNPACK_FILE_BUF_SIZE FILE_BUF_SIZE

#define CPPHTTPLIB_ZLIB_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT