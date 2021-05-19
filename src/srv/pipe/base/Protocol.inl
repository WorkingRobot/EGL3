#include "Protocol.h"

namespace EGL3::Service::Pipe {
    template<bool Write, typename... Ts>
    void OperateStream(Utils::Streams::Stream& Stream, Ts&&... Args) {
        if constexpr (Write) {
            (Stream << ... << std::forward<Ts>(Args));
        }
        else {
            (Stream >> ... >> std::forward<Ts>(Args));
        }
    }

    template<bool Write, PacketId Id, PacketDirection Direction>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<Id, Direction>& Packet) {

    }

    template<PacketId Id, PacketDirection Direction>
    Utils::Streams::Stream& operator>>(Utils::Streams::Stream& Stream, Packet<Id, Direction>& Val) {
        OperateStream<false>(Stream, Val);

        return Stream;
    }

    template<PacketId Id, PacketDirection Direction>
    Utils::Streams::Stream& operator<<(Utils::Streams::Stream& Stream, const Packet<Id, Direction>& Val) {
        OperateStream<true>(Stream, (Packet<Id, Direction>&)Val);

        return Stream;
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::Handshake, PacketDirection::ServerBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.Protocol,
            Packet.ClientName
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::Handshake, PacketDirection::ClientBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.Response,
            Packet.ServerName
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::BeginMount, PacketDirection::ServerBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.Path
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::BeginMount, PacketDirection::ClientBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.Response,
            Packet.MountHandle
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::GetMountPath, PacketDirection::ServerBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.MountHandle
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::GetMountPath, PacketDirection::ClientBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.Response,
            Packet.Path
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::GetMountInfo, PacketDirection::ServerBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.MountHandle
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::GetMountInfo, PacketDirection::ClientBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.Response
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::EndMount, PacketDirection::ServerBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.MountHandle
        );
    }

    template<bool Write>
    void OperateStream(Utils::Streams::Stream& Stream, Packet<PacketId::EndMount, PacketDirection::ClientBound>& Packet) {
        OperateStream<Write>(Stream,
            Packet.Response
        );
    }
}