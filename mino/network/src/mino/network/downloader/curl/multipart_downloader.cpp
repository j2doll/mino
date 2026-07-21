#ifdef USE_CURL

#include <mutex>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include <curl/curl.h>

#include "mino/network/downloader/curl/multipart_downloader.hpp"

namespace mino::network::downloader::curl
{
    namespace
    {
        std::once_flag g_curl_global_init_flag;

        size_t write_body_callback(void* ptr, size_t size, size_t nmemb, void* userdata)
        {
            size_t total = size * nmemb;
            auto* body = static_cast<std::string*>(userdata);
            body->append(static_cast<const char*>(ptr), total);
            return total;
        }

        size_t write_header_callback(char* buffer, size_t size, size_t nitems, void* userdata)
        {
            size_t total = size * nitems;
            auto* headers = static_cast<std::string*>(userdata);
            headers->append(buffer, total);
            return total;
        }

        int xfer_info_callback(void* clientp,
            curl_off_t dltotal, curl_off_t dlnow,
            curl_off_t, curl_off_t)
        {
            auto* self = static_cast<multipart_downloader*>(clientp);
            if (!self || !self->progress_cb_)
                return 0;

            bool keep = self->progress_cb_->on_progress(
                static_cast<long long>(dltotal),
                static_cast<long long>(dlnow));

            return keep ? 0 : 1;
        }

        std::string trim(const std::string& s)
        {
            auto begin = s.begin();
            while (begin != s.end() && std::isspace(static_cast<unsigned char>(*begin)))
                ++begin;

            auto end = s.end();
            while (end != begin)
            {
                auto prev = end;
                --prev;
                if (!std::isspace(static_cast<unsigned char>(*prev)))
                    break;
                end = prev;
            }
            return std::string(begin, end);
        }

        bool extract_boundary_from_headers(const std::string& headers, std::string& boundary_out)
        {
            std::istringstream iss(headers);
            std::string line;

            while (std::getline(iss, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();

                std::string lower = line;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower.rfind("content-type:", 0) == 0)
                {
                    auto pos = lower.find("boundary=");
                    if (pos == std::string::npos)
                        continue;

                    std::string boundary = trim(line.substr(pos + 9));
                    if (!boundary.empty() && boundary.front() == '"' && boundary.back() == '"')
                        boundary = boundary.substr(1, boundary.size() - 2);

                    boundary_out = boundary;
                    return true;
                }
            }
            return false;
        }

        bool extract_filename_from_content_disposition(const std::string& header_block,
            std::string& filename_out)
        {
            std::istringstream iss(header_block);
            std::string line;

            while (std::getline(iss, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();

                std::string lower = line;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower.rfind("content-disposition:", 0) == 0)
                {
                    auto pos = lower.find("filename=");
                    if (pos == std::string::npos)
                        return false;

                    std::string name = trim(line.substr(pos + 9));
                    if (!name.empty() && name.front() == '"' && name.back() == '"')
                        name = name.substr(1, name.size() - 2);

                    filename_out = name;
                    return !filename_out.empty();
                }
            }
            return false;
        }

        void try_delete_file(const std::filesystem::path& path)
        {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }

    } // unnamed namespace

    multipart_downloader::multipart_downloader()
    {
        ensure_global_init();
    }

    multipart_downloader::~multipart_downloader() = default;

    void multipart_downloader::ensure_global_init()
    {
        std::call_once(g_curl_global_init_flag, []() {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            });
    }

    void multipart_downloader::set_progress_callback(multipart_progress_callback* cb)
    {
        progress_cb_ = cb;
    }

    void multipart_downloader::set_delete_partial_file_on_fail(bool enable)
    {
        delete_partial_on_fail_ = enable;
    }

    bool multipart_downloader::download_multipart(const std::string& url,
        const std::string& output_dir,
        std::vector<std::string>& saved_files,
        std::string* error_message)
    {
        ensure_global_init();

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            if (error_message) *error_message = "curl initialization failed";
            return false;
        }

        std::string body;
        std::string headers;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_body_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xfer_info_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            if (error_message)
            {
                *error_message =
                    (res == CURLE_ABORTED_BY_CALLBACK)
                    ? "Download aborted by user."
                    : curl_easy_strerror(res);
            }
            curl_easy_cleanup(curl);
            return false;
        }

        std::string boundary;
        if (!extract_boundary_from_headers(headers, boundary))
        {
            if (error_message) {
                *error_message = "Failed to extract boundary";
            }
            curl_easy_cleanup(curl);
            return false;
        }

        const std::string marker = "--" + boundary;
        const std::string end_marker = marker + "--";

        std::filesystem::path out_dir(output_dir);

        size_t pos = 0;
        size_t part_index = 0;
        bool found_any = false;

        while (true)
        {
            size_t bpos = body.find(marker, pos);
            if (bpos == std::string::npos)
                break;

            if (body.compare(bpos, end_marker.size(), end_marker) == 0)
                break;

            size_t cur = bpos + marker.size();
            if (body.compare(cur, 2, "\r\n") == 0)
                cur += 2;

            size_t header_end = body.find("\r\n\r\n", cur);
            if (header_end == std::string::npos)
            {
                if (error_message) *error_message = "Failed to parse multipart headers";
                curl_easy_cleanup(curl);
                return false;
            }

            std::string header_block = body.substr(cur, header_end - cur);

            size_t data_start = header_end + 4;
            size_t next = body.find("\r\n" + marker, data_start);
            if (next == std::string::npos)
                next = body.size();

            std::string filename;
            if (!extract_filename_from_content_disposition(header_block, filename))
                filename = "part_" + std::to_string(part_index) + ".bin";

            std::filesystem::path final_path = out_dir / filename;
            std::filesystem::path temp_path = final_path;
            temp_path += ".part";

            {
                std::ofstream ofs(temp_path, std::ios::binary);
                if (!ofs)
                {
                    if (error_message)
                        *error_message = "Failed to create temp file: " + temp_path.string();
                    if (delete_partial_on_fail_)
                        try_delete_file(temp_path);
                    curl_easy_cleanup(curl);
                    return false;
                }

                ofs.write(body.data() + data_start,
                    static_cast<std::streamsize>(next - data_start));

                if (!ofs)
                {
                    if (error_message)
                        *error_message = "Failed to write temp file: " + temp_path.string();
                    ofs.close();
                    if (delete_partial_on_fail_)
                        try_delete_file(temp_path);
                    curl_easy_cleanup(curl);
                    return false;
                }
            }

            std::error_code ec;
            std::filesystem::rename(temp_path, final_path, ec);
            if (ec)
            {
                if (error_message)
                    *error_message = "Failed to rename file: " + final_path.string();
                if (delete_partial_on_fail_)
                    try_delete_file(temp_path);
                curl_easy_cleanup(curl);
                return false;
            }

            saved_files.push_back(final_path.string());
            found_any = true;

            pos = next;
            ++part_index;
        }

        curl_easy_cleanup(curl);

        if (!found_any)
        {
            if (error_message) *error_message = "No multipart data found.";
            return false;
        }

        return true;
    }

}  


#endif // USE_CURL
