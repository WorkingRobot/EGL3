#include "Protocol.h"

namespace EGL3::Service::Sock {
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, PacketHeader& Val)
    {
        Stream >> Val.Data;

        return Stream;
    }

    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const PacketHeader& Val)
    {
        Stream << Val.Data;

        return Stream;
    }

    uint64_t PacketHeader::GetSize() const
    {
        return (Data & 0x0000FFFFFFFFFFFFllu) >> 0;
    }

    void PacketHeader::SetSize(uint64_t Val) {
        Data = (Val << 0) | (uint64_t(GetId()) << 48);
    }

    uint16_t PacketHeader::GetId() const {
        return (Data & 0xFFFF000000000000llu) >> 48;
    }

    void PacketHeader::SetId(uint16_t Val) {
        Data = (GetSize() << 0) | (uint64_t(Val) << 48);
    }
}