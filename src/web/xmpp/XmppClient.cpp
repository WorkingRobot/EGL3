#include "XmppClient.h"

#include "../../utils/streams/MemoryStream.h"
#include "../../utils/Log.h"
#include "../../utils/Base64.h"
#include "../../utils/Crc32.h"
#include "../../utils/Hex.h"
#include "../../utils/Random.h"
#include "../../web/Hosts.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <ixwebsocket/IXNetSystem.h>
#include <rapidjson/writer.h>
#include <rapidxml/rapidxml_print.hpp>

namespace EGL3::Web::Xmpp {
    XmppClient::XmppClient(const std::string& AccountId, const std::string& AccessToken) :
        CurrentAccountId(AccountId),
        State(ClientState::OPENING)
    {
        std::call_once(InitFlag, ix::initNetSystem);

        Socket.setUrl(GetHostUrl<Host::XMPP>());
        Socket.addSubProtocol("xmpp");
        Socket.setOnMessageCallback([this](auto& Message) { ReceivedMessage(Message); });
        Socket.setTrafficTrackerCallback([this](size_t size, bool incoming) {
            // BackgroundPingNextTime is the time until a ping needs to be sent from the client
            // Any text traffic resets this timeout back to 60s
            {
                std::unique_lock Lock(BackgroundPingMutex);
                BackgroundPingNextTime = std::chrono::steady_clock::now() + std::chrono::seconds(60);
            }
            BackgroundPingCV.notify_all();
        });

        {
            Utils::Streams::MemoryStream AuthStream;
            AuthStream << '\0';
            AuthStream.write(AccountId.c_str(), AccountId.size());
            AuthStream << '\0';
            AuthStream.write(AccessToken.c_str(), AccessToken.size());
            EncodedAuthValue = Utils::B64Encode((uint8_t*)AuthStream.get(), AuthStream.size());
        }
        CurrentJidWithoutResource = CurrentJid = AccountId + "@prod.ol.epicgames.com";

        Socket.start();
    }

    std::unique_ptr<rapidxml::xml_document<>> CreateDocument() {
        return std::make_unique<rapidxml::xml_document<>>();
    }

    void SendXml(ix::WebSocket& Socket, const rapidxml::xml_node<>& Node)
    {
        std::string OutputString;
        rapidxml::print(std::back_inserter(OutputString), Node, rapidxml::print_no_indenting);
        Socket.sendText(OutputString);
    }

    void XmppClient::SetPresence(const Json::Presence& NewPresence)
    {
        Authenticated.wait(false);

        auto CurrentTime = GetCurrentTimePoint();
        auto Document = CreateDocument();
        auto RootNode = Document->allocate_node(rapidxml::node_element, "presence");
        Document->append_node(RootNode);
        if (NewPresence.ShowStatus != Json::ShowStatus::Online && NewPresence.ShowStatus != Json::ShowStatus::Offline) {
            auto ShowNode = Document->allocate_node(rapidxml::node_element, "show");
            RootNode->append_node(ShowNode);
            switch (NewPresence.ShowStatus)
            {
            case EGL3::Web::Xmpp::Json::ShowStatus::Chat:
                ShowNode->value("chat", 4);
                break;
            case EGL3::Web::Xmpp::Json::ShowStatus::DoNotDisturb:
                ShowNode->value("dnd", 3);
                break;
            case EGL3::Web::Xmpp::Json::ShowStatus::Away:
                ShowNode->value("away", 4);
                break;
            case EGL3::Web::Xmpp::Json::ShowStatus::ExtendedAway:
                ShowNode->value("xa", 2);
                break;
            }
        }

        auto StatusNode = Document->allocate_node(rapidxml::node_element, "status");
        RootNode->append_node(StatusNode);
        rapidjson::StringBuffer StatusBuffer;
        {
            auto& NewStatus = NewPresence.Status;

            rapidjson::Writer<rapidjson::StringBuffer> Writer(StatusBuffer);

            Writer.StartObject();

            Writer.Key("Status");
            Writer.String(NewStatus.GetStatus().c_str());

            Writer.Key("bIsPlaying");
            Writer.Bool(NewStatus.IsPlaying());

            Writer.Key("bIsJoinable");
            Writer.Bool(NewStatus.IsJoinable());

            Writer.Key("bHasVoiceSupport");
            Writer.Bool(NewStatus.HasVoiceSupport());

            Writer.Key("ProductName");
            Writer.String(NewStatus.GetProductName().c_str());

            Writer.Key("SessionId");
            Writer.String(NewStatus.GetSessionId().c_str());

            Writer.Key("Properties");
            NewStatus.GetProperties().WriteTo(Writer);

            Writer.EndObject();
        }
        StatusNode->value(StatusBuffer.GetString(), StatusBuffer.GetLength());

        auto DelayNode = Document->allocate_node(rapidxml::node_element, "delay");
        RootNode->append_node(DelayNode);
        DelayNode->append_attribute(Document->allocate_attribute("stamp", CurrentTime.c_str(), 5, CurrentTime.size()));
        DelayNode->append_attribute(Document->allocate_attribute("xmlns", "urn:xmpp:delay"));

        SendXml(Socket, *Document);
    }

