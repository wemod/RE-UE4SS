#include <Compatibility/Catalog.hpp>

#include <stdexcept>
#include <string>

namespace RC::Compatibility
{
    namespace
    {
        auto game_name(std::string_view name) -> std::string
        {
            std::string result;
            for (const char value : name)
            {
                if (value >= 'A' && value <= 'Z')
                    result += static_cast<char>(value + ('a' - 'A'));
                else if ((value >= 'a' && value <= 'z') || (value >= '0' && value <= '9'))
                    result += value;
            }
            return result;
        }

        auto equal_ascii(std::string_view left, std::string_view right) -> bool
        {
            if (left.size() != right.size()) return false;
            auto lower = [](char value) {
                return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
            };
            for (size_t index = 0; index < left.size(); ++index)
            {
                if (lower(left[index]) != lower(right[index])) return false;
            }
            return true;
        }
    } // namespace

    auto Catalog::needs_fingerprint(std::string_view executable) const -> bool
    {
        for (const auto& build : builds)
        {
            if (equal_ascii(build.executable, executable)) return true;
        }
        return false;
    }

    auto Catalog::select(std::string_view requested, std::string_view executable, std::string_view sha256) const -> std::string_view
    {
        if (equal_ascii(requested, "None")) return {};
        if (requested.empty() || equal_ascii(requested, "Auto"))
        {
            bool pinned_executable = false;
            for (const auto& build : builds)
            {
                if (!equal_ascii(build.executable, executable)) continue;
                pinned_executable = true;
                if (!sha256.empty() && equal_ascii(build.sha256, sha256)) return build.profile;
            }
            // Version-specific layouts must not fall back to a different build's tables.
            if (pinned_executable) return {};
            if (executable.size() < 4 || !equal_ascii(executable.substr(executable.size() - 4), ".exe")) return {};
            executable.remove_suffix(4);
            constexpr std::string_view shipping = "-Win64-Shipping";
            if (executable.size() >= shipping.size() && equal_ascii(executable.substr(executable.size() - shipping.size()), shipping))
                executable.remove_suffix(shipping.size());
            const auto identity = game_name(executable);
            if (identity.empty()) return {};
            std::string_view selected;
            for (const auto& resource : resources)
            {
                if (game_name(resource.profile) != identity) continue;
                if (!selected.empty() && selected != resource.profile) return {};
                selected = resource.profile;
            }
            return selected;
        }
        for (const auto& resource : resources)
        {
            if (equal_ascii(resource.profile, requested)) return resource.profile;
        }
        throw std::runtime_error{"Unknown embedded compatibility profile"};
    }

    auto Catalog::find(std::string_view profile, std::string_view path) const -> const Resource*
    {
        if (profile.empty()) return nullptr;
        for (const auto& resource : resources)
        {
            if (equal_ascii(resource.profile, profile) && equal_ascii(resource.path, path)) return &resource;
        }
        return nullptr;
    }
} // namespace RC::Compatibility
