#include "Store.h"

#include "../../utils/Config.h"

namespace EGL3::Storage::Persistent {
    Store::Store(const std::filesystem::path& Path) : Path(Path) {
        if (std::filesystem::is_regular_file(Path)) {
            Utils::Streams::FileStream Stream;
            Stream.open(Path, "rb+");
            EGL3_ENSURE(Stream.valid(), LogLevel::Error, "Could not open store file. Try removing or renaming it.");
            uint32_t ElementCount;
            Stream >> ElementCount;
            for (int i = 0; i < ElementCount; ++i) {
                uint32_t KeyConstant;
                Stream >> KeyConstant;

                auto Elem = Data.emplace(KeyConstant, KeyConstant);
                if (Elem.first->second.HasValue()) {
                    EGL3_ENSURE(Elem.second, LogLevel::Error, "Could not emplace new constant to store");
                    Elem.first->second.Deserialize(Stream);
                }
                else { // Constant is not defined
                    Data.erase(KeyConstant);
                }
            }
        }
    }

    Store::~Store() {
        Flush();
    }

    void Store::Flush() {
        std::lock_guard Guard(Mutex);

        std::filesystem::create_directories(Path.parent_path());
        Utils::Streams::FileStream Stream;
        Stream.open(Path, "wb+");
        
        Stream << (uint32_t)Data.size();
        for (auto& Key : Data) {
            Stream << Key.first;
            Key.second.Serialize(Stream);
        }
    }
}