// get_client.hpp
#pragma once

#include <memory>
#include <string>
#include "mino/network_openssl/rest/rest_types.hpp"

namespace mino::network_openssl::rest {

    class get_client
    {
    public:
        using http_status = rest::http_status;
        using result_code = rest::result_code;
        using response = rest::response;
        using query_params = rest::query_params;
        using headers = rest::headers;

        get_client();
        ~get_client();

        void set_server(const std::string& scheme, const std::string& host, long port, const std::string& path);
        void set_headers(const headers& headers);
        void set_timeout_ms(long timeout_ms);
        void set_ignore_ssl_errors(bool ignore);

        response get(const query_params& query_params = {});
        result_code get(const query_params& query_params, response& out_resp) noexcept;

        static result_code classify(const response& resp) { return rest::classify(resp); }

    private:
        struct impl;
        std::unique_ptr<impl> impl_;

        std::string scheme_ = "http";
        std::string host_;
        long        port_ = 0;
        std::string path_;

        headers headers_;
        long timeout_ms_ = 30000;
        bool ignore_ssl_errors_ = false;
    };

}
