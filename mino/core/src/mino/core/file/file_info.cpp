#include "mino/core/file/file_info.hpp"

namespace mino::core::file {

    file_info::file_info(const std::string& path)
        : path_(path)
    {
    }
     
    bool file_info::exists() const {
        return std::filesystem::exists(path_);
    }

    bool file_info::is_file() const {
        return std::filesystem::is_regular_file(path_);
    }

    bool file_info::is_dir() const {
        return std::filesystem::is_directory(path_);
    }

    std::string file_info::file_name() const {
        return path_.filename().string();
    }

    std::string file_info::extension() const {
        return path_.extension().string();
    }

    std::string file_info::parent_path() const {
        return path_.parent_path().string();
    }

    std::string file_info::absolute_path() const {
        return std::filesystem::absolute(path_).string();
    }

    std::uintmax_t file_info::file_size() const {
        if (is_file()) {
            return std::filesystem::file_size(path_);
        }
        return 0;
    }

} // namespace mino::core::file
