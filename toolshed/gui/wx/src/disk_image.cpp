#include "disk_image.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

extern "C" {
#include <cocopath.h>
#include <os9module.h>
}

namespace toolshed {
namespace {

std::vector<char> mutable_path(const std::string& path)
{
    std::vector<char> value(path.begin(), path.end());
    value.push_back('\0');
    return value;
}

std::runtime_error image_error(const std::string& operation, error_code code)
{
    return std::runtime_error(operation + " failed with ToolShed error " +
                              std::to_string(code));
}

std::string normalize_image_path(std::string path)
{
    while (!path.empty() && path.front() == '/') {
        path.erase(path.begin());
    }
    while (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const auto end = path.find('/', start);
        const auto part = path.substr(start, end == std::string::npos
            ? std::string::npos : end - start);
        if (part == ".." || part == ".") {
            throw std::runtime_error("Relative traversal is not a valid image path");
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return path;
}

std::string join_image_path(const std::string& directory, const std::string& name)
{
    return directory.empty() ? name : directory + "/" + name;
}

std::string attributes_text(int attributes)
{
    struct AttributeName {
        int flag;
        const char* name;
    };
    static constexpr std::array<AttributeName, 7> names = {{
        {FAP_DIR, "d"}, {FAP_SINGLE, "s"}, {FAP_READ, "r"}, {FAP_WRITE, "w"},
        {FAP_EXEC, "e"}, {FAP_PREAD, "pr"}, {FAP_PWRITE, "pw"}
    }};

    std::string result;
    for (const auto& item : names) {
        if ((attributes & item.flag) != 0) {
            if (!result.empty()) {
                result += ' ';
            }
            result += item.name;
        }
    }
    if ((attributes & FAP_PEXEC) != 0) {
        result += result.empty() ? "pe" : " pe";
    }
    return result;
}

class CocoPath {
public:
    CocoPath(const std::string& path, int mode)
    {
        auto value = mutable_path(path);
        const error_code error = _coco_open(&path_, value.data(), mode);
        if (error != 0) {
            throw image_error("Open", error);
        }
    }

    ~CocoPath()
    {
        if (path_ != nullptr) {
            _coco_close(path_);
        }
    }

    CocoPath(const CocoPath&) = delete;
    CocoPath& operator=(const CocoPath&) = delete;

    [[nodiscard]] coco_path_id get() const noexcept { return path_; }

private:
    coco_path_id path_ = nullptr;
};

std::string type_name(const coco_dir_entry& entry)
{
    if (entry.type == OS9) {
        return "OS-9 file";
    }

    if (entry.type == DECB) {
        static constexpr std::array<const char*, 4> names = {
            "BASIC program", "BASIC data", "Machine language", "Text"
        };
        const auto file_type = entry.dentry.decb.file_type;
        return file_type < names.size() ? names[file_type] : "Unknown";
    }

    return "File";
}

std::string lowercase_extension(const std::filesystem::path& path)
{
    auto extension = path.extension().string();
    if (!extension.empty() && extension.front() == '.') {
        extension.erase(extension.begin());
    }
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return extension;
}

std::uint16_t read_be16(const std::uint8_t* bytes)
{
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[0]) << 8) |
                                      bytes[1]);
}

std::uint32_t read_be32(const std::uint8_t* bytes)
{
    return (static_cast<std::uint32_t>(bytes[0]) << 24) |
           (static_cast<std::uint32_t>(bytes[1]) << 16) |
           (static_cast<std::uint32_t>(bytes[2]) << 8) |
           static_cast<std::uint32_t>(bytes[3]);
}

std::string os9_name(const std::uint8_t* module, std::size_t module_size,
                     std::size_t name_offset, std::size_t& edition_offset)
{
    if (name_offset >= module_size - 3) {
        throw std::runtime_error("OS-9 module name is outside the module");
    }

    std::string name;
    for (std::size_t cursor = name_offset; cursor < module_size - 3; ++cursor) {
        const auto value = module[cursor];
        name.push_back(static_cast<char>(value & 0x7f));
        if ((value & 0x80) != 0) {
            edition_offset = cursor + 1;
            return name;
        }
    }
    throw std::runtime_error("OS-9 module name is not terminated");
}

const char* module_type_name(unsigned int type)
{
    static constexpr std::array<const char*, 16> names = {
        "???", "Program", "Subroutine", "Multi-module", "Data", "User 5",
        "User 6", "User 7", "User 8", "User 9", "User A", "User B", "System",
        "File Manager", "Device Driver", "Device Descriptor"
    };
    return names[type & 0x0f];
}

const char* module_language_name(unsigned int language)
{
    static constexpr std::array<const char*, 16> names = {
        "Data", "6809 object", "BASIC09 I-code", "Pascal P-code", "C I-code",
        "COBOL I-code", "FORTRAN I-code", "6309 object", "Unknown", "Unknown",
        "Unknown", "Unknown", "Unknown", "Unknown", "Unknown", "Unknown"
    };
    return names[language & 0x0f];
}

std::string referenced_name(const std::uint8_t* module, std::size_t module_size,
                            std::size_t offset)
{
    std::size_t ignored = 0;
    return os9_name(module, module_size, offset, ignored);
}

std::string identify_osk_modules(std::vector<std::uint8_t>& bytes)
{
    std::ostringstream output;
    std::size_t offset = 0;
    unsigned int module_number = 0;
    while (offset < bytes.size()) {
        if (bytes.size() - offset < OSK_HEADER_SIZE || bytes[offset] != OSK_ID0 ||
            bytes[offset + 1] != OSK_ID1) {
            throw std::runtime_error(module_number == 0
                ? "The selected file is not an OS-9/68K module"
                : "Unexpected data follows the last OS-9/68K module");
        }

        auto* module = bytes.data() + offset;
        const std::size_t module_size = read_be32(module + 4);
        if (module_size < OSK_HEADER_SIZE + 3 || module_size > bytes.size() - offset) {
            throw std::runtime_error("The OS-9/68K module is truncated or has an invalid size");
        }

        const std::size_t name_offset = read_be32(module + 12);
        std::size_t ignored = 0;
        const auto name = os9_name(module, module_size, name_offset, ignored);
        const auto type = module[18] & 0x0f;
        const auto language = module[19] & 0x0f;
        const auto attributes = module[20];
        const auto revision = module[21];

        std::array<u_char, 3> calculated_crc = {0xff, 0xff, 0xff};
        const bool crc_good = _os9_crc_compute(
            module, static_cast<u_int>(module_size), calculated_crc.data()) != 0;
        if (!crc_good) {
            calculated_crc = {0xff, 0xff, 0xff};
            _os9_crc_compute(module, static_cast<u_int>(module_size - 3),
                             calculated_crc.data());
            for (auto& value : calculated_crc) {
                value ^= 0xff;
            }
        }

        if (module_number++ != 0) {
            output << "\n";
        }
        output << "Header for : " << name << "\n"
               << "Module size: $" << std::uppercase << std::hex << module_size
               << std::dec << "  #" << module_size << "\n"
               << "Module CRC : $" << std::uppercase << std::hex << std::setfill('0')
               << std::setw(2) << static_cast<unsigned int>(module[module_size - 3])
               << std::setw(2) << static_cast<unsigned int>(module[module_size - 2])
               << std::setw(2) << static_cast<unsigned int>(module[module_size - 1]);
        if (crc_good) {
            output << "  (Good)\n";
        } else {
            output << "  (Mismatch; calculated $" << std::setw(2)
                   << static_cast<unsigned int>(calculated_crc[0]) << std::setw(2)
                   << static_cast<unsigned int>(calculated_crc[1]) << std::setw(2)
                   << static_cast<unsigned int>(calculated_crc[2]) << ")\n";
        }

        auto* osk_module = reinterpret_cast<OSK_MODULE_t*>(module);
        const auto header_parity = _osk_header(osk_module);
        output << "Hdr parity : $" << std::setw(4) << header_parity
               << (header_parity == 0xffff ? "  (Good)\n" : "  (Bad)\n")
               << "Edition    : $" << std::setw(4) << read_be16(module + 22) << std::dec
               << "  #" << read_be16(module + 22) << "\n"
               << "Ty/La At/Rv: $" << std::hex << std::setw(2)
               << static_cast<unsigned int>(module[18]) << " $" << std::setw(2)
               << static_cast<unsigned int>(module[19]) << " $" << std::setw(2)
               << static_cast<unsigned int>(attributes) << " $" << std::setw(2)
               << static_cast<unsigned int>(revision) << std::dec << "\n";

        if (type == Prgrm && module_size >= 60) {
            output << "Exec. off  : $" << std::hex << std::setw(8) << read_be32(module + 48)
                   << std::dec << "  #" << read_be32(module + 48) << "\n"
                   << "Data size  : $" << std::hex << std::setw(8) << read_be32(module + 56)
                   << std::dec << "  #" << read_be32(module + 56) << "\n";
        } else if (type == Drivr && module_size >= 60) {
            output << "Exec. off  : $" << std::hex << std::setw(8) << read_be32(module + 48)
                   << std::dec << "  #" << read_be32(module + 48) << "\n"
                   << "Data size  : $" << std::hex << std::setw(8) << read_be32(module + 56)
                   << std::dec << "  #" << read_be32(module + 56) << "\n";
        }

        output << module_type_name(type) << " module, "
               << (language == Objct ? "68000 object" : module_language_name(language))
               << ", " << ((attributes & ReEnt) != 0 ? "shareable" : "non-shareable")
               << ", " << ((attributes & Modprot) != 0 ? "sticky" : "non-sticky") << "\n";
        offset += module_size;
    }
    return output.str();
}

}  // namespace

