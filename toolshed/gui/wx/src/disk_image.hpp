#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace toolshed {

enum class ImageFormat {
    os9,
    disk_basic,
};

struct Entry {
    std::string name;
    std::string image_path;
    std::uint64_t size = 0;
    bool directory = false;
    std::string type;
    std::string encoding;
    std::string attributes;
    unsigned int user_id = 0;
    unsigned int group_id = 0;
    std::int64_t modified_time = 0;
};

class DiskImage {
public:
    static DiskImage open(const std::filesystem::path& path);

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] ImageFormat format() const noexcept { return format_; }
    [[nodiscard]] std::string format_name() const;
    [[nodiscard]] std::vector<Entry> list_directory(const std::string& directory = {}) const;
    [[nodiscard]] std::vector<std::uint8_t> read_file(const std::string& image_path) const;
    [[nodiscard]] bool has_module_signature(const std::string& image_path) const noexcept;
    [[nodiscard]] std::string identify_file(const std::string& image_path) const;
    void import_file(const std::string& directory, const std::filesystem::path& host_file,
                     bool replace = false);
    void make_directory(const std::string& directory, const std::string& name);
    void rename_entry(const std::string& image_path, const std::string& new_name);
    void delete_entry(const std::string& image_path, bool directory);
    [[nodiscard]] const std::optional<std::filesystem::path>& backup_path() const noexcept
    {
        return backup_path_;
    }

private:
    DiskImage(std::filesystem::path path, ImageFormat format);
    void ensure_backup();
    [[nodiscard]] std::string toolshed_path(const std::string& image_path) const;

    std::filesystem::path path_;
    ImageFormat format_;
    std::optional<std::filesystem::path> backup_path_;
};

}  // namespace toolshed