    void XmppClient::SendChat(const std::string& AccountId, const std::string& Content)
    {
        auto ToJid = AccountId + "@prod.ol.epicgames.com";

        auto Document = CreateDocument();
        auto RootNode = Document->allocate_node(rapidxml::node_element, "message");
        RootNode->append_attribute(Document->allocate_attribute("to", ToJid.c_str(), 2, ToJid.size()));
        RootNode->append_attribute(Document->allocate_attribute("type", "chat", 4, 4));
        Document->append_node(RootNode);

        auto BodyNode = Document->allocate_node(rapidxml::node_element, "body");
        RootNode->append_node(BodyNode);
        BodyNode->value(Content.c_str(), Content.size());

        SendXml(Socket, *Document);
    }

    void XmppClient::Close()
    {
        CloseInternal(false);
    }

    XmppClient::~XmppClient()
    {
        Close();
    }

    void XmppClient::CloseInternal(bool Errored)
    {
        if (State == ClientState::CLOSED || State == ClientState::CLOSED_ERRORED) {
            return;
        }

        PresenceUpdate.Set();
        SystemMessage.Set();

        {
            std::unique_lock Lock(BackgroundPingMutex);
            BackgroundPingNextTime = std::chrono::steady_clock::time_point::max();
        }
        BackgroundPingCV.notify_all();
        if (BackgroundPingFuture.valid()) {
            BackgroundPingFuture.get();
        }
        Socket.close();
        // TODO: move this to when gtk is closing the app,
        // so we can ensure safety when 2+ clients are used
        ix::uninitNetSystem();

        State = Errored ? ClientState::CLOSED_ERRORED : ClientState::CLOSED;
    }

    void XmppClient::FinishAuthentication()
    {
        State = ClientState::AUTHENTICATED;
        Authenticated = true;
        Authenticated.notify_all();
    }

    // Subtract 1, we aren't comparing \0 at the end of the source string
    template<size_t SourceStringSize>
    bool XmlStringEqual(const char* XmlString, size_t XmlStringSize, const char(&SourceString)[SourceStringSize]) {
        if (XmlStringSize != SourceStringSize - 1) {
            return false;
        }
        return memcmp(XmlString, SourceString, SourceStringSize - 1) == 0;
    }

    bool XmlStringEqual(const char* XmlString, size_t XmlStringSize, const std::string& SourceString) {
        if (XmlStringSize != SourceString.size()) {
            return false;
        }
        return memcmp(XmlString, SourceString.c_str(), SourceString.size()) == 0;
    }

    template<size_t SourceStringSize>
    bool XmlNameEqual(const rapidxml::xml_base<>* Node, const char(&SourceString)[SourceStringSize]) {
        return XmlStringEqual(Node->name(), Node->name_size(), SourceString);
    }

    bool XmlNameEqual(const rapidxml::xml_base<>* Node, const std::string& SourceString) {
        return XmlStringEqual(Node->name(), Node->name_size(), SourceString);
    }

    template<size_t SourceStringSize>
    bool XmlValueEqual(const rapidxml::xml_base<>* Node, const char(&SourceString)[SourceStringSize]) {
        return XmlStringEqual(Node->value(), Node->value_size(), SourceString);
    }

    bool XmlValueEqual(const rapidxml::xml_base<>* Node, const std::string& SourceString) {
        return XmlStringEqual(Node->value(), Node->value_size(), SourceString);
    }

    void SendXmppOpen(ix::WebSocket& Socket)
    {
        auto Document = CreateDocument();
        auto RootNode = Document->allocate_node(rapidxml::node_element, "open");
        Document->append_node(RootNode);
        RootNode->append_attribute(Document->allocate_attribute("xmlns", "urn:ietf:params:xml:ns:xmpp-framing"));
        RootNode->append_attribute(Document->allocate_attribute("to", "prod.ol.epicgames.com"));
        RootNode->append_attribute(Document->allocate_attribute("version", "1.0"));
        SendXml(Socket, *Document);
    }