DiskImage::DiskImage(std::filesystem::path path, ImageFormat format)
    : path_(std::move(path)), format_(format)
{
}

DiskImage DiskImage::open(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("The selected image does not exist or is not a regular file");
    }

    const std::string root = path.string() + ",";
    auto value = mutable_path(root);
    _path_type type = NATIVE;
    const error_code error = _coco_identify_image(value.data(), &type);
    if (error != 0) {
        throw image_error("Identify image", error);
    }

    switch (type) {
    case OS9:
        return DiskImage(path, ImageFormat::os9);
    case DECB:
        return DiskImage(path, ImageFormat::disk_basic);
    default:
        throw std::runtime_error("The selected file is not an OS-9 RBF or Disk BASIC image");
    }
}

std::string DiskImage::format_name() const
{
    return format_ == ImageFormat::os9 ? "OS-9 RBF" : "Disk BASIC";
}

std::string DiskImage::toolshed_path(const std::string& image_path) const
{
    return path_.string() + "," + normalize_image_path(image_path);
}

std::vector<Entry> DiskImage::list_directory(const std::string& requested_directory) const
{
    const std::string directory_name = normalize_image_path(requested_directory);
    if (format_ == ImageFormat::disk_basic && !directory_name.empty()) {
        throw std::runtime_error("Disk BASIC images do not contain subdirectories");
    }

    const std::string root = path_.string() + ",";
    const std::string directory_path = root + directory_name;
    const int mode = format_ == ImageFormat::os9 ? FAM_READ | FAM_DIR : FAM_READ;
    CocoPath directory(directory_path, mode);
    std::vector<Entry> entries;

    for (;;) {
        coco_dir_entry native_entry{};
        const error_code error = _coco_readdir(directory.get(), &native_entry);
        if (error != 0) {
            break;
        }

        std::array<u_char, 256> name_buffer{};
        if (_coco_ncpy_name(&native_entry, name_buffer.data(), name_buffer.size()) != 0) {
            continue;
        }

        const std::string name(reinterpret_cast<char*>(name_buffer.data()));
        if (name.empty() || static_cast<unsigned char>(name.front()) == 0xff ||
            name == "." || name == "..") {
            continue;
        }

        Entry entry;
        entry.name = name;
        entry.image_path = join_image_path(directory_name, name);
        entry.type = type_name(native_entry);
        if (format_ == ImageFormat::disk_basic) {
            entry.encoding = native_entry.dentry.decb.ascii_flag == 0xff
                ? "ASCII" : "Binary";
        }

        const std::string child = root + entry.image_path;
        coco_file_stat metadata{};
        if (format_ == ImageFormat::os9) {
            try {
                CocoPath child_directory(child, FAM_READ | FAM_DIR);
                entry.directory = true;
                entry.type = "Folder";
                _coco_gs_fd(child_directory.get(), &metadata);
            } catch (const std::runtime_error&) {
                // It is an ordinary file.
            }
        }

        if (!entry.directory) {
            try {
                CocoPath file(child, FAM_READ);
                u_int size = 0;
                if (_coco_gs_size(file.get(), &size) == 0) {
                    entry.size = size;
                }
                if (_coco_gs_fd(file.get(), &metadata) == 0) {
                    entry.attributes = attributes_text(metadata.attributes);
                    entry.user_id = metadata.user_id;
                    entry.group_id = metadata.group_id;
                    entry.modified_time = format_ == ImageFormat::os9
                        ? static_cast<std::int64_t>(metadata.last_modified_time) : 0;
                }
            } catch (const std::runtime_error&) {
                // Keep the directory entry visible even if its metadata is damaged.
            }
        } else {
            entry.attributes = attributes_text(metadata.attributes);
            entry.user_id = metadata.user_id;
            entry.group_id = metadata.group_id;
            entry.modified_time = static_cast<std::int64_t>(metadata.last_modified_time);
        }

        entries.push_back(std::move(entry));
    }

    return entries;
}

