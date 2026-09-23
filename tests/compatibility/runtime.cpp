#include <Compatibility/Embedded.hpp>
#include <Compatibility/Layout.hpp>
#include <Helpers/String.hpp>
#include <IniParser/Ini.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>

#include <Windows.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

auto require(bool value, const char* message) -> void
{
    if (!value) throw std::runtime_error{message};
}

auto main() -> int
{
    using namespace RC;
    namespace compatibility = RC::Compatibility;
    const auto root = std::filesystem::current_path() / ("compatibility-smoke-" + std::to_string(GetCurrentProcessId()));
    try
    {
        require(std::filesystem::create_directory(root), "Scratch directory already exists");
        const auto settings = root / "UE4SS-settings.ini";
        const auto layout = root / "MemberVariableLayout.ini";
        const auto executable = root / "AbioticFactor-Win64-Shipping.exe";
        _putenv_s("UE4SS_COMPATIBILITY_PROFILE", "");
        compatibility::initialize(executable, root, settings);
        require(compatibility::selected_profile() == "Abiotic Factor", "Default automatic selection failed");
        auto contents = compatibility::read_override(layout);
        require(contents.has_value(), "Embedded layout unavailable");
        Ini::Parser parser;
        parser.parse(*contents);
        require(parser.get_int64(STR("UEnum"), STR("CppForm")) == 0x50, "Embedded INI did not parse");
        require(std::filesystem::is_empty(root), "Embedded lookup extracted files to disk");
        Ini::Parser vtables;
        auto vtable_contents = compatibility::read_override(root / "VTableLayout.ini");
        require(vtable_contents.has_value(), "Embedded vtable layout unavailable");
        vtables.parse(*vtable_contents);
        require(compatibility::property_uses_ffield(-1, -1, vtables.get_ordered_list(STR("FField")).size() > 0),
                "Abiotic property layout must use FField before engine detection");
        require(!compatibility::property_uses_ffield(-1, -1, false), "Legacy layout without FField must retain UObject inheritance");
        require(!compatibility::property_uses_ffield(4, 24, true), "Explicit legacy engine version must take precedence");
        require(compatibility::property_uses_ffield(4, 25, false), "UE4.25 must use FField inheritance");
        require(compatibility::property_uses_ffield(5, 4, false), "UE5 must use FField inheritance");
        require(!compatibility::embedded_override(root / ".." / "MemberVariableLayout.ini"), "Lookup escaped working directory");

        {
            std::ofstream file{layout};
            file << "[UEnum]\nCppForm = 0x99\n";
        }
        contents = compatibility::read_override(layout);
        Ini::Parser external;
        external.parse(*contents);
        require(external.get_int64(STR("UEnum"), STR("CppForm")) == 0x99, "External override did not win");
        {
            std::ofstream file{layout, std::ios::trunc};
        }
        require(compatibility::read_override(layout)->empty(), "Empty external layout did not suppress builtin");
        std::filesystem::remove(layout);

        _putenv_s("UE4SS_COMPATIBILITY_PROFILE", "The Quarry");
        compatibility::initialize(executable, root, settings);
        require(compatibility::profile_settings().has_value(), "Embedded settings unavailable");
        const auto signature = root / "UE4SS_Signatures/StaticConstructObject.lua";
        require(compatibility::has_override(signature), "Embedded signature not detected");
        const auto source = compatibility::embedded_override(signature);
        require(source.has_value(), "Embedded signature unavailable");
        const auto& lua = LuaMadeSimple::new_state();
        lua.execute_string(*source);
        lua.call_function("Register", 0, 1);
        require(lua.is_string() && lua.get_string().starts_with("48 8B C4"), "Embedded Lua did not execute");
        require(std::filesystem::is_empty(root), "Signature lookup extracted files to disk");

        _putenv_s("UE4SS_COMPATIBILITY_PROFILE", "");
        {
            std::ofstream file{settings};
            file << "[Compatibility]\nProfile = Abiotic Factor\n";
        }
        compatibility::initialize(executable, root, settings);
        require(compatibility::selected_profile() == "Abiotic Factor", "INI selection failed");
        _putenv_s("UE4SS_COMPATIBILITY_PROFILE", "None");
        compatibility::initialize(executable, root, settings);
        require(compatibility::selected_profile().empty(), "Environment did not override INI selection");
        require(!compatibility::read_override(layout), "Disabled profile still supplied layouts");
        std::filesystem::remove(settings);
        std::filesystem::remove(root);
        std::cout << "Verified memory-only INI/Lua loading, external precedence, and profile selection\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
