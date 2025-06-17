#pragma once
/**
 * @file utils/filesystem.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-13
 *
 * @brief Filesystem utilities for file and directory operations.
 *
 * Provides functions for reading, writing, deleting, and managing files
 * and directories using modern C++ and std::filesystem.
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <vector> // TODO: Use small vector

#include "core/types.hpp"

namespace gouda::fs {

/**
 * @enum Error
 * @brief Enum representing possible filesystem errors.
 */
enum class Error : u8 {
    None,                    ///< No error occurred
    PathNotDirectory,        ///< Path exists but is not a directory
    DirectoryCreationFailed, ///< Failed to create directory
    FileNotFound,            ///< File not found
    FileReadError,           ///< Error reading file
    FileWriteError,          ///< Error writing to file
    EmptyFileName,           ///< Provided file name is empty
    DirectoryNotFound,       ///< Directory not found
    DirectoryInvalid,        ///< Invalid directory path
    FileDeleteFailed,        ///< Failed to delete file
    DirectoryDeleteFailed    ///< Failed to delete directory
};

constexpr StringView error_to_string(Error error) noexcept
{
    constexpr std::array<StringView, 11> error_strings{
        "No error occurred",           "Path exists but is not a directory",
        "Failed to create directory",  "File not found",
        "Error reading file",          "Error writing to file",
        "Provided file name is empty", "Directory not found",
        "Invalid directory path",      "Failed to delete file",
        "Failed to delete directory"};

    const auto index = static_cast<std::size_t>(error);
    return index < error_strings.size() ? error_strings[index] : "Unknown error";
}

/**
 * @brief Ensures a directory exists, creating it if necessary.
 * @param path Path to the directory.
 * @param create_if_missing Whether to create the directory if missing.
 * @return Success or an Error code.
 */
[[nodiscard]] Expect<void, Error> EnsureDirectoryExists(const FilePath &path, bool create_if_missing);

/**
 * @brief Reads an entire file into a string.
 * @param file_name Name of the file to read.
 * @return File content or an Error code.
 */
[[nodiscard]] Expect<std::string, Error> ReadFile(StringView file_name);

/**
 * @brief Writes a string to a file.
 * @param file_name Name of the file.
 * @param data Data to write.
 * @return Success or an Error code.
 */
[[nodiscard]] Expect<void, Error> WriteFile(StringView file_name, const String &data);

/**
 * @brief Reads an entire binary file into a vector of bytes.
 * @param file_name Name of the file to read.
 * @return File content or an Error code.
 */
[[nodiscard]] Expect<std::vector<std::byte>, Error> ReadBinaryFile(StringView file_name);

/**
 * @brief Writes a binary file from a vector of bytes.
 * @param file_name Name of the file.
 * @param data Data to write.
 * @return Success or an Error code.
 */
[[nodiscard]] Expect<void, Error> WriteBinaryFile(StringView file_name, std::span<const std::byte> data);

/**
 * @brief Writes a binary file from a vector of u32 values.
 * @param file_name Name of the file.
 * @param data Data to write.
 * @return Success or an Error code.
 */
[[nodiscard]] Expect<void, Error> WriteBinaryFile(StringView file_name, const std::vector<u32> &data);

/**
 * @brief Writes a binary file from a vector of byte values.
 * @param file_name Name of the file.
 * @param data Data to write.
 * @return Success or an Error code.
 */
[[nodiscard]] Expect<void, Error> WriteBinaryFile(StringView file_name, const std::vector<std::byte> &data);

/**
 * @brief Checks if a file exists.
 * @param filepath Path to the file.
 * @return True if the file exists, false otherwise.
 */
[[nodiscard]] bool IsFileExists(StringView filepath);

/**
 * @brief Checks if a file is empty.
 * @param filepath Path to the file.
 * @return True if the file is empty, false otherwise.
 */
[[nodiscard]] bool IsFileEmpty(StringView filepath);

/**
 * @brief Gets the current working directory.
 * @return The current working directory as a string.
 */
[[nodiscard]] String GetCurrentWorkingDirectory();

/**
 * @brief Lists all files in a directory.
 * @param dir_path Path to the directory.
 * @return Vector of file paths or an Error code.
 */
[[nodiscard]] Expect<std::vector<FilePath>, Error> ListFilesInDirectory(StringView dir_path);

/**
 * @brief Deletes a file.
 * @param filepath Path to the file.
 * @return Success or an Error code.
 */
[[nodiscard]] Expect<void, Error> DeleteFile(StringView filepath);

/**
 * @brief Deletes a directory and all its contents.
 * @param dir_path Path to the directory.
 * @return Success or an Error code.
 */
[[nodiscard]] Expect<void, Error> DeleteDirectory(StringView dir_path);

/**
 * @brief Gets the file extension from a file path.
 * @param filepath Path to the file.
 * @return The file extension as a string.
 */
[[nodiscard]] String GetFileExtension(StringView filepath);

/**
 * @brief Gets the file name excluding the file extension from a file path.
 * @param filepath Path to the file.
 * @return The file name as a string.
 */
[[nodiscard]] String GetFileName(StringView filepath);

/**
 * @brief Gets the last time the file was writen.
 * @param filepath Path to the file.
 * @return The last write time as FileTimeType (std::chrono timepoint).
 */
[[nodiscard]] FileTimeType GetLastWriteTime(StringView filepath);


}