std::vector<std::uint8_t> DiskImage::read_file(const std::string& requested_path) const
{
    const std::string image_path = normalize_image_path(requested_path);
    if (image_path.empty()) {
        throw std::runtime_error("A file must be selected");
    }

    CocoPath file(path_.string() + "," + image_path, FAM_READ);
    u_int size = 0;
    const error_code size_error = _coco_gs_size(file.get(), &size);
    if (size_error != 0) {
        throw image_error("Read file size", size_error);
    }

    std::vector<std::uint8_t> bytes(size);
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto remaining = bytes.size() - offset;
        u_int chunk = static_cast<u_int>(
            std::min<std::size_t>(remaining, std::numeric_limits<u_int>::max()));
        const error_code read_error = _coco_read(file.get(), bytes.data() + offset, &chunk);
        if (read_error != 0 || chunk == 0) {
            throw image_error("Read file", read_error != 0 ? read_error : EOS_EOF);
        }
        offset += chunk;
    }
    return bytes;
}

bool DiskImage::has_module_signature(const std::string& requested_path) const noexcept
{
    try {
        const auto image_path = normalize_image_path(requested_path);
        CocoPath file(path_.string() + "," + image_path, FAM_READ);
        std::array<u_char, 2> signature{};
        u_int size = static_cast<u_int>(signature.size());
        if (_coco_read(file.get(), signature.data(), &size) != 0 || size != signature.size()) {
            return false;
        }
        return (signature[0] == OS9_ID0 && signature[1] == OS9_ID1) ||
               (signature[0] == OSK_ID0 && signature[1] == OSK_ID1);
    } catch (const std::exception&) {
        return false;
    }
}

