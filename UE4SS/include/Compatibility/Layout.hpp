#pragma once

namespace RC::Compatibility
{
    // Layout overrides are loaded before the scanner establishes the engine version.
    constexpr auto property_uses_ffield(int major, int minor, bool has_ffield_layout) -> bool
    {
        if (major < 0 || minor < 0) return has_ffield_layout;
        return major > 4 || (major == 4 && minor >= 25);
    }
} // namespace RC::Compatibility
