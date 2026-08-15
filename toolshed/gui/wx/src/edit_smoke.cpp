#include "disk_image.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

void usage()
{
    std::cerr << "usage:\n"
                 "  toolshed-image-edit-smoke IMAGE import HOST_FILE DIRECTORY\n"
                 "  toolshed-image-edit-smoke IMAGE mkdir DIRECTORY NAME\n"
                 "  toolshed-image-edit-smoke IMAGE rename IMAGE_PATH NEW_NAME\n"
                 "  toolshed-image-edit-smoke IMAGE delete IMAGE_PATH file|directory\n";
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 5) {
        usage();
        return 2;
    }

    try {
        auto image = toolshed::DiskImage::open(argv[1]);
        const std::string command = argv[2];
        if (command == "import") {
            image.import_file(argv[4], argv[3]);
        } else if (command == "mkdir") {
            image.make_directory(argv[3], argv[4]);
        } else if (command == "rename") {
            image.rename_entry(argv[3], argv[4]);
        } else if (command == "delete") {
            const std::string kind = argv[4];
            if (kind != "file" && kind != "directory") {
                usage();
                return 2;
            }
            image.delete_entry(argv[3], kind == "directory");
        } else {
            usage();
            return 2;
        }

        if (image.backup_path()) {
            std::cout << "backup " << image.backup_path()->string() << "\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