std::string DiskImage::identify_file(const std::string& image_path) const
{
    auto bytes = read_file(image_path);
    if (bytes.size() >= 2 && bytes[0] == OSK_ID0 && bytes[1] == OSK_ID1) {
        return identify_osk_modules(bytes);
    }
    std::ostringstream output;
    std::size_t offset = 0;
    unsigned int module_number = 0;
    while (offset < bytes.size()) {
        if (bytes.size() - offset < OS9_HEADER_SIZE || bytes[offset] != OS9_ID0 ||
            bytes[offset + 1] != OS9_ID1) {
            throw std::runtime_error(module_number == 0
                ? "The selected file is not an OS-9 module"
                : "Unexpected data follows the last OS-9 module");
        }

        auto* module = bytes.data() + offset;
        const std::size_t module_size = read_be16(module + 2);
        if (module_size < OS9_HEADER_SIZE + 3 || module_size > bytes.size() - offset) {
            throw std::runtime_error("The OS-9 module is truncated or has an invalid size");
        }

        const std::size_t name_offset = read_be16(module + 4);
        std::size_t edition_offset = 0;
        const auto name = os9_name(module, module_size, name_offset, edition_offset);
        if (edition_offset >= module_size - 3) {
            throw std::runtime_error("OS-9 module has no edition byte");
        }
        const auto type_language = module[6];
        const auto attributes_revision = module[7];
        std::uint8_t parity = 0;
        for (std::size_t index = 0; index < OS9_HEADER_SIZE; ++index) {
            parity ^= module[index];
        }

        std::array<u_char, 3> calculated_crc = {0xff, 0xff, 0xff};
        const bool crc_good = _os9_crc_compute(
            module, static_cast<u_int>(module_size), calculated_crc.data()) != 0;
        if (!crc_good) {
            calculated_crc = {0xff, 0xff, 0xff};
            _os9_crc_compute(module, static_cast<u_int>(module_size - 3),
                             calculated_crc.data());
            for (auto& value : calculated_crc) {
                value ^= 0xff;
            }
        }

        if (module_number++ != 0) {
            output << "\n";
        }
        output << "Header for : " << name << "\n"
               << "Module size: $" << std::uppercase << std::hex << module_size
               << std::dec << "  #" << module_size << "\n"
               << "Module CRC : $" << std::uppercase << std::hex << std::setfill('0')
               << std::setw(2) << static_cast<unsigned int>(module[module_size - 3])
               << std::setw(2) << static_cast<unsigned int>(module[module_size - 2])
               << std::setw(2) << static_cast<unsigned int>(module[module_size - 1]);
        if (crc_good) {
            output << "  (Good)\n";
        } else {
            output << "  (Mismatch; calculated $" << std::setw(2)
                   << static_cast<unsigned int>(calculated_crc[0]) << std::setw(2)
                   << static_cast<unsigned int>(calculated_crc[1]) << std::setw(2)
                   << static_cast<unsigned int>(calculated_crc[2]) << ")\n";
        }
        output << "Hdr parity : $" << std::setw(2) << static_cast<unsigned int>(module[8])
               << (parity == 0xff ? "  (Good)\n" : "  (Bad)\n");

        const unsigned int type = (type_language >> 4) & 0x0f;
        if ((type == Prgrm || type == Drivr) && module_size >= 13) {
            const auto exec_offset = read_be16(module + 9);
            const auto data_size = read_be16(module + 11);
            output << "Exec. off  : $" << std::setw(4) << exec_offset << std::dec
                   << "  #" << exec_offset << "\n"
                   << "Data size  : $" << std::hex << std::setw(4) << data_size << std::dec
                   << "  #" << data_size << "\n";
        } else if (type == Devic && module_size >= 13) {
            output << "File Mgr   : "
                   << referenced_name(module, module_size, read_be16(module + 9)) << "\n"
                   << "Driver     : "
                   << referenced_name(module, module_size, read_be16(module + 11)) << "\n";
        }

        output << "Edition    : $" << std::hex << std::setw(2)
               << static_cast<unsigned int>(module[edition_offset]) << std::dec
               << "  #" << static_cast<unsigned int>(module[edition_offset]) << "\n"
               << "Ty/La At/Rv: $" << std::hex << std::setw(2)
               << static_cast<unsigned int>(type_language) << " $" << std::setw(2)
               << static_cast<unsigned int>(attributes_revision) << std::dec << "\n"
               << module_type_name(type) << " module, "
               << module_language_name(type_language & 0x0f) << ", "
               << ((attributes_revision & ReEnt) != 0 ? "re-entrant" : "non-shareable")
               << ", " << ((attributes_revision & Modprot) != 0 ? "R/W" : "R/O") << "\n";

        offset += module_size;
    }

    if (module_number == 0) {
        throw std::runtime_error("The selected file is empty");
    }
    return output.str();
}

