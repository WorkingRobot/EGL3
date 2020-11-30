#include "XmppClient.h"

#include <ixwebsocket/IXNetSystem.h>
#include <rapidxml/rapidxml_print.hpp>
#include <glibmm/ustring.h>

#include "../../utils/Assert.h"
#include "../../utils/Base64.h"
#include "../../utils/Hex.h"
#include "../../utils/RandGuid.h"
#include "../../utils/streams/MemoryStream.h"

namespace EGL3::Web::Xmpp {
	XmppClient::XmppClient(const std::string& AccountId, const std::string& AccessToken) : State(ClientState::OPENING)
	{
		std::call_once(InitFlag, []() {printf("initializing system\n"); ix::initNetSystem(); printf("initalized system\n"); });

		Socket.setUrl("wss://xmpp-service-prod.ol.epicgames.com//");
		Socket.addSubProtocol("xmpp");
		Socket.setOnMessageCallback([this](auto& Message) { ReceivedMessage(Message); });
		Socket.setTrafficTrackerCallback([](size_t size, bool incoming) {
			printf("size %zu, %s\n", size, incoming ? "in" : "out");
		});

		{
			Utils::Streams::MemoryStream AuthStream;
			AuthStream << '\0';
			AuthStream.write(AccountId.c_str(), AccountId.size());
			AuthStream << '\0';
			AuthStream.write(AccessToken.c_str(), AccessToken.size());
			EncodedAuthValue = Utils::B64Encode((uint8_t*)AuthStream.get(), AuthStream.size());
		}
		CurrentJid = AccountId + "@prod.ol.epicgames.com";

		printf("starting\n");
		Socket.start();
	}

	void XmppClient::KillAuthentication()
	{
		//BackgroundPingCancelled = true;
		BackgroundPingFuture.get();
		// TODO: move this to when gtk is closing the app,
		// so we can ensure safety when 2+ clients are used
		ix::uninitNetSystem();
	}

	std::unique_ptr<rapidxml::xml_document<>> CreateDocument() {
		return std::make_unique<rapidxml::xml_document<>>();
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
		if (XmlStringSize != SourceString.size() - 1) {
			return false;
		}
		return memcmp(XmlString, SourceString.c_str(), SourceString.size() - 1) == 0;
	}

	template<size_t SourceStringSize>
	bool XmlNameEqual(const rapidxml::xml_base<>* Node, const char(&SourceString)[SourceStringSize]) {
		return XmlStringEqual(Node->name(), Node->name_size(), SourceString);
	}

	bool XmlNameEqual(const rapidxml::xml_base<>* Node, const std::string& SourceString) {
		return XmlStringEqual(Node->name(), Node->name_size(), SourceString);
	}

	void SendXml(ix::WebSocket& Socket, const rapidxml::xml_node<>& Node)
	{
		std::string OutputString;
		rapidxml::print(std::back_inserter(OutputString), Node, rapidxml::print_no_indenting);
		Socket.sendText(OutputString);
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

	void ReadXmppOpen(const rapidxml::xml_node<>* Node)
	{
		// RFC6120 section 4.7 should be adhered to but I'm just doing whatever works fine
		// https://tools.ietf.org/html/rfc6120#section-4.7
		EGL3_ASSERT(XmlNameEqual(Node, "open"), "Bad node name, <open> expected");

		auto XmlnsAttr = Node->first_attribute("xmlns", 5);
		EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <open>, xmlns=\"urn:ietf:params:xml:ns:xmpp-framing\" expected");
		EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-framing"), "Bad xmlns attr value with <open>, xmlns=\"urn:ietf:params:xml:ns:xmpp-framing\" expected");

		auto FromAttr = Node->first_attribute("from", 4); // JID
		EGL3_ASSERT(FromAttr, "RFC 6120 4.7.1 (Page 37) requires from attr to be given by server");
		EGL3_ASSERT(XmlNameEqual(FromAttr, "prod.ol.epicgames.com"), "Bad from attr value with <open>, from=\"prod.ol.epicgames.com\" expected");

		auto IdAttr = Node->first_attribute("id", 2);
		EGL3_ASSERT(IdAttr, "RFC 6120 4.7.3 (Page 39) requires id attr to be given by server");

		auto VersionAttr = Node->first_attribute("version", 7);
		EGL3_ASSERT(VersionAttr, "RFC 6120 4.7.5 (Page 42) requires version attr to be given by server");
		EGL3_ASSERT(XmlNameEqual(VersionAttr, "1.0"), "Bad version attr value with <open>, version=\"1.0\" expected");