    bool ReadXmppOpen(const rapidxml::xml_node<>* Node)
    {
        // RFC6120 section 4.7 should be adhered to but I'm just doing whatever works fine
        // https://tools.ietf.org/html/rfc6120#section-4.7

        if (!EGL3_ENSURE(XmlNameEqual(Node, "open"), LogLevel::Error, "RFC 7395 3.4 (Page 6) requires first node to be <open>")) {
            return false; // This is absolutely necessary, the others are just checks for good standing
        }

        auto XmlnsAttr = Node->first_attribute("xmlns", 5);
        if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <open>, xmlns=\"urn:ietf:params:xml:ns:xmpp-framing\" expected")) {
            EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-framing"), LogLevel::Warning, "Bad xmlns attr value with <open>, xmlns=\"urn:ietf:params:xml:ns:xmpp-framing\" expected");
        }

        auto FromAttr = Node->first_attribute("from", 4); // JID
        if (EGL3_ENSURE(FromAttr, LogLevel::Warning, "RFC 6120 4.7.1 (Page 37) requires from attr to be given by server")) {
            EGL3_ENSURE(XmlValueEqual(FromAttr, "prod.ol.epicgames.com"), LogLevel::Warning, "Bad from attr value with <open>, from=\"prod.ol.epicgames.com\" expected");
        }

        auto IdAttr = Node->first_attribute("id", 2);
        EGL3_ENSURE(IdAttr, LogLevel::Warning, "RFC 6120 4.7.3 (Page 39) requires id attr to be given by server");

        auto VersionAttr = Node->first_attribute("version", 7);
        if (EGL3_ENSURE(VersionAttr, LogLevel::Warning, "RFC 6120 4.7.5 (Page 42) requires version attr to be given by server")) {
            EGL3_ENSURE(XmlValueEqual(VersionAttr, "1.0"), LogLevel::Warning, "Bad version attr value with <open>, version=\"1.0\" expected");
        }

        auto LangAttr = Node->first_attribute("xml:lang", 8);
        if (EGL3_ENSURE(LangAttr, LogLevel::Warning, "RFC 6120 4.7.4 (Page 40) requires xml:lang attr to be given by server")) {
            EGL3_ENSURE(XmlValueEqual(LangAttr, "en"), LogLevel::Warning, "Bad xml:lang attr value with <open>, xml:lang=\"en\" expected");
        }

        return true;
    }

    template<bool Authed>
    bool ReadXmppFeatures(const rapidxml::xml_node<>* Node)
    {
        // <stream:features> (root node)
        {
            // I believe this can also be just <features>, but this is able to be kept for backwards compat
            if (!EGL3_ENSURE(XmlNameEqual(Node, "stream:features"), LogLevel::Error, "Bad node name, <stream:features> required")) {
                return false;
            }

            auto XmlnsAttr = Node->first_attribute("xmlns:stream", 12);
            if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <stream:features>, xmlns:stream=\"http://etherx.jabber.org/streams\" expected")) {
                EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "http://etherx.jabber.org/streams"), LogLevel::Warning, "Bad xmlns:stream attr value with <stream:features>, xmlns:stream=\"http://etherx.jabber.org/streams\" expected");
            }
        }

        // <ver>
        {
            // Roster versioning isn't used (or at least, I don't actually see it used),
            // but it's required to be listed as a stream feature if the server supports it
            // RFC 6121 2.6
            auto VerNode = Node->first_node("ver", 3);
            if (EGL3_ENSURE(VerNode, LogLevel::Warning, "While optional, <ver> is given by Epic's servers")) {
                auto XmlnsAttr = VerNode->first_attribute("xmlns", 5);
                if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <ver>, xmlns=\"urn:xmpp:features:rosterver\" expected")) {
                    EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:xmpp:features:rosterver"), LogLevel::Warning, "Bad xmlns attr value with <ver>, xmlns=\"urn:xmpp:features:rosterver\" expected");
                }
            }
        }

        // <starttls>
        {
            // RFC 6120 6.4.1 says that the server must provide the feature in the response if it's capable
            // We must assert that <required/> is not a child element
            // We don't know how to manage STARTTLS, and we're already connected via wss anyway
            auto TlsNode = Node->first_node("starttls", 8);
            if (EGL3_ENSURE(TlsNode, LogLevel::Warning, "While optional, <starttls> is given by Epic's servers")) {
                auto XmlnsAttr = TlsNode->first_attribute("xmlns", 5);
                if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <starttls>, xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\" expected")) {
                    EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-tls"), LogLevel::Warning, "Bad xmlns attr value with <starttls>, xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\" expected");
                }

                if (!EGL3_ENSURE(TlsNode->first_node("required", 8) == NULL, LogLevel::Error, "STARTTLS is required. We do not support it")) {
                    return false;
                }
            }
        }

        // <compression>
        {
            // XEP-0138 has definitions for what the feature should have
            // We aren't required to pick one if we don't support any (we don't)
            // Epic servers offer zlib, but the official launcher doesn't use it anyway
            auto CompressionNode = Node->first_node("compression", 11);
            if (EGL3_ENSURE(CompressionNode, LogLevel::Warning, "While optional, <compression> is given by Epic's servers")) {
                auto XmlnsAttr = CompressionNode->first_attribute("xmlns", 5);
                if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <compression>, xmlns=\"http://jabber.org/features/compress\" expected")) {
                    EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "http://jabber.org/features/compress"), LogLevel::Warning, "Bad xmlns attr value with <compression>, xmlns=\"http://jabber.org/features/compress\" expected");
                }

                if (!EGL3_ENSURE(CompressionNode->first_node("method", 6), LogLevel::Error, "Compression nodes must have at least one included method")) {
                    // We're technically fine without any methods, but XMPP RFC requires at least one method
                    return false;
                }
            }
        }

        // <mechanisms>
        if constexpr (!Authed)
        {
            auto MechanismsNode = Node->first_node("mechanisms", 10);
            // It's not given the next time when the stream is restarted since SASL/auth is already negotiated before that
            if (!EGL3_ENSURE(MechanismsNode, LogLevel::Error, "RFC 6120 6.4.1 (Page 82) requires mechanisms node to be given on first connection")) {
                return false;
            }

            auto XmlnsAttr = MechanismsNode->first_attribute("xmlns", 5);
            if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <mechanisms>, xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" expected")) {
                EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-sasl"), LogLevel::Warning, "Bad xmlns attr value with <mechanisms>, xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" expected");
            }

            bool PlainAuthExists = false;
            for (auto MechanismNode = MechanismsNode->first_node(); MechanismNode; MechanismNode = MechanismNode->next_sibling()) {
                if (XmlValueEqual(MechanismNode, "PLAIN")) {
                    PlainAuthExists = true;
                    break;
                }
            }
            if (!EGL3_ENSURE(PlainAuthExists, LogLevel::Error, "Only the PLAIN SASL mechanism is supported")) {
                return false;
            }
        }

        // <auth>
        if constexpr (!Authed)
        {
            // XEP-0078's iq-auth feature is given to us, but we never use it (allows for non-SASL auth)
            // The official launcher uses SASL's PLAIN mechanism to login anyway
            auto IqAuthNode = Node->first_node("auth", 4);
            if (EGL3_ENSURE(IqAuthNode, LogLevel::Warning, "While optional, <auth> is given by Epic's servers")) {
                auto XmlnsAttr = IqAuthNode->first_attribute("xmlns", 5);
                if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <auth>, xmlns=\"http://jabber.org/features/iq-auth\" expected")) {
                    EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "http://jabber.org/features/iq-auth"), LogLevel::Warning, "Bad xmlns attr value with <auth>, xmlns=\"http://jabber.org/features/iq-auth\" expected");
                }
            }
        }

        // <bind>
        if constexpr (Authed)
        {
            // RFC 6120 7.4
            // If a client gets authenticated, it must be provided a bind feature, and never otherwise
            auto BindNode = Node->first_node("bind", 4);
            if (!EGL3_ENSURE(BindNode, LogLevel::Error, "Bind node must be present after authenticating")) {
                return false;
            }

            auto XmlnsAttr = BindNode->first_attribute("xmlns", 5);
            if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <bind>, xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\" expected")) {
                EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-bind"), LogLevel::Warning, "Bad xmlns attr value with <bind>, xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\" expected");
            }
        }

        // <session>
        if constexpr (Authed)
        {
            // RFC 3921 3.
            // Epic servers support sessions, and must be provided to the client
            auto SessionNode = Node->first_node("session", 7);
            if (!EGL3_ENSURE(SessionNode, LogLevel::Error, "Session node must be present after authenticating")) {
                return false;
            }

            auto XmlnsAttr = SessionNode->first_attribute("xmlns", 5);
            if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <session>, xmlns=\"urn:ietf:params:xml:ns:xmpp-session\" expected")) {
                EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-session"), LogLevel::Warning, "Bad xmlns attr value with <session>, xmlns=\"urn:ietf:params:xml:ns:xmpp-session\" expected");
            }
        }

        return true;
    }

    // RFC6120 7.6.1
    template<bool Session>
    bool ReadXmppInfoQueryResult(const rapidxml::xml_node<>* Node, const std::string& CurrentJid)
    {
        // <iq> (root node)
        {
            if (!EGL3_ENSURE(XmlNameEqual(Node, "iq"), LogLevel::Error, "Bad node name, <iq> required")) {
                return false;
            }

            auto XmlnsAttr = Node->first_attribute("xmlns", 5);
            if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <iq>, xmlns=\"jabber:client\" expected")) {
                EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "jabber:client"), LogLevel::Warning, "Bad xmlns attr value with <iq>, xmlns=\"jabber:client\" expected");
            }

            auto TypeAttr = Node->first_attribute("type", 4);
            if (!EGL3_ENSURE(TypeAttr, LogLevel::Error, "No type recieved (required) with <iq>, type=\"result\" required")) {
                return false;
            }
            if (!EGL3_ENSURE(XmlValueEqual(TypeAttr, "result"), LogLevel::Error, "Bad type attr value with <iq>, type=\"result\" required")) {
                return false;
            }

            auto IdAttr = Node->first_attribute("id", 2);
            if constexpr (Session) {
                if (!EGL3_ENSURE(IdAttr, LogLevel::Error, "No id recieved (required) with <iq>, id=\"_xmpp_session1\" expected")) {
                    return false;
                }
                EGL3_ENSURE(XmlValueEqual(IdAttr, "_xmpp_session1"), LogLevel::Warning, "Bad id attr value with <iq>, id=\"_xmpp_session1\" expected");
            }
            else {
                if (!EGL3_ENSURE(IdAttr, LogLevel::Error, "No id recieved (required) with <iq>, id=\"_xmpp_bind1\" expected")) {
                    return false;
                }
                EGL3_ENSURE(XmlValueEqual(IdAttr, "_xmpp_bind1"), LogLevel::Warning, "Bad id attr value with <iq>, id=\"_xmpp_bind1\" expected");
            }

            auto ToAttr = Node->first_attribute("to", 2);
            if (EGL3_ENSURE(ToAttr, LogLevel::Warning, "No to jid recieved with <iq>")) {
                EGL3_ENSURE(XmlValueEqual(ToAttr, CurrentJid), LogLevel::Warning, "Bad type attr value with <iq>, expected JID does not match");
            }
        }

        if constexpr (!Session)
        {
            // <bind>
            auto BindNode = Node->first_node("bind", 4);
            {
                if (!EGL3_ENSURE(BindNode, LogLevel::Error, "Bind node must be present in a binding success result")) {
                    return false;
                }

                auto XmlnsAttr = BindNode->first_attribute("xmlns", 5);
                if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <bind>, xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\" expected")) {
                    EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-bind"), LogLevel::Warning, "Bad xmlns attr value with <bind>, xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\" expected");
                }
            }

            // <jid>
            {
                auto JidNode = BindNode->first_node("jid", 3);
                if (!EGL3_ENSURE(JidNode, LogLevel::Error, "Jid must be given back to the client in a binding success result")) {
                    return false;
                }

                if (!EGL3_ENSURE(XmlValueEqual(JidNode, CurrentJid), LogLevel::Error, "Jid must be given back to the client in a binding success result")) {
                    return false;
                }
            }
        }

        return true;
    }

    bool ParseJID(const std::string& JID, std::string_view& Node, std::string_view& Domain, std::string_view& Resource) {
        auto at = JID.find('@');
        auto slash = JID.find('/', at == std::string::npos ? 0 : at);

        if (at != std::string::npos) {
            Node = std::string_view(JID.data(), at);
            if (Node.size() >= 1024) {
                return false;
            }
        }

        Domain = std::string_view(JID.data() + (at == std::string::npos ? 0 : at + 1), std::min(slash - at - 1, JID.size()));
        if (Domain.size() >= 1024) {
            return false;
        }

        if (slash != std::string::npos) {
            Resource = std::string_view(JID.data() + slash + 1);
            if (Resource.size() >= 1024) {
                return false;
            }
        }

        return true;
    }

    bool XmppClient::HandlePresence(const rapidxml::xml_node<>* Node) {
        if (XmlNameEqual(Node, "presence")) {
            // I've recieved presences without an xmnls (third party games just don't care about em)

            auto ToAttr = Node->first_attribute("to", 2);
            if (EGL3_ENSURE(ToAttr, LogLevel::Warning, "No to jid recieved with <presence>")) {
                EGL3_ENSURE(XmlValueEqual(ToAttr, CurrentJidWithoutResource) || XmlValueEqual(ToAttr, CurrentJid), LogLevel::Warning, "Bad to attr value with <presence>, expected JID does not match");
            }

            Json::Presence ParsedPresence;

            auto FromAttr = Node->first_attribute("from", 4);
            if (!EGL3_ENSURE(FromAttr, LogLevel::Warning, "No from recieved with <presence>. Ignoring presence as it is required for parsing.")) {
                return true;
            }
            auto FromJID = std::string(FromAttr->value(), FromAttr->value_size());
            std::string_view JIDId, JIDDomain, JIDResource;
            if (!EGL3_ENSURE(ParseJID(FromJID, JIDId, JIDDomain, JIDResource), LogLevel::Warning, "Bad from attr value with <presence>, expected valid JID. Ignoring presence as it is required for parsing.")) {
                return true;
            }
            ParsedPresence.Resource = std::string(JIDResource);

            auto ShowNode = Node->first_node("show", 4);
            if (ShowNode) {
                switch (Utils::Crc32(ShowNode->value(), ShowNode->value_size()))
                {
                case Utils::Crc32("chat"):
                    ParsedPresence.ShowStatus = Json::ShowStatus::Chat;
                    break;
                case Utils::Crc32("dnd"):
                    ParsedPresence.ShowStatus = Json::ShowStatus::DoNotDisturb;
                    break;
                case Utils::Crc32("away"):
                    ParsedPresence.ShowStatus = Json::ShowStatus::Away;
                    break;
                case Utils::Crc32("xa"):
                    ParsedPresence.ShowStatus = Json::ShowStatus::ExtendedAway;
                    break;
                default:
                    ParsedPresence.ShowStatus = Json::ShowStatus::Online;
                }
            }
            else {
                ParsedPresence.ShowStatus = Json::ShowStatus::Online;
            }

            // https://tools.ietf.org/html/rfc3921#section-2.2.1
            // I've only observed "unavailable", and that's basically the same as offline
            // All other values are errors, and we can set those to offline as well
            auto TypeAttr = Node->first_attribute("type", 4);
            if (TypeAttr) {
                if (!XmlValueEqual(TypeAttr, "available")) {
                    ParsedPresence.ShowStatus = Json::ShowStatus::Offline;
                }
            }

            auto StatusNode = Node->first_node("status", 6);
            if (EGL3_ENSURE(StatusNode, LogLevel::Debug, "No status recieved from user. This is technically optional and the presence will be ignored.")) {
                {
                    rapidjson::Document Json;
                    Json.Parse(StatusNode->value(), StatusNode->value_size());
                    if (!EGL3_ENSURE(!Json.HasParseError(), LogLevel::Warning, "Failed to parse status json from presence (invalid json). Ignoring presence.")) {
                        return true;
                    }
                    if (!EGL3_ENSURE(Json::PresenceStatus::Parse(Json, ParsedPresence.Status), LogLevel::Warning, "Failed to parse status json from presence. Ignoring presence.")) {
                        return true;
                    }
                }
                {
                    auto DelayNode = Node->first_node("delay", 5);
                    ParsedPresence.LastUpdated = Web::TimePoint::clock::now(); // Set as default
                    if (DelayNode) {
                        auto XmlnsAttr = DelayNode->first_attribute("xmlns", 5);
                        if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <delay>, xmlns=\"urn:xmpp:delay\" expected")) {
                            EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:xmpp:delay"), LogLevel::Warning, "Bad xmlns attr value with <delay>, xmlns=\"urn:xmpp:delay\" expected");
                        }

                        auto StampAttr = DelayNode->first_attribute("stamp", 5);
                        if (EGL3_ENSURE(StampAttr, LogLevel::Warning, "No stamp attr recieved with <delay>")) {
                            EGL3_ENSURE(Web::GetTimePoint(StampAttr->value(), StampAttr->value_size(), ParsedPresence.LastUpdated), LogLevel::Warning, "Bad stamp attr value with <delay>, expected valid ISO8601 datetime. Presence will use current system clock time instead.");
                        }
                    }
                }

                PresenceUpdate(std::string(JIDId), std::move(ParsedPresence));
            }
            return true;
        }
        return false;
    }

    bool XmppClient::HandleSystemMessage(const rapidjson::Document& Data) {
        auto& TypeJson = Data["type"];
        switch (Utils::Crc32(TypeJson.GetString(), TypeJson.GetStringLength())) {
        case Utils::Crc32("FRIENDSHIP_REQUEST"):
        {
            Messages::FriendshipRequest Val;
            if (Messages::FriendshipRequest::Parse(Data, Val)) {
                SystemMessage(Messages::SystemMessage(std::move(Val), CurrentAccountId));
            }
            break;
        }
        case Utils::Crc32("FRIENDSHIP_REMOVE"):
        {
            Messages::FriendshipRemove Val;
            if (Messages::FriendshipRemove::Parse(Data, Val)) {
                SystemMessage(Messages::SystemMessage(std::move(Val), CurrentAccountId));
            }
            break;
        }
        case Utils::Crc32("FRIENDSHIP_ENTRY_UPDATE"):
        {
            Messages::FriendshipEntryUpdate Val;
            if (Messages::FriendshipEntryUpdate::Parse(Data, Val)) {
                SystemMessage(std::move(Val));
            }
            break;
        }
        case Utils::Crc32("USER_BLOCKLIST_UPDATE"):
        {
            Messages::UserBlocklistUpdate Val;
            if (Messages::UserBlocklistUpdate::Parse(Data, Val)) {
                SystemMessage(std::move(Val));
            }
            break;
        }
        case Utils::Crc32("com.epicgames.friends.core.apiobjects.Friend"):
            // FRIENDSHIP_REQUEST handles it
            break;
        case Utils::Crc32("com.epicgames.friends.core.apiobjects.FriendRemoval"):
            // FRIENDSHIP_REMOVE handles it
            break;
        case Utils::Crc32("com.epicgames.friends.core.apiobjects.BlockListEntryAdded"):
        case Utils::Crc32("com.epicgames.friends.core.apiobjects.BlockListEntryRemoved"):
            // USER_BLOCKLIST_UPDATE can handle both of these
            break;
        case Utils::Crc32("com.epicgames.social.interactions.notification.v1"):
            // kind of useless (for egl3 anyway)
            break;
        default:
            EGL3_LOGF(LogLevel::Warning, "Unknown xmpp system message type ({}) found, ignoring", std::string_view{ TypeJson.GetString(), TypeJson.GetStringLength() });
            break;
        }

        return true;
    }

    bool XmppClient::HandleSystemMessage(const rapidxml::xml_node<>* Node) {
        if (XmlNameEqual(Node, "message")) {
            auto FromAttr = Node->first_attribute("from", 4);
            if (!FromAttr) {
                return false;
            }

            auto FromJID = std::string(FromAttr->value(), FromAttr->value_size());
            std::string_view JIDId, JIDDomain, JIDResource;
            if (!ParseJID(FromJID, JIDId, JIDDomain, JIDResource)) {
                return false;
            }
            if (JIDId != "xmpp-admin") {
                return false;
            }

            auto XmlnsAttr = Node->first_attribute("xmlns", 5);
            if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <message>, xmlns=\"jabber:client\" expected")) {
                EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "jabber:client"), LogLevel::Warning, "Bad xmlns attr value with <message>, xmlns=\"jabber:client\" expected");
            }

            auto ToAttr = Node->first_attribute("to", 2);
            if (EGL3_ENSURE(ToAttr, LogLevel::Warning, "No to jid recieved with <message>")) {
                EGL3_ENSURE(XmlValueEqual(ToAttr, CurrentJid), LogLevel::Warning, "Bad type attr value with <message>, expected JID does not match");
            }

            auto BodyNode = Node->first_node("body", 4);
            if (!EGL3_ENSURE(BodyNode, LogLevel::Warning, "No body recieved with <message>. Message will be ignored.")) {
                return true;
            }

            rapidjson::Document Json;
            Json.Parse(BodyNode->value(), BodyNode->value_size());
            if (EGL3_ENSURE(!Json.HasParseError(), LogLevel::Warning, "Failed to parse body json from message (invalid json). Message will be ignored.")) {
                HandleSystemMessage(Json);
            }

            return true;
        }
        return false;
    }

    bool XmppClient::HandleChat(const rapidxml::xml_node<>* Node) {
        if (XmlNameEqual(Node, "message")) {
            auto TypeAttr = Node->first_attribute("type", 4);
            if (!TypeAttr) {
                return false;
            }
            if (!XmlValueEqual(TypeAttr, "chat")) {
                return false;
            }

            auto FromAttr = Node->first_attribute("from", 4);
            if (!FromAttr) {
                return false;
            }

            auto FromJID = std::string(FromAttr->value(), FromAttr->value_size());
            std::string_view JIDId, JIDDomain, JIDResource;
            if (!ParseJID(FromJID, JIDId, JIDDomain, JIDResource)) {
                return false;
            }

            auto XmlnsAttr = Node->first_attribute("xmlns", 5);
            if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <message>, xmlns=\"jabber:client\" expected")) {
                EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "jabber:client"), LogLevel::Warning, "Bad xmlns attr value with <message>, xmlns=\"jabber:client\" expected");
            }

            auto ToAttr = Node->first_attribute("to", 2);
            if (EGL3_ENSURE(ToAttr, LogLevel::Warning, "No to jid recieved with <message>")) {
                EGL3_ENSURE(XmlValueEqual(ToAttr, CurrentJid), LogLevel::Warning, "Bad type attr value with <message>, expected JID does not match");
            }

            auto BodyNode = Node->first_node("body", 4);
            if (!EGL3_ENSURE(BodyNode, LogLevel::Warning, "No body recieved with <message>. Message will be ignored.")) {
                return true;
            }

            ChatRecieved(std::string(JIDId), std::string(BodyNode->value(), BodyNode->value_size()));
            return true;
        }
        return false;
    }

    // https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/XMPP/Private/XmppConnection.cpp#L88
    std::string GenerateResourceId() {
        char Guid[16];
        Utils::GenerateRandomGuid(Guid);
        return "V2:launcher:WIN::" + Utils::ToHex<true>(Guid);
    }

    void SendPing(ix::WebSocket& Socket, const std::string& CurrentJid) {
        char Guid[16];
        Utils::GenerateRandomGuid(Guid);
        auto IdString = Utils::ToHex<true>(Guid);
        auto Document = CreateDocument();
        auto RootNode = Document->allocate_node(rapidxml::node_element, "iq");
        Document->append_node(RootNode);
        RootNode->append_attribute(Document->allocate_attribute("id", IdString.c_str(), 2, IdString.size()));
        RootNode->append_attribute(Document->allocate_attribute("type", "get"));
        RootNode->append_attribute(Document->allocate_attribute("to", "prod.ol.epicgames.com"));
        RootNode->append_attribute(Document->allocate_attribute("from", CurrentJid.c_str(), 4, CurrentJid.size()));
        auto PingNode = Document->allocate_node(rapidxml::node_element, "ping");
        PingNode->append_attribute(Document->allocate_attribute("xmlns", "urn:xmpp:ping"));

        RootNode->append_node(PingNode);
        SendXml(Socket, *Document);
    }

    void XmppClient::BackgroundPingTask()
    {
        while (true) {
            std::chrono::steady_clock::time_point NextTime;
            {
                std::shared_lock Lock(BackgroundPingMutex);
                NextTime = BackgroundPingNextTime;
                BackgroundPingCV.wait_until(Lock, NextTime, [&, this]() {
                    return NextTime != BackgroundPingNextTime || BackgroundPingNextTime == std::chrono::steady_clock::time_point::max();
                });
                NextTime = BackgroundPingNextTime;
            }
            if (NextTime == std::chrono::steady_clock::time_point::max()) {
                return;
            }
            if (NextTime <= std::chrono::steady_clock::now()) {
                SendPing(Socket, CurrentJid);
            }
        }
    }

    void XmppClient::ReceivedMessage(const ix::WebSocketMessagePtr& Message)
    {
        switch (Message->type)
        {
        case ix::WebSocketMessageType::Message:
        {
            auto Document = CreateDocument();
            auto Data = std::make_unique<char[]>(Message->str.size() + 1);
            memcpy(Data.get(), Message->str.c_str(), Message->str.size() + 1); // c_str is required to have a \0 at the end
            // I'd use rapidxml::parse_fastest, but entity translation is pretty necessary
            Document->parse<rapidxml::parse_no_string_terminators | rapidxml::parse_no_data_nodes>(Data.get());

            EGL3_ENSUREF(ParseMessage(Document->first_node()), LogLevel::Warning, "Unrecognized xmpp message: {}", Message->str);
            break;
        }
        case ix::WebSocketMessageType::Open:
        {
            SendXmppOpen(Socket);
            BackgroundPingFuture = std::async(std::launch::async, &XmppClient::BackgroundPingTask, this);
            State = ClientState::BEFORE_OPEN;
            break;
        }
        case ix::WebSocketMessageType::Close:
            EGL3_LOG(LogLevel::Info, "Recieved close");
            break;
        case ix::WebSocketMessageType::Error:
            EGL3_LOG(LogLevel::Info, "Recieved error");
            break;
        case ix::WebSocketMessageType::Ping:
            EGL3_LOG(LogLevel::Info, "Recieved ping");
            break;
        case ix::WebSocketMessageType::Pong:
            EGL3_LOG(LogLevel::Info, "Recieved pong");
            break;
        case ix::WebSocketMessageType::Fragment:
            EGL3_LOG(LogLevel::Info, "Recieved fragment");
            break;
        default:
            EGL3_LOG(LogLevel::Info, "Recieved unknown");
            break;
        }
    }

    bool XmppClient::ParseMessage(const rapidxml::xml_node<>* Node)
    {
        if (!EGL3_ENSURE(Node, LogLevel::Warning, "XmppClient recieved a non-xml message. Ignoring message.")) {
            return false;
        }

        if (HandleSystemMessage(Node)) {
            return true;
        }

        if (HandlePresence(Node)) {
            if (State != ClientState::AUTHENTICATED) {
                FinishAuthentication();
            }
            return true;
        }

        if (HandleChat(Node)) {
            return true;
        }

        if (State == ClientState::AUTHENTICATED) {
            return false;
        }

        switch (State)
        {
        case ClientState::BEFORE_OPEN:
        {
            if (!EGL3_ENSURE(ReadXmppOpen(Node), LogLevel::Error, "Failed to read XmppClient open node. Aborting connection.")) {
                CloseInternal(true);
                return false;
            }

            State = ClientState::BEFORE_FEATURES;
            break;
        }
        case ClientState::BEFORE_FEATURES:
        {
            if (!EGL3_ENSURE(ReadXmppFeatures<false>(Node), LogLevel::Error, "Failed to read XmppClient pre-auth features. Aborting connection.")) {
                CloseInternal(true);
                return false;
            }

            // After grabbing all the stream features (more so just verifying them honestly)
            // we can send an auth response back
            {
                auto Document = CreateDocument();
                auto RootNode = Document->allocate_node(rapidxml::node_element, "auth");
                Document->append_node(RootNode);
                RootNode->append_attribute(Document->allocate_attribute("mechanism", "PLAIN"));
                RootNode->append_attribute(Document->allocate_attribute("xmlns", "urn:ietf:params:xml:ns:xmpp-sasl"));
                RootNode->append_attribute(Document->allocate_attribute("version", "1.0"));
                RootNode->value(EncodedAuthValue.c_str(), EncodedAuthValue.size());
                SendXml(Socket, *Document);
            }

            State = ClientState::BEFORE_AUTH_RESPONSE;
            break;
        }
        case ClientState::BEFORE_AUTH_RESPONSE:
        {
            // Maybe add more error handling? Who knows
            if (!EGL3_ENSURE(XmlNameEqual(Node, "success"), LogLevel::Error, "Failed to authenticate XmppClient correctly. Aborting connection.")) {
                CloseInternal(true);
                return false;
            }

            auto XmlnsAttr = Node->first_attribute("xmlns", 5);
            if (EGL3_ENSURE(XmlnsAttr, LogLevel::Warning, "No xmlns recieved with <success>, xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" expected")) {
                EGL3_ENSURE(XmlValueEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-sasl"), LogLevel::Warning, "Bad xmlns attr value with <success>, xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" expected");
            }

            // After recieving success, we restart the xmpp
            // and we can send an auth response back
            SendXmppOpen(Socket);

            State = ClientState::BEFORE_AUTHED_OPEN;
            break;
        }
        case ClientState::BEFORE_AUTHED_OPEN:
        {
            ReadXmppOpen(Node);

            State = ClientState::BEFORE_AUTHED_FEATURES;
            break;
        }
        case ClientState::BEFORE_AUTHED_FEATURES:
        {
            if (!EGL3_ENSURE(ReadXmppFeatures<true>(Node), LogLevel::Error, "Failed to read XmppClient post-auth features. Aborting connection.")) {
                CloseInternal(true);
                return false;
            }

            CurrentResource = GenerateResourceId();
            CurrentJid.append("/");
            CurrentJid.append(CurrentResource);

            {
                auto Document = CreateDocument();
                auto RootNode = Document->allocate_node(rapidxml::node_element, "iq");
                Document->append_node(RootNode);
                RootNode->append_attribute(Document->allocate_attribute("id", "_xmpp_bind1"));
                RootNode->append_attribute(Document->allocate_attribute("type", "set"));
                auto BindNode = Document->allocate_node(rapidxml::node_element, "bind");
                RootNode->append_node(BindNode);
                BindNode->append_attribute(Document->allocate_attribute("xmlns", "urn:ietf:params:xml:ns:xmpp-bind"));
                auto ResourceNode = Document->allocate_node(rapidxml::node_element, "resource");
                BindNode->append_node(ResourceNode);
                ResourceNode->value(CurrentResource.c_str(), CurrentResource.size());
                SendXml(Socket, *Document);
            }

            State = ClientState::BEFORE_AUTHED_BIND;
            break;
        }
        case ClientState::BEFORE_AUTHED_BIND:
        {
            if (!EGL3_ENSURE(ReadXmppInfoQueryResult<false>(Node, CurrentJid), LogLevel::Error, "Failed to read XmppClient bind infoquery. Aborting connection.")) {
                CloseInternal(true);
                return false;
            }

            {
                auto Document = CreateDocument();
                auto RootNode = Document->allocate_node(rapidxml::node_element, "iq");
                Document->append_node(RootNode);
                RootNode->append_attribute(Document->allocate_attribute("id", "_xmpp_session1"));
                RootNode->append_attribute(Document->allocate_attribute("type", "set"));
                auto SessionNode = Document->allocate_node(rapidxml::node_element, "session");
                RootNode->append_node(SessionNode);
                SessionNode->append_attribute(Document->allocate_attribute("xmlns", "urn:ietf:params:xml:ns:xmpp-session"));
                SendXml(Socket, *Document);
            }

            State = ClientState::BEFORE_AUTHED_SESSION;
            break;
        }
        case ClientState::BEFORE_AUTHED_SESSION:
        {
            if (!EGL3_ENSURE(ReadXmppInfoQueryResult<true>(Node, CurrentJid), LogLevel::Error, "Failed to read XmppClient session infoquery. Aborting connection.")) {
                CloseInternal(true);
                return false;
            }

            FinishAuthentication();
            break;
        }
        default:
            return false;
        }

        return true;
    }
}
