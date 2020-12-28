#include "WhatsNew.h"

#include "../../utils/Crc32.h"

namespace EGL3::Storage::Models {
    WhatsNew::WhatsNew(decltype(Item)&& Item, decltype(Date) Date, ItemSource Source) :
        Item(std::move(Item)),
        Date(Date),
        Source(Source)
    {

    }
}