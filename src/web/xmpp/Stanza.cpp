#include "Stanza.h"

#include "../../utils/Log.h"

namespace EGL3::Web::Xmpp {
    Stanza::Stanza(xmpp_stanza_t* Ptr) :
        Ptr(Ptr ? xmpp_stanza_clone(Ptr) : nullptr)
    {
        
    }

    Stanza::Stanza(xmpp_ctx_t* Ctx) :
        Ptr(xmpp_stanza_new(Ctx))
    {
        
    }

    Stanza::Stanza() :
        Ptr(nullptr)
    {
    }

    Stanza::Stanza(const Stanza& Other) :
        Stanza(Other.Ptr)
    {
        
    }

    Stanza::Stanza(Stanza&& Other) noexcept :
        Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    Stanza& Stanza::operator=(const Stanza& Other)
    {
        Stanza(Other).Swap(*this);
        return *this;
    }

    Stanza& Stanza::operator=(Stanza&& Other) noexcept
    {
        Stanza(std::move(Other)).Swap(*this);
        return *this;
    }

    Stanza::~Stanza()
    {
        if (Ptr) {
            xmpp_stanza_release(Ptr);
        }
    }

    bool Stanza::IsValid() const
    {
        return Ptr;
    }

    Stanza::operator bool() const
    {
        return IsValid();
    }

    void Stanza::AddChild(Stanza& Stanza)
    {
        xmpp_stanza_add_child(Ptr, Stanza.Ptr);
    }

    Stanza Stanza::GetChild(const char* ChildName) const
    {
        return xmpp_stanza_get_child_by_name(Ptr, ChildName);
    }

    std::vector<Stanza> Stanza::GetChildren() const
    {
        std::vector<Stanza> Ret;
        for (auto Child = xmpp_stanza_get_children(Ptr); Child; Child = xmpp_stanza_get_next(Child)) {
            Ret.emplace_back(Child);
        }

        return Ret;
    }

    void Stanza::SetName(const char* NewName)
    {
        xmpp_stanza_set_name(Ptr, NewName);
    }

    void Stanza::SetNamespace(const char* NewNamespace)
    {
        xmpp_stanza_set_ns(Ptr, NewNamespace);
    }

    void Stanza::SetText(const char* NewText)
    {
        Stanza TextStanza(xmpp_stanza_get_context(Ptr));
        xmpp_stanza_set_text(TextStanza.Ptr, NewText);
        AddChild(TextStanza);
    }

    void Stanza::SetBodyText(const char* NewBodyText)
    {
        xmpp_message_set_body(Ptr, NewBodyText);
    }

    void Stanza::SetType(const char* NewType)
    {
        xmpp_stanza_set_type(Ptr, NewType);
    }

    void Stanza::SetId(const char* NewId)
    {
        xmpp_stanza_set_id(Ptr, NewId);
    }

    void Stanza::SetTo(const char* NewTo)
    {
        xmpp_stanza_set_to(Ptr, NewTo);
    }

    void Stanza::SetFrom(const char* NewFrom)
    {
        xmpp_stanza_set_from(Ptr, NewFrom);
    }

    void Stanza::SetAttribute(const char* Name, const char* NewValue)
    {
        xmpp_stanza_set_attribute(Ptr, Name, NewValue);
    }

    xmpp_stanza_t* Stanza::Get() const
    {
        return Ptr;
    }

    std::string WrapPtr(const char* Ptr) {
        return Ptr ? Ptr : "";
    }

    std::string Stanza::GetName() const
    {
        return WrapPtr(xmpp_stanza_get_name(Ptr));
    }

    std::string Stanza::GetNamespace() const
    {
        return WrapPtr(xmpp_stanza_get_ns(Ptr));
    }

    std::string Stanza::GetText() const
    {
        auto Data = xmpp_stanza_get_text(Ptr);
        auto Ret = WrapPtr(Data);
        xmpp_free(xmpp_stanza_get_context(Ptr), Data);
        return Ret;
    }

    std::string Stanza::GetBodyText() const
    {
        auto Data = xmpp_message_get_body(Ptr);
        auto Ret = WrapPtr(Data);
        xmpp_free(xmpp_stanza_get_context(Ptr), Data);
        return Ret;
    }

    std::string Stanza::GetType() const
    {
        return WrapPtr(xmpp_stanza_get_type(Ptr));
    }

    std::string Stanza::GetId() const
    {
        return WrapPtr(xmpp_stanza_get_id(Ptr));
    }

    std::string Stanza::GetTo() const
    {
        return WrapPtr(xmpp_stanza_get_to(Ptr));
    }

    std::string Stanza::GetFrom() const
    {
        return WrapPtr(xmpp_stanza_get_from(Ptr));
    }

    std::string Stanza::GetAttribute(const char* Name) const
    {
        return WrapPtr(xmpp_stanza_get_attribute(Ptr, Name));
    }

    std::string Stanza::ToString() const
    {
        char* Data;
        size_t Size;
        if (xmpp_stanza_to_text(Ptr, &Data, &Size) == XMPP_EOK) {
            std::string Ret(Data, Size);
            xmpp_free(xmpp_stanza_get_context(Ptr), Data);
            return Ret;
        }

        return "";
    }

    void Stanza::Swap(Stanza& Other)
    {
        std::swap(Ptr, Other.Ptr);
    }
}