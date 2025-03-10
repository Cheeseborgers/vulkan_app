#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "core/types.hpp"

namespace Gouda {
namespace FS {

/**
 * @brief Ensures that a directory exists, creating it if requested.
 *
 * @param directory_path The path of the directory to check.
 * @param create_if_missing If true, the directory will be created if it does not exist.
 * @return std::expected<void, std::string>
 *         - Success: void (empty expected)
 *         - Failure: std::string describing the error
 */
std::expected<void, std::string> EnsureDirectoryExists(const std::filesystem::path &directory_path,
                                                       bool create_if_missing);

/**
 * @brief Reads an entire file into a string.
 *
 * @param file_name The name of the file to read.
 * @return std::expected<std::string, std::string>
 *         - Success: The file contents as a string.
 *         - Failure: std::string describing the error.
 */
std::expected<std::string, std::string> ReadFile(std::string_view file_name);

/**
 * @brief Reads a binary file into a vector of bytes.
 *
 * @param file_name The name of the file to read.
 * @return std::expected<std::vector<char>, std::string>
 *         - Success: A vector containing the file's binary data.
 *         - Failure: std::string describing the error.
 */
std::expected<std::vector<char>, std::string> ReadBinaryFile(std::string_view file_name);

/**
 * @brief Writes binary data to a file.
 *
 * @param file_name The name of the file to write.
 * @param data The binary data to write to the file.
 * @return std::expected<void, std::string>
 *         - Success: void (empty expected)
 *         - Failure: std::string describing the error.
 */
std::expected<void, std::string> WriteBinaryFile(std::string_view file_name, const std::vector<char> &data);

/**
 * @brief Writes a vector of 32-bit unsigned integers to a binary file.
 *
 * @param file_name The name of the file to write.
 * @param data The vector of u32 values to write.
 * @return std::expected<void, std::string>
 *         - Success: void (empty expected)
 *         - Failure: std::string describing the error.
 */
std::expected<void, std::string> WriteBinaryFile(std::string_view file_name, const std::vector<u32> &data);

/**
 * @brief Retrieves the current working directory as a string.
 *
 * @return std::string The current working directory.
 */
std::string GetCurrentWorkingDirectory();

/**
 * @brief Lists all files in a specified directory.
 *
 * @param dir_path The directory to list files from.
 * @return std::expected<std::vector<std::filesystem::path>, std::string>
 *         - Success: A vector containing the paths of the files in the directory.
 *         - Failure: std::string describing the error.
 */
std::expected<std::vector<std::filesystem::path>, std::string>
ListFilesInDirectory(const std::filesystem::path &dir_path);

/**
 * @brief Deletes a specified file.
 *
 * @param file_path The path of the file to delete.
 * @return std::expected<void, std::string>
 *         - Success: void (empty expected)
 *         - Failure: std::string describing the error.
 */
std::expected<void, std::string> DeleteFile(const std::filesystem::path &file_path);

/**
 * @brief Deletes a directory and its contents.
 *
 * @param dir_path The path of the directory to delete.
 * @return std::expected<void, std::string>
 *         - Success: void (empty expected)
 *         - Failure: std::string describing the error.
 */
std::expected<void, std::string> DeleteDirectory(const std::filesystem::path &dir_path);

/**
 * @brief Checks if a file is empty.
 *
 * @param file_path The path of the file to check.
 * @return true if the file is empty, false otherwise.
 */
bool IsFileEmpty(const std::filesystem::path &file_path);

/**
 * @brief Gets the file extension of a given file path.
 *
 * @param file_path The path of the file.
 * @return std::string The file extension (e.g., ".txt", ".png"). Returns an empty string if no extension is found.
 */
std::string GetFileExtension(const std::filesystem::path &file_path);

} // namespace FS
} // namespace Gouda
