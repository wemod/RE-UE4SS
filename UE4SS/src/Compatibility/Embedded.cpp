#include <Compatibility/Embedded.hpp>
#include <Compatibility/Catalog.hpp>
#include <Compatibility/Fingerprint.hpp>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <Helpers/String.hpp>
#include <IniParser/Ini.hpp>

namespace RC::Compatibility
{
    namespace
    {
        std::filesystem::path resource_root;
        std::string_view active_profile;
    } // namespace

    auto initialize(const std::filesystem::path& executable, const std::filesystem::path& working_directory, const std::filesystem::path& settings_file) -> void
    {
        resource_root = working_directory.lexically_normal();
        active_profile = {};
        std::string requested = "Auto";
        if (std::filesystem::exists(settings_file))
        {
            auto file = File::open(settings_file);
            Ini::Parser parser;
            parser.parse(file);
            requested = to_string(parser.get_string(STR("Compatibility"), STR("Profile"), STR("Auto")));
        }
        char* environment_profile{};
        size_t environment_length{};
        if (_dupenv_s(&environment_profile, &environment_length, "UE4SS_COMPATIBILITY_PROFILE") != 0)
        {
            throw std::runtime_error{"Could not read compatibility profile environment override"};
        }
        const std::unique_ptr<char, decltype(&std::free)> override_profile{environment_profile, &std::free};
        if (override_profile && *override_profile) requested = override_profile.get();
        const auto catalog = builtin_catalog();
        const auto name = executable.filename().string();
        const auto automatic = requested.empty() || String::iequal(requested, "Auto");
        const auto digest = automatic && catalog.needs_fingerprint(name) ? executable_sha256(executable) : std::string{};
        active_profile = catalog.select(requested, name, digest);
    }

    auto selected_profile() -> std::string_view
    {
        return active_profile;
    }

    auto embedded_override(const std::filesystem::path& path) -> std::optional<std::string_view>
    {
        const auto relative = path.lexically_normal().lexically_relative(resource_root).generic_string();
        if (const auto* resource = builtin_catalog().find(active_profile, relative)) return resource->contents;
        return std::nullopt;
    }

    auto has_override(const std::filesystem::path& path) -> bool
    {
        return std::filesystem::exists(path) || embedded_override(path).has_value();
    }

    auto read_override(const std::filesystem::path& path) -> std::optional<File::StringType>
    {
        // An external file wins, including an empty file used to suppress a built-in layout.
        if (std::filesystem::exists(path)) return File::open(path).read_all();
        if (auto contents = embedded_override(path)) return ensure_str(*contents);
        return std::nullopt;
    }

    auto profile_settings() -> std::optional<File::StringType>
    {
        if (const auto* resource = builtin_catalog().find(active_profile, "UE4SS-settings.ini")) return ensure_str(resource->contents);
        return std::nullopt;
    }
} // namespace RC::Compatibility
