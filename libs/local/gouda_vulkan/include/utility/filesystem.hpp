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

#include <string>
#include <vector>

#include "core/types.hpp"

namespace gouda {
namespace fs {

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

/**
 * @brief Ensures a directory exists, creating it if necessary.
 * @param path Path to the directory.
 * @param create_if_missing Whether to create the directory if missing.
 * @return Success or an Error code.
 */
Expect<void, Error> EnsureDirectoryExists(const FilePath &path, bool create_if_missing);

/**
 * @brief Reads an entire file into a string.
 * @param file_name Name of the file to read.
 * @return File content or an Error code.
 */
Expect<std::string, Error> ReadFile(std::string_view file_name);

/**
 * @brief Writes a string to a file.
 * @param file_name Name of the file.
 * @param data Data to write.
 * @return Success or an Error code.
 */
Expect<void, Error> WriteFile(std::string_view file_name, const std::string &data);

/**
 * @brief Reads an entire binary file into a vector of bytes.
 * @param file_name Name of the file to read.
 * @return File content or an Error code.
 */
Expect<std::vector<char>, Error> ReadBinaryFile(std::string_view file_name);

/**
 * @brief Writes a binary file from a vector of bytes.
 * @param file_name Name of the file.
 * @param data Data to write.
 * @return Success or an Error code.
 */
Expect<void, Error> WriteBinaryFile(std::string_view file_name, const std::vector<char> &data);

/**
 * @brief Writes a binary file from a vector of u32 values.
 * @param file_name Name of the file.
 * @param data Data to write.
 * @return Success or an Error code.
 */
Expect<void, Error> WriteBinaryFile(std::string_view file_name, const std::vector<u32> &data);

/**
 * @brief Checks if a file exists.
 * @param file_path Path to the file.
 * @return True if the file exists, false otherwise.
 */
bool IsFileExists(const FilePath &file_path);

/**
 * @brief Checks if a file is empty.
 * @param file_path Path to the file.
 * @return True if the file is empty, false otherwise.
 */
bool IsFileEmpty(const FilePath &file_path);

/**
 * @brief Gets the current working directory.
 * @return The current working directory as a string.
 */
std::string GetCurrentWorkingDirectory();

/**
 * @brief Lists all files in a directory.
 * @param dir_path Path to the directory.
 * @return Vector of file paths or an Error code.
 */
Expect<std::vector<FilePath>, Error> ListFilesInDirectory(const FilePath &dir_path);

/**
 * @brief Deletes a file.
 * @param file_path Path to the file.
 * @return Success or an Error code.
 */
Expect<void, Error> DeleteFile(const FilePath &file_path);

/**
 * @brief Deletes a directory and all its contents.
 * @param dir_path Path to the directory.
 * @return Success or an Error code.
 */
Expect<void, Error> DeleteDirectory(const FilePath &dir_path);

/**
 * @brief Gets the file extension from a file path.
 * @param file_path Path to the file.
 * @return The file extension as a string.
 */
std::string GetFileExtension(const FilePath &file_path);

} // namespace fs
} // namespace gouda
