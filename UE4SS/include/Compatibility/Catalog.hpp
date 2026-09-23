#pragma once

#include <span>
#include <string_view>

namespace RC::Compatibility
{
    struct Resource
    {
        std::string_view profile;
        std::string_view path;
        std::string_view contents;
    };

    struct Build
    {
        std::string_view executable;
        std::string_view sha256;
        std::string_view profile;
    };

    class Catalog
    {
      public:
        std::span<const Resource> resources;
        std::span<const Build> builds;

        auto needs_fingerprint(std::string_view executable) const -> bool;
        // Exact build mappings take priority; otherwise recognize full game names in executable basenames.
        auto select(std::string_view requested, std::string_view executable, std::string_view sha256) const -> std::string_view;
        auto find(std::string_view profile, std::string_view path) const -> const Resource*;
    };

    auto builtin_catalog() -> Catalog;
} // namespace RC::Compatibility
