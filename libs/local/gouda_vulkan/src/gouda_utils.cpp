#include "gouda_utils.hpp"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "gouda_types.hpp"

namespace GoudaUtils {

bool EnsureDirectoryExists(const std::string &directory_path, bool create_if_missing)
{
    std::filesystem::path dir_path(directory_path);

    if (std::filesystem::exists(dir_path)) {
        return std::filesystem::is_directory(dir_path); // Return true if it is a directory
    }

    if (create_if_missing) {
        return std::filesystem::create_directories(dir_path); // Create directory and return result
    }

    return false; // Directory does not exist, and we did not create it
}

bool ReadFile(std::string_view file_name, std::string &out_file)
{
    log(std::format("Current working directory: {}", std::filesystem::current_path().string()), "INFO");

    std::filesystem::path absolute_path = std::filesystem::absolute(file_name);
    log(std::format("Attempting to read: {}", absolute_path.string()));

    if (!std::filesystem::exists(absolute_path)) {
        log(std::format("File does not exist ffff: {}", absolute_path.string()), "ERROR");
        return false;
    }
    std::filesystem::path file_path(file_name);
    std::ifstream file(file_path, std::ios::in);

    if (!file) {
        // TODO: Implemement proper logging facilities
        log(std::format("Failed to open file: {}", file_name), "ERROR");
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        out_file += line + "\n";
    }

    if (!file.eof()) {
        // TODO: Implemement proper logging facilities
        log("Error reading file contents", "ERROR");
        return false;
    }

    // TODO: Implemement proper logging facilities
    log(std::format("Successfully read file: {}", file_name));
    return true; // true if we reached end of file without errors
}

std::vector<char> ReadBinaryFile(std::string_view file_name)
{
    std::filesystem::path file_path(file_name);
    std::ifstream file(file_path, std::ios::binary);

    if (!file) {
        // TODO: Implemement proper logging facilities
        log(std::format("Error opening '{}': {}", file_name, std::strerror(errno)), "ERROR");
        throw std::runtime_error("Failed to open file");
    }

    std::vector<char> buffer(std::filesystem::file_size(file_path));
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    if (!file) {
        // TODO: Implemement proper logging facilities
        log(std::format("Read file error for '{}': {}", file_name, std::strerror(errno)), "ERROR");
        throw std::runtime_error("Failed to read file");
    }

    return buffer;
}

void WriteBinaryFile(std::string_view file_name, const std::vector<char> &data)
{
    if (file_name.empty()) {
        log("Empty filename provided", "ERROR");
        throw std::runtime_error("Empty filename provided");
    }

    std::filesystem::path file_path(file_name);
    std::ofstream file(file_path, std::ios::binary);

    if (!file) {
        // TODO: Implemement proper logging facilities
        log(std::format("Error opening '{}' for writing: {}", file_name, std::strerror(errno)), "ERROR");
        throw std::runtime_error("Failed to open file for writing");
    }

    file.write(data.data(), static_cast<std::streamsize>(data.size()));

    if (!file) {
        // TODO: Implemement proper logging facilities
        log(std::format("Error writing to file '{}': {}", file_name, std::strerror(errno)), "ERROR");
        throw std::runtime_error("Failed to write file");
    }

    log(std::format("Successfully wrote binary file: {}", file_name));
}

void WriteBinaryFile(std::string_view file_name, const std::vector<u32> &data)
{
    // Convert to std::vector<char> inside the function
    std::vector<char> char_data(reinterpret_cast<const char *>(data.data()),
                                reinterpret_cast<const char *>(data.data()) + data.size() * sizeof(u32));
    // Call the original function
    WriteBinaryFile(file_name, char_data);
}

std::string GetCurrentWorkingDirectory() { return std::filesystem::current_path().string(); }

void log(const std::string_view &message, const std::string_view &level)
{
    auto now = SystemClock::now();
    auto in_time_t = SystemClock::to_time_t(now);

    std::stringstream time_stream;
    time_stream << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");

    std::cout << std::format("[{}][{}] {}\n", time_stream.str(), level, message);
}

} // end namespace
