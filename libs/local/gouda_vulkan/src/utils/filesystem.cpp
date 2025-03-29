#include "utils/filesystem.hpp"

#include <cstring>
#include <fstream>

#include "debug/logger.hpp"
#include "debug/throw.hpp"

namespace gouda {
namespace fs {

Expect<void, Error> EnsureDirectoryExists(const FilePath &path, bool create_if_missing)
{
    std::filesystem::path directory_path = path;

    // Ensure we're working with a directory, trimming off the filename if necessary
    if (std::filesystem::exists(directory_path)) {
        if (std::filesystem::is_regular_file(directory_path)) {
            directory_path = directory_path.parent_path(); // Trim to parent directory
        }
        else if (!std::filesystem::is_directory(directory_path)) {
            return std::unexpected(Error::PathNotDirectory);
        }
    }

    // Ensure the directory exists or create it
    if (std::filesystem::exists(directory_path)) {
        if (std::filesystem::is_directory(directory_path)) {
            return {}; // Success
        }
        return std::unexpected(Error::PathNotDirectory);
    }

    if (create_if_missing) {
        std::error_code ec;
        if (std::filesystem::create_directories(directory_path, ec)) {
            return {}; // Success
        }
        return std::unexpected(Error::DirectoryCreationFailed);
    }

    return std::unexpected(Error::DirectoryNotFound);
}

// Read an entire file into a string
Expect<std::string, Error> ReadFile(std::string_view file_name)
{
    FilePath file_path(file_name);
    std::ifstream file(file_path, std::ios::in);

    if (!file) {
        return std::unexpected(Error::FileNotFound);
    }

    std::string out_file((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!file.eof() && file.fail()) {
        return std::unexpected(Error::FileReadError);
    }

    return out_file;
}

Expect<void, Error> WriteFile(std::string_view file_name, const std::string &data)
{
    if (file_name.empty()) {
        return std::unexpected(Error::EmptyFileName);
    }

    std::ofstream file(FilePath(file_name), std::ios::out | std::ios::trunc);
    if (!file) {
        return std::unexpected(Error::FileWriteError);
    }

    file << data;
    return {}; // Success
}

Expect<std::vector<char>, Error> ReadBinaryFile(std::string_view file_name)
{
    FilePath file_path(file_name);
    std::ifstream file(file_path, std::ios::binary | std::ios::ate); // Open at end

    if (!file) {
        return std::unexpected(Error::FileNotFound);
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        return std::unexpected(Error::FileReadError);
    }

    std::vector<char> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(buffer.data(), size);

    if (!file) {
        return std::unexpected(Error::FileReadError);
    }

    return buffer;
}

Expect<void, Error> WriteBinaryFile(std::string_view file_name, const std::vector<char> &data)
{
    if (file_name.empty()) {
        return std::unexpected(Error::EmptyFileName);
    }

    FilePath file_path(file_name);
    std::ofstream file(file_path, std::ios::binary);

    if (!file) {
        return std::unexpected(Error::FileWriteError);
    }

    file.write(data.data(), static_cast<std::streamsize>(data.size()));

    if (!file) {
        return std::unexpected(Error::FileWriteError);
    }

    return {}; // Success
}

Expect<void, Error> WriteBinaryFile(std::string_view file_name, const std::vector<u32> &data)
{
    std::vector<char> char_data(reinterpret_cast<const char *>(data.data()),
                                reinterpret_cast<const char *>(data.data()) + data.size() * sizeof(u32));

    auto result = WriteBinaryFile(file_name, char_data);
    if (!result) {
        return std::unexpected(result.error()); // Propagate the error
    }

    return {}; // Success
}

bool IsFileExists(const FilePath &file_path) { return std::filesystem::exists(file_path); }

bool IsFileEmpty(const FilePath &file_path) { return std::filesystem::file_size(file_path) == 0; }

std::string GetCurrentWorkingDirectory() { return std::filesystem::current_path().string(); }

Expect<std::vector<FilePath>, Error> ListFilesInDirectory(const FilePath &dir_path)
{
    std::vector<FilePath> file_list;

    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
        return std::unexpected(Error::DirectoryInvalid);
    }

    for (const auto &entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            file_list.push_back(entry.path());
        }
    }

    return file_list;
}

Expect<void, Error> DeleteFile(const FilePath &file_path)
{
    if (std::filesystem::remove(file_path)) {
        return {};
    }
    return std::unexpected(Error::FileDeleteFailed);
}

Expect<void, Error> DeleteDirectory(const FilePath &dir_path)
{
    std::uintmax_t removed = std::filesystem::remove_all(dir_path);
    if (removed > 0) {
        return {};
    }
    return std::unexpected(Error::DirectoryDeleteFailed);
}

std::string GetFileExtension(const FilePath &file_path) { return file_path.extension().string(); }

} // namespace fs
} // namespace gouda
