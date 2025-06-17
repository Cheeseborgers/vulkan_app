#include "utils/filesystem.hpp"

#include <fstream>

#include "debug/logger.hpp"
#include "debug/throw.hpp"


namespace gouda::fs {

Expect<void, Error> EnsureDirectoryExists(const FilePath &path, const bool create_if_missing)
{
    FilePath directory_path = path;

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
        if (std::error_code ec; std::filesystem::create_directories(directory_path, ec)) {
            return {}; // Success
        }
        return std::unexpected(Error::DirectoryCreationFailed);
    }

    return std::unexpected(Error::DirectoryNotFound);
}

// Read an entire file into a string
Expect<std::string, Error> ReadFile(StringView file_name)
{
    FilePath file_path(file_name);
    std::ifstream file(file_path, std::ios::in);

    if (!file) {
        return std::unexpected(Error::FileNotFound);
    }

    String out_file((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!file.eof() && file.fail()) {
        return std::unexpected(Error::FileReadError);
    }

    return out_file;
}

Expect<void, Error> WriteFile(StringView file_name, const String &data)
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

Expect<std::vector<std::byte>, Error> ReadBinaryFile(StringView file_name)
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

    std::vector<std::byte> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), size);

    if (!file) {
        return std::unexpected(Error::FileReadError);
    }

    return buffer;
}

Expect<void, Error> WriteBinaryFile(StringView file_name, std::span<const std::byte> data)
{
    if (file_name.empty()) {
        return std::unexpected(Error::EmptyFileName);
    }

    FilePath file_path(file_name);
    std::ofstream file(file_path, std::ios::binary);

    if (!file) {
        return std::unexpected(Error::FileWriteError);
    }

    file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));

    if (!file) {
        return std::unexpected(Error::FileWriteError);
    }

    return {}; // Success
}

Expect<void, Error> WriteBinaryFile(StringView file_name, const std::vector<u32> &data)
{
    if (data.empty()) {
        return {}; // Nothing to write, but not an error
    }

    // Safely reinterpret as bytes using std::span
    const auto byte_span = std::as_bytes(std::span<const u32>(data));
    return WriteBinaryFile(file_name, byte_span);
}

Expect<void, Error> WriteBinaryFile(StringView file_name, const std::vector<std::byte> &data)
{
    return WriteBinaryFile(file_name, std::span<const std::byte>(data));
}

bool IsFileExists(StringView file_path) { return std::filesystem::exists(file_path); }

bool IsFileEmpty(StringView file_path) { return std::filesystem::file_size(file_path) == 0; }

String GetCurrentWorkingDirectory() { return std::filesystem::current_path().string(); }

Expect<std::vector<FilePath>, Error> ListFilesInDirectory(StringView dir_path)
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

Expect<void, Error> DeleteFile(StringView file_path)
{
    if (std::filesystem::remove(file_path)) {
        return {};
    }
    return std::unexpected(Error::FileDeleteFailed);
}

Expect<void, Error> DeleteDirectory(StringView dir_path)
{
    if (const std::uintmax_t removed = std::filesystem::remove_all(dir_path); removed > 0) {
        return {};
    }
    return std::unexpected(Error::DirectoryDeleteFailed);
}

String GetFileExtension(StringView file_path)
{
    return FilePath{file_path}.extension().string();
}

String GetFileName(StringView file_path)
{
    return FilePath{file_path}.stem().string();
}

FileTimeType GetLastWriteTime(StringView file_path)
{
    return std::filesystem::last_write_time(file_path);
}

} // namespace gouda::fs

