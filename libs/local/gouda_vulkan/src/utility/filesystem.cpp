#include "utility/filesystem.hpp"

#include <cstring>
#include <fstream>

#include "gouda_throw.hpp"
#include "logger.hpp"

namespace Gouda {
namespace FS {

// Ensure a directory exists or create it if requested
std::expected<void, std::string> EnsureDirectoryExists(const std::filesystem::path &directory_path,
                                                       bool create_if_missing)
{
    if (std::filesystem::exists(directory_path)) {
        if (std::filesystem::is_directory(directory_path)) {
            return {}; // Success
        }
        return std::unexpected("Path exists but is not a directory: " + directory_path.string());
    }

    if (create_if_missing) {
        if (std::filesystem::create_directories(directory_path)) {
            return {};
        }
        return std::unexpected("Failed to create directory: " + directory_path.string());
    }

    return std::unexpected("Directory does not exist and creation was not allowed: " + directory_path.string());
}

// Read an entire file into a string
std::expected<std::string, std::string> ReadFile(std::string_view file_name)
{
    std::filesystem::path file_path(file_name);
    std::ifstream file(file_path, std::ios::in);

    if (!file) {
        return std::unexpected("Failed to open file: " + file_path.string());
    }

    std::string out_file((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!file.eof() && file.fail()) {
        return std::unexpected("Error reading file: " + file_path.string());
    }

    return out_file;
}

// Read a binary file into a vector of bytes
std::expected<std::vector<char>, std::string> ReadBinaryFile(std::string_view file_name)
{
    std::filesystem::path file_path(file_name);
    std::ifstream file(file_path, std::ios::binary | std::ios::ate); // Open at end

    if (!file) {
        return std::unexpected("Failed to open binary file: " + file_path.string());
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        return std::unexpected("File is empty or unreadable: " + file_path.string());
    }

    std::vector<char> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(buffer.data(), size);

    if (!file) {
        return std::unexpected("Error reading binary file: " + file_path.string());
    }

    return buffer;
}

// Write a binary file from a vector of bytes
std::expected<void, std::string> WriteBinaryFile(std::string_view file_name, const std::vector<char> &data)
{
    if (file_name.empty()) {
        return std::unexpected("Empty filename provided");
    }

    std::filesystem::path file_path(file_name);
    std::ofstream file(file_path, std::ios::binary);

    if (!file) {
        return std::unexpected("Error opening file for writing: " + file_path.string());
    }

    file.write(data.data(), static_cast<std::streamsize>(data.size()));

    if (!file) {
        return std::unexpected("Error writing to file: " + file_path.string());
    }

    return {}; // Success
}

// Write a binary file from a vector of u32 values
std::expected<void, std::string> WriteBinaryFile(std::string_view file_name, const std::vector<u32> &data)
{
    std::vector<char> char_data(reinterpret_cast<const char *>(data.data()),
                                reinterpret_cast<const char *>(data.data()) + data.size() * sizeof(u32));

    // Explicitly handle the result from the other function
    auto result = WriteBinaryFile(file_name, char_data);
    if (!result) {
        return std::unexpected(result.error()); // Properly propagate the error
    }

    return {}; // Success
}

// Get current working directory
std::string GetCurrentWorkingDirectory() { return std::filesystem::current_path().string(); }

// List all files in a directory
std::expected<std::vector<std::filesystem::path>, std::string>
ListFilesInDirectory(const std::filesystem::path &dir_path)
{
    std::vector<std::filesystem::path> file_list;

    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
        return std::unexpected("Invalid directory: " + dir_path.string());
    }

    for (const auto &entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            file_list.push_back(entry.path());
        }
    }

    return file_list;
}

// Delete a file
std::expected<void, std::string> DeleteFile(const std::filesystem::path &file_path)
{
    if (std::filesystem::remove(file_path)) {
        return {};
    }
    return std::unexpected("Failed to delete file: " + file_path.string());
}

// Delete a directory and its contents
std::expected<void, std::string> DeleteDirectory(const std::filesystem::path &dir_path)
{
    std::uintmax_t removed = std::filesystem::remove_all(dir_path);
    if (removed > 0) {
        return {};
    }
    return std::unexpected("Failed to delete directory: " + dir_path.string());
}

// Check if a file is empty
bool IsFileEmpty(const std::filesystem::path &file_path)
{
    return std::filesystem::exists(file_path) && std::filesystem::file_size(file_path) == 0;
}

// Get file extension
std::string GetFileExtension(const std::filesystem::path &file_path) { return file_path.extension().string(); }

} // namespace FS
} // namespace Gouda
