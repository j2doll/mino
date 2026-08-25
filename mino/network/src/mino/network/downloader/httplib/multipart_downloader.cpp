#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include "mino/network/downloader/httplib/multipart_downloader.hpp"

#ifdef USE_OPENSSL
    #ifndef CPPHTTPLIB_OPENSSL_SUPPORT
        #define CPPHTTPLIB_OPENSSL_SUPPORT // HTTPS 지원이 필요한 경우 활성화
    #endif
#endif

// A C++ header-only HTTP/HTTPS server and client library
// https://github.com/yhirose/cpp-httplib
#include "mino/network/third-party/httplib/httplib.h"

namespace mino::network::downloader::httplib
{

    namespace
    {
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

        // httplib::Headers에서 Content-Type을 찾아 boundary를 추출합니다.
        bool extract_boundary_from_headers(const ::httplib::Headers& headers, std::string& boundary_out)
        {
            auto it = headers.find("Content-Type");
            if (it == headers.end())
            {
                // 대소문자 구분 없이 찾기 위해 추가 검사
                for (const auto& kv : headers)
                {
                    std::string key = kv.first;
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    if (key == "content-type")
                    {
                        it = headers.find(kv.first);
                        break;
                    }
                }
            }

            if (it == headers.end()) return false;

            std::string content_type = it->second;
            std::string lower = content_type;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            auto pos = lower.find("boundary=");
            if (pos == std::string::npos) return false;

            std::string boundary = trim(content_type.substr(pos + 9));
            if (!boundary.empty() && boundary.front() == '"' && boundary.back() == '"')
                boundary = boundary.substr(1, boundary.size() - 2);

            boundary_out = boundary;
            return !boundary_out.empty();
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

        // URL을 scheme, host, path로 분리하는 간단한 헬퍼 함수
        bool parse_url(const std::string& url, std::string& scheme_host, std::string& path)
        {
            size_t pos = url.find("://");
            if (pos == std::string::npos) return false;

            size_t host_start = pos + 3;
            size_t path_start = url.find('/', host_start);

            if (path_start == std::string::npos)
            {
                scheme_host = url;
                path = "/";
            }
            else
            {
                scheme_host = url.substr(0, path_start);
                path = url.substr(path_start);
            }
            return true;
        }

    } // unnamed namespace

    multipart_downloader::multipart_downloader() = default;
    multipart_downloader::~multipart_downloader() = default;

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
        std::string scheme_host;
        std::string path;
        if (!parse_url(url, scheme_host, path))
        {
            if (error_message) *error_message = "Invalid URL format.";
            return false;
        }

        ::httplib::Client cli(scheme_host);
        cli.set_follow_location(true); // Redirect 허용

        // 1. 데이터 수신 및 프로그레스 처리를 위한 준비
        std::string body;
        ::httplib::Headers response_headers;
        bool aborted_by_user = false;

        // Content Receiver: cpp-httplib의 ContentReceiver 시그니처에 맞추어 정의
        auto content_receiver = [&](const char* data, size_t data_length) {
            body.append(data, data_length);
            return true;
        };

        // Progress: cpp-httplib 의 Progress(current, total) 시그니처에 맞추어 정의
        auto progress = [&](uint64_t current, uint64_t total) -> bool {
            if (progress_cb_)
            {
                // multipart_progress_callback::on_progress은 (total, now) 순서이므로 맞춰 호출
                if (!progress_cb_->on_progress(static_cast<long long>(total), static_cast<long long>(current)))
                {
                    aborted_by_user = true;
                    return false;
                }
            }
            return true;
        };

        // 헤더가 먼저 도착했을 때의 처리
        auto response_handler = [&](const ::httplib::Response& response) {
            if (response.status != 200)
            {
                if (error_message) *error_message = "HTTP Error Status: " + std::to_string(response.status);
                return false; // Status가 200이 아니면 수신 중단
            }
            response_headers = response.headers;
            return true;
        };

        // 2. HTTP GET 요청 수행 (response_handler, content_receiver, progress 사용)
        auto res = cli.Get(path, response_handler, content_receiver, progress);

        if (aborted_by_user)
        {
            if (error_message) *error_message = "Download aborted by user.";
            return false;
        }

        if (!res)
        {
            if (error_message)
            {
                std::stringstream ss;
                ss << "HTTP request failed with error code: " << static_cast<int>(res.error());
                *error_message = ss.str();
            }
            return false;
        }

        // 3. Boundary 분리 및 파싱
        std::string boundary;
        if (!extract_boundary_from_headers(response_headers, boundary))
        {
            if (error_message) *error_message = "Failed to extract boundary from headers.";
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
                    if (error_message) *error_message = "Failed to create temp file: " + temp_path.string();
                    if (delete_partial_on_fail_) try_delete_file(temp_path);
                    return false;
                }

                ofs.write(body.data() + data_start, static_cast<std::streamsize>(next - data_start));

                if (!ofs)
                {
                    if (error_message) *error_message = "Failed to write temp file: " + temp_path.string();
                    ofs.close();
                    if (delete_partial_on_fail_) try_delete_file(temp_path);
                    return false;
                }
            }

            std::error_code ec;
            std::filesystem::rename(temp_path, final_path, ec);
            if (ec)
            {
                if (error_message) *error_message = "Failed to rename file: " + final_path.string();
                if (delete_partial_on_fail_) try_delete_file(temp_path);
                return false;
            }

            saved_files.push_back(final_path.string());
            found_any = true;

            pos = next;
            ++part_index;
        }

        if (!found_any)
        {
            if (error_message) *error_message = "No multipart data found.";
            return false;
        }

        return true;
    }

}  
