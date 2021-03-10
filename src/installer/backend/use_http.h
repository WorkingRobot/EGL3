#pragma once

#include "consts.h"
#include "httplib.h"

class SSLPinner {
public:
    SSLPinner() {
        valid = false;
        delete_file = false;
        httplib::SSLClient cli("curl.se");
        cli.set_compress(true);
        auto resp = cli.Get("/ca/cacert.pem");
        if (resp == nullptr) {
            return;
        }
        if (tmpnam_s(filename, L_tmpnam_s)) {
            return;
        }
        delete_file = true;
        if (FILE* file = fopen(filename, "wb")) {
            fwrite(resp->body.data(), 1, resp->body.size(), file);
            fclose(file);
        }
        else {
            return;
        }
        cli.set_ca_cert_path(filename);
        cli.enable_server_certificate_verification(true);
        valid = cli.Get("/ca/cacert.pem") != nullptr;
    }

    ~SSLPinner() {
        if (delete_file) {
            remove(filename);
        }
    }

    bool valid;
    char filename[L_tmpnam_s];

private:
    bool delete_file;
};