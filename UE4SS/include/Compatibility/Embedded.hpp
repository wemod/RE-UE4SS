#pragma once

#include <filesystem>
#include <optional>
#include <string_view>
#include <File/File.hpp>

namespace RC::Compatibility
{
    // Called once before settings and engine initialization. Resources remain in the DLL.
    auto initialize(const std::filesystem::path& executable, const std::filesystem::path& working_directory, const std::filesystem::path& settings_file) -> void;
    auto selected_profile() -> std::string_view;
    auto embedded_override(const std::filesystem::path& path) -> std::optional<std::string_view>;
    auto has_override(const std::filesystem::path& path) -> bool;
    auto read_override(const std::filesystem::path& path) -> std::optional<File::StringType>;
    auto profile_settings() -> std::optional<File::StringType>;
} // namespace RC::Compatibility
