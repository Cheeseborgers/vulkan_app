#pragma once

#include <string>
#include <vector>

#include "gouda_types.hpp"

namespace GoudaUtils {

bool EnsureDirectoryExists(const std::string &directory_path, bool create_if_missing = false);

bool ReadFile(std::string_view file_name, std::string &out_file);

std::vector<char> ReadBinaryFile(std::string_view file_name);

void WriteBinaryFile(std::string_view file_name, const std::vector<char> &data);
void WriteBinaryFile(std::string_view file_name, const std::vector<u32> &data);

std::string GetCurrentWorkingDirectory();

void log(const std::string_view &message, const std::string_view &level = "INFO");

}