		auto LangAttr = Node->first_attribute("xml:lang", 8);
		EGL3_ASSERT(LangAttr, "RFC 6120 4.7.4 (Page 40) requires xml:lang attr to be given by server");
		EGL3_ASSERT(XmlNameEqual(LangAttr, "en"), "Bad xml:lang attr value with <open>, xml:lang=\"en\" expected");
	}

	template<bool Authed>
	void ReadXmppFeatures(const rapidxml::xml_node<>* Node)
	{
		// <stream:features> (root node)
		{
			// I believe this can also be just <features>, but this is able to be kept for backwards compat
			EGL3_ASSERT(XmlNameEqual(Node, "stream:features"), "Bad node name, <stream:features> expected");

			auto XmlnsAttr = Node->first_attribute("xmlns:stream", 12);
			EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <stream:features>, xmlns:stream=\"http://etherx.jabber.org/streams\" expected");
			EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "http://etherx.jabber.org/streams"), "Bad xmlns:stream attr value with <stream:features>, xmlns:stream=\"http://etherx.jabber.org/streams\" expected");
		}

		// <ver>
		{
			// Roster versioning isn't used (or at least, I don't actually see it used),
			// but it's required to be listed as a stream feature if the server supports it
			// RFC 6121 2.6
			auto VerNode = Node->first_node("ver", 3);
			if (VerNode) {
				auto XmlnsAttr = VerNode->first_attribute("xmlns", 5);
				EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <ver>, xmlns=\"urn:xmpp:features:rosterver\" expected");
				EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "urn:xmpp:features:rosterver"), "Bad xmlns attr value with <ver>, xmlns=\"urn:xmpp:features:rosterver\" expected");
			}
			else {
				// WARN: roster versioning node is usually given by epic
			}
		}

		// <starttls>
		{
			// RFC 6120 6.4.1 says that the server must provide the feature in the response if it's capable
			// We must assert that <required/> is not a child element
			// We don't know how to manage STARTTLS, and we're already connected via wss anyway
			auto TlsNode = Node->first_node("starttls", 8);
			if (TlsNode) {
				auto XmlnsAttr = TlsNode->first_attribute("xmlns", 5);
				EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <starttls>, xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\" expected");
				EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-tls"), "Bad xmlns attr value with <starttls>, xmlns=\"urn:ietf:params:xml:ns:xmpp-tls\" expected");

				EGL3_ASSERT(TlsNode->first_node("required", 8) == NULL, "STARTTLS is required. We do not support it");
			}
			else {
				// WARN: starttls node is usually given by epic
			}
		}

		// <compression>
		{
			// XEP-0138 has definitions for what the feature should have
			// We aren't required to pick one if we don't support any (we don't)
			// Epic servers offer zlib, but the official launcher doesn't use it anyway
			auto CompressionNode = Node->first_node("compression", 11);
			if (CompressionNode) {
				auto XmlnsAttr = CompressionNode->first_attribute("xmlns", 5);
				EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <compression>, xmlns=\"http://jabber.org/features/compress\" expected");
				EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "http://jabber.org/features/compress"), "Bad xmlns attr value with <compression>, xmlns=\"http://jabber.org/features/compress\" expected");

				EGL3_ASSERT(CompressionNode->first_node("method", 6), "Compression nodes must have at least one included method");
			}
			else {
				// WARN: compression node is usually given by epic
			}
		}

		// <mechanisms>
		if constexpr (!Authed)
		{
			auto MechanismsNode = Node->first_node("mechanisms", 10);
			// It's not given the next time when the stream is restarted since SASL/auth is already negotiated before that
			EGL3_ASSERT(MechanismsNode, "RFC 6120 6.4.1 (Page 82) requires mechanisms node to be given on first connection");

			auto XmlnsAttr = MechanismsNode->first_attribute("xmlns", 5);
			EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <mechanisms>, xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" expected");
			EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-sasl"), "Bad xmlns attr value with <mechanisms>, xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" expected");

			bool PlainAuthExists = false;
			for (auto MechanismNode = MechanismsNode->first_node(); MechanismNode; MechanismNode = MechanismNode->next_sibling()) {
				if (XmlStringEqual(MechanismNode->value(), MechanismNode->value_size(), "PLAIN")) {
					PlainAuthExists = true;
					break;
				}
			}
			EGL3_ASSERT(!PlainAuthExists, "Only the PLAIN SASL mechanism is supported");
		}

		// <auth>
		if constexpr (!Authed)
		{
			// XEP-0078's iq-auth feature is given to us, but we never use it (allows for non-SASL auth)
			// The official launcher uses SASL's PLAIN mechanism to login anyway
			auto IqAuthNode = Node->first_node("auth", 4);
			if (IqAuthNode) {
				auto XmlnsAttr = IqAuthNode->first_attribute("xmlns", 5);
				EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <auth>, xmlns=\"http://jabber.org/features/iq-auth\" expected");
				EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "http://jabber.org/features/iq-auth"), "Bad xmlns attr value with <auth>, xmlns=\"http://jabber.org/features/iq-auth\" expected");
			}
			else {
				// WARN: iq auth node is usually given by epic
			}
		}

		// <bind>
		if constexpr (Authed)
		{
			// RFC 6120 7.4
			// If a client gets authenticated, it must be provided a bind feature, and never otherwise
			auto BindNode = Node->first_node("bind", 4);
			EGL3_ASSERT(BindNode, "Bind node must be present after authenticating");

			auto XmlnsAttr = BindNode->first_attribute("xmlns", 5);
			EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <bind>, xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\" expected");
			EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-bind"), "Bad xmlns attr value with <bind>, xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\" expected");
		}

		// <session>
		if constexpr (Authed)
		{
			// RFC 3921 3.
			// Epic servers support sessions, and must be provided to the client
			auto SessionNode = Node->first_node("session", 7);
			EGL3_ASSERT(SessionNode, "Bind node must be present after authenticating");

			auto XmlnsAttr = SessionNode->first_attribute("xmlns", 5);
			EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <session>, xmlns=\"urn:ietf:params:xml:ns:xmpp-session\" expected");
			EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-session"), "Bad xmlns attr value with <session>, xmlns=\"urn:ietf:params:xml:ns:xmpp-session\" expected");
		}
	}

	// RFC6120 7.6.1
	template<bool Session>
	void ReadXmppInfoQueryResult(const rapidxml::xml_node<>* Node, const std::string& CurrentJid)
	{
		// <iq> (root node)
		{
			EGL3_ASSERT(XmlNameEqual(Node, "iq"), "Bad node name, <iq> expected");

			auto XmlnsAttr = Node->first_attribute("xmlns", 5);
			EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <iq>, xmlns=\"jabber:client\" expected");
			EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "jabber:client"), "Bad xmlns attr value with <iq>, xmlns=\"jabber:client\" expected");

			auto TypeAttr = Node->first_attribute("type", 4);
			EGL3_ASSERT(TypeAttr, "No type recieved with <iq>, type=\"result\" expected");
			EGL3_ASSERT(XmlNameEqual(TypeAttr, "result"), "Bad type attr value with <iq>, type=\"result\" expected");

			auto ToAttr = Node->first_attribute("to", 2);
			EGL3_ASSERT(ToAttr, "No to jid recieved with <iq>");
			EGL3_ASSERT(XmlNameEqual(ToAttr, CurrentJid), "Bad type attr value with <iq>, expected JID does not match");

			auto IdAttr = Node->first_attribute("id", 2);
			if constexpr (!Session) {
				EGL3_ASSERT(IdAttr, "No id recieved with <iq>, id=\"_xmpp_bind1\" expected");
				EGL3_ASSERT(XmlNameEqual(IdAttr, "_xmpp_bind1"), "Bad id attr value with <iq>, id=\"_xmpp_bind1\" expected");
			}
			else {
				EGL3_ASSERT(IdAttr, "No id recieved with <iq>, id=\"_xmpp_session1\" expected");
				EGL3_ASSERT(XmlNameEqual(IdAttr, "_xmpp_session1"), "Bad id attr value with <iq>, id=\"_xmpp_session1\" expected");
			}
		}

		if constexpr (!Session)
		{
			// <bind>
			auto BindNode = Node->first_node("bind", 4);
			{
				EGL3_ASSERT(BindNode, "Bind node must be present in a binding success result");

				auto XmlnsAttr = BindNode->first_attribute("xmlns", 5);
				EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <bind>, xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\" expected");
				EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-bind"), "Bad xmlns attr value with <bind>, xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\" expected");
			}

			// <jid>
			{
				auto JidNode = BindNode->first_node("jid", 3);
				EGL3_ASSERT(JidNode, "Jid must be given back to the client in a binding success result");

				EGL3_ASSERT(XmlStringEqual(JidNode->value(), JidNode->value_size(), CurrentJid), "Jid must be given back to the client in a binding success result");
			}
		}
	}

	// https://github.com/EpicGames/UnrealEngine/blob/df84cb430f38ad08ad831f31267d8702b2fefc3e/Engine/Source/Runtime/Online/XMPP/Private/XmppConnection.cpp#L88
	std::string GenerateResourceId() {
		uint8_t Guid[16];
		Utils::GenerateRandomGuid(Guid);
		return "V2:launcher:WIN::" + Utils::ToHex<true>(Guid);
	}

	void XmppClient::BackgroundPingTask()
	{
		std::chrono::steady_clock::time_point NextTime = BackgroundPingNextTime;
		while (NextTime != std::chrono::steady_clock::time_point::max()) {
			if (NextTime <= std::chrono::steady_clock::now()) {
				// send ping

				// Only if the value hasn't changed, set it to a minute forward
				BackgroundPingNextTime.compare_exchange_weak(NextTime, NextTime + std::chrono::seconds(60));
			}
			std::this_thread::yield();
			NextTime = BackgroundPingNextTime;
		}
	}

	void XmppClient::ReceivedMessage(const ix::WebSocketMessagePtr& Message)
	{
		switch (Message->type)
		{
		case ix::WebSocketMessageType::Message:
		{
			printf("Recieved message\n");
			auto Document = CreateDocument();
			auto Data = std::make_unique<char[]>(Message->str.size() + 1);
			memcpy(Data.get(), Message->str.c_str(), Message->str.size() + 1); // c_str is required to have a \0 at the end
			Document->parse<0>(Data.get());

			ParseMessage(Document->first_node());
			break;
		}
		case ix::WebSocketMessageType::Open:
		{
			printf("Recieved open\n");
			SendXmppOpen(Socket);
			BackgroundPingFuture = std::async(std::launch::async, [this]() {
				BackgroundPingTask();
			});
			State = ClientState::BEFORE_OPEN;
			break;
		}
		case ix::WebSocketMessageType::Close:
			printf("Recieved close\n");
			break;
		case ix::WebSocketMessageType::Error:
			printf("Recieved error\n");
			break;
		case ix::WebSocketMessageType::Ping:
			printf("Recieved ping\n");
			break;
		case ix::WebSocketMessageType::Pong:
			printf("Recieved pong\n");
			break;
		case ix::WebSocketMessageType::Fragment:
			printf("Recieved fragment\n");
			break;
		default:
			printf("Recieved unknown\n");
			break;
		}
	}

	void XmppClient::ParseMessage(const rapidxml::xml_node<>* Node)
	{
		EGL3_ASSERT(Node, "No xml recieved?");
		switch (State)
		{
		case ClientState::BEFORE_OPEN:
		{
			ReadXmppOpen(Node);

			State = ClientState::BEFORE_FEATURES;
			break;
		}
		case ClientState::BEFORE_FEATURES:
		{
			ReadXmppFeatures<false>(Node);

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
			EGL3_ASSERT(XmlNameEqual(Node, "success"), "Failed to authenticate?");

			auto XmlnsAttr = Node->first_attribute("xmlns", 5);
			EGL3_ASSERT(XmlnsAttr, "No xmlns recieved with <success>, xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" expected");
			EGL3_ASSERT(XmlNameEqual(XmlnsAttr, "urn:ietf:params:xml:ns:xmpp-sasl"), "Bad xmlns attr value with <success>, xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" expected");

			// After recieving success, we restart the xmpp
			// we can send an auth response back
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
			ReadXmppFeatures<true>(Node);

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
				ResourceNode->value(CurrentJid.c_str(), CurrentJid.size());
				SendXml(Socket, *Document);
			}

			State = ClientState::BEFORE_AUTHED_BIND;
			break;
		}
		case ClientState::BEFORE_AUTHED_BIND:
		{
			ReadXmppInfoQueryResult<false>(Node, CurrentJid);

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
			ReadXmppInfoQueryResult<true>(Node, CurrentJid);

			State = ClientState::AUTHENTICATED;
			break;
		}
		case ClientState::AUTHENTICATED:
			break;
		default:
			break;
		}
	}
}
