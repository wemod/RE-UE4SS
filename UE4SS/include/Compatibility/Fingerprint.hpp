#pragma once
#include <filesystem>
#include <string>
namespace RC::Compatibility
{
    auto executable_sha256(const std::filesystem::path& path) -> std::string;
}
