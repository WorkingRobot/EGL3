#pragma once

#include <strophe.h>

#include <string>
#include <vector>

namespace EGL3::Web::Xmpp {
    class Stanza {
    public:
        Stanza(xmpp_stanza_t* PtrCopy);
        explicit Stanza(xmpp_ctx_t* Ctx);

        Stanza();
        Stanza(const Stanza&);
        Stanza(Stanza&&) noexcept;
        Stanza& operator=(const Stanza&);
        Stanza& operator=(Stanza&&) noexcept;

        ~Stanza();

        bool IsValid() const;
        explicit operator bool() const;

        void AddChild(Stanza& Stanza);

        Stanza GetChild(const char* ChildName) const;
        std::vector<Stanza> GetChildren() const;

        void SetName(const char* NewName);
        void SetNamespace(const char* NewNamespace);
        void SetText(const char* NewText);
        void SetBodyText(const char* NewBodyText);
        void SetType(const char* NewType);
        void SetId(const char* NewId);
        void SetTo(const char* NewTo);
        void SetFrom(const char* NewFrom);
        void SetAttribute(const char* Name, const char* NewValue);

        xmpp_stanza_t* Get() const;
        std::string GetName() const;
        std::string GetNamespace() const;
        std::string GetText() const;
        std::string GetBodyText() const;
        std::string GetType() const;
        std::string GetId() const;
        std::string GetTo() const;
        std::string GetFrom() const;
        std::string GetAttribute(const char* Name) const;

        std::string ToString() const;

    private:
        void Swap(Stanza& Other);

        xmpp_stanza_t* Ptr;
    };
}