void DiskImage::ensure_backup()
{
    if (backup_path_) {
        return;
    }

    auto candidate = std::filesystem::path(path_.string() + ".diskshed-backup");
    for (unsigned int suffix = 1; std::filesystem::exists(candidate); ++suffix) {
        candidate = std::filesystem::path(path_.string() + ".diskshed-backup-" +
                                          std::to_string(suffix));
    }

    std::error_code copy_error;
    if (!std::filesystem::copy_file(path_, candidate,
                                    std::filesystem::copy_options::none, copy_error)) {
        throw std::runtime_error("Could not create image backup: " + copy_error.message());
    }
    backup_path_ = std::move(candidate);
}

void DiskImage::import_file(const std::string& requested_directory,
                            const std::filesystem::path& host_file, bool replace)
{
    if (!std::filesystem::is_regular_file(host_file)) {
        throw std::runtime_error("Only regular files can be imported");
    }

    const auto host_size = std::filesystem::file_size(host_file);
    if (host_size > std::numeric_limits<u_int>::max()) {
        throw std::runtime_error("The host file is too large for ToolShed");
    }

    std::ifstream input(host_file, std::ios::binary);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(host_size));
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input && !bytes.empty()) {
        throw std::runtime_error("Could not read the host file");
    }

    const std::string directory = normalize_image_path(requested_directory);
    const std::string image_path = join_image_path(directory, host_file.filename().string());
    ensure_backup();

    coco_file_stat metadata{};
    metadata.perms = FAP_READ | FAP_WRITE | FAP_PREAD;
    if (format_ == ImageFormat::disk_basic) {
        const std::string extension = lowercase_extension(host_file);
        metadata.file_type = extension == "bas" ? 0 : extension == "dat" ? 1
            : extension == "txt" || extension == "asc" ? 3 : 2;
        metadata.data_type = extension == "bas" || extension == "dat" ||
            extension == "txt" || extension == "asc" ? 0xff : 0;
    }

    auto path_value = mutable_path(toolshed_path(image_path));
    coco_path_id destination = nullptr;
    const int mode = FAM_WRITE | (replace ? 0 : FAM_NOCREATE);
    error_code error = _coco_create(&destination, path_value.data(), mode, &metadata);
    if (error != 0) {
        throw image_error("Create imported file", error);
    }

    std::size_t offset = 0;
    while (offset < bytes.size()) {
        u_int chunk = static_cast<u_int>(std::min<std::size_t>(
            bytes.size() - offset, std::numeric_limits<u_int>::max()));
        error = _coco_write(destination, bytes.data() + offset, &chunk);
        if (error != 0 || chunk == 0) {
            _coco_close(destination);
            throw image_error("Write imported file", error != 0 ? error : EOS_WRITE);
        }
        offset += chunk;
    }
    error = _coco_close(destination);
    if (error != 0) {
        throw image_error("Close imported file", error);
    }
}

