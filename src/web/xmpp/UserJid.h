#pragma once

#include <string>

namespace EGL3::Web::Xmpp {
    struct UserJid {
        UserJid() = default;
        UserJid(const UserJid&) = delete;
        UserJid(UserJid&&) = delete;

        UserJid(const std::string& Node, const std::string& Domain, const std::string& Resource = "") :
            UserJid(CreateJid(Node, Domain, Resource))
        {

        }

        UserJid(const std::string& DataRef) :
            Data(DataRef)
        {
            auto At = Data.find('@');
            auto Slash = Data.find('/', At == std::string::npos ? 0 : At);

            if (At != std::string::npos) {
                Node = std::string_view(Data.data(), At);
            }

            Domain = std::string_view(Data.data() + (At == std::string::npos ? 0 : At + 1), std::min(Slash - At - 1, Data.size()));

            if (Slash != std::string::npos) {
                Resource = std::string_view(Data.data() + Slash + 1);
            }
        }

        bool IsValid() const {
            return !Data.empty();
        }
        
        const std::string& GetString() const {
            return Data;
        }

        const std::string_view& GetNode() const {
            return Node;
        }

        const std::string_view& GetDomain() const {
            return Domain;
        }

        const std::string_view& GetResource() const {
            return Resource;
        }

    private:
        static std::string CreateJid(const std::string& Node, const std::string& Domain, const std::string& Resource) {
            std::string Data;

            if (!Node.empty()) {
                Data += Node;
                Data += '@';
            }

            Data += Domain;

            if (!Resource.empty()) {
                Data += '/';
                Data += Resource;
            }

            return Data;
        }

        std::string Data;
        std::string_view Node;
        std::string_view Domain;
        std::string_view Resource;
    };
}