void DiskImage::make_directory(const std::string& requested_directory, const std::string& name)
{
    if (format_ != ImageFormat::os9) {
        throw std::runtime_error("Disk BASIC images do not support directories");
    }
    if (name.empty() || name.find('/') != std::string::npos) {
        throw std::runtime_error("Enter a single directory name");
    }

    const std::string image_path = join_image_path(
        normalize_image_path(requested_directory), normalize_image_path(name));
    ensure_backup();
    auto value = mutable_path(toolshed_path(image_path));
    const error_code error = _coco_makdir(value.data());
    if (error != 0) {
        throw image_error("Create directory", error);
    }
}

void DiskImage::rename_entry(const std::string& requested_path, const std::string& new_name)
{
    if (new_name.empty() || new_name.find('/') != std::string::npos) {
        throw std::runtime_error("Enter a single new name");
    }

    ensure_backup();
    auto path_value = mutable_path(toolshed_path(requested_path));
    auto name_value = mutable_path(new_name);
    const error_code error = _coco_rename(path_value.data(), name_value.data());
    if (error != 0) {
        throw image_error("Rename entry", error);
    }
}

void DiskImage::delete_entry(const std::string& requested_path, bool directory)
{
    ensure_backup();
    auto value = mutable_path(toolshed_path(requested_path));
    const error_code error = directory ? _coco_delete_directory(value.data())
                                       : _coco_delete(value.data());
    if (error != 0) {
        throw image_error("Delete entry", error);
    }
}

}  // namespace toolshed
