#include <Compatibility/Catalog.hpp>
#include <Compatibility/Fingerprint.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace RC::Compatibility;

auto require(bool value, const char* message) -> void
{
    if (!value) throw std::runtime_error{message};
}

auto main(int argc, char** argv) -> int
{
    try
    {
        require(argc == 2, "Expected asset root");
        const auto catalog = builtin_catalog();
        require(!catalog.resources.empty(), "No embedded resources");
        const auto profile = catalog.select("abiotic factor", {}, {});
        require(profile == "Abiotic Factor", "Explicit profile lookup failed");
        require(catalog.find(profile, "membervariablelayout.ini"), "Missing member layout");
        require(catalog.find(profile, "VTableLayout.ini"), "Missing vtable layout");
        require(!catalog.find(profile, "UE4SS-settings.ini"), "Missing resources must not be invented");
        require(!catalog.find(profile, "../Atomic Heart/MemberVariableLayout.ini"), "Profile isolation failed");
        require(catalog.select("None", {}, {}).empty(), "Disable failed");
        require(catalog.select("Auto", "Unknown.exe", "unknown").empty(), "Unknown game matched");
        require(catalog.select("Auto", "AbioticFactor-Win64-Shipping.exe", {}) == profile, "Automatic shipping executable selection failed");
        require(catalog.select("", "abioticfactor.exe", {}) == profile, "Automatic plain executable selection failed");
        require(catalog.select("Auto", "AtomicHeart-Win64-Shipping.exe", {}) == "Atomic Heart", "Second game was not automatically recognized");
        require(catalog.select("Auto", "NotAbioticFactor-Win64-Shipping.exe", {}).empty(), "Partial game name matched");
        require(catalog.select("Auto", "AbioticFactor.dll", {}).empty(), "Non-executable matched");
        require(catalog.select("None", "AbioticFactor-Win64-Shipping.exe", {}).empty(), "Disable must override automatic recognition");
        bool rejected = false;
        try
        {
            catalog.select("Typo", {}, {});
        }
        catch (const std::runtime_error&)
        {
            rejected = true;
        }
        require(rejected, "Unknown explicit profile was silently ignored");

        constexpr std::string_view digest = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
        const std::array builds{Build{"AbioticFactor-Win64-Shipping.exe", digest, "Abiotic Factor"}};
        const Catalog validated{catalog.resources, builds};
        require(validated.needs_fingerprint("abioticfactor-win64-shipping.exe"), "Executable comparison should ignore case");
        require(!validated.needs_fingerprint("Other.exe"), "Unrelated executable should not need hashing");
        require(validated.select("Auto", "ABIOTICFACTOR-WIN64-SHIPPING.EXE", digest) == profile, "Known build did not match");
        require(validated.select("Auto", "AbioticFactor-Win64-Shipping.exe", "new-build").empty(), "New build used stale offsets");
        require(validated.select("Auto", "Other.exe", digest).empty(), "Executable identity was ignored");
        require(validated.select("Auto", "AbioticFactor-Win64-Shipping.exe", {}).empty(), "Missing hash matched");
        require(validated.select("None", "AbioticFactor-Win64-Shipping.exe", digest).empty(), "Disable must win over validated build");

        size_t bytes = 0;
        for (const auto& resource : catalog.resources)
        {
            std::ifstream file{std::filesystem::path{argv[1]} / resource.profile / resource.path, std::ios::binary};
            require(static_cast<bool>(file), "Source asset missing");
            std::string source{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
            if (source.starts_with("\xef\xbb\xbf")) source.erase(0, 3);
            for (size_t index = 0; (index = source.find("\r\n", index)) != std::string::npos;)
                source.erase(index, 1);
            require(resource.contents == source, "Compiled resource differs from source");
            require(resource.contents.data()[resource.contents.size()] == '\0', "Lua source must be NUL terminated");
            bytes += resource.contents.size();
        }

        const auto sample = std::filesystem::current_path() / "fingerprint-test.bin";
        {
            std::ofstream output{sample, std::ios::binary};
            output << "abc";
        }
        require(executable_sha256(sample) == digest, "SHA-256 does not match known vector");
        {
            std::ofstream output{sample, std::ios::binary | std::ios::trunc};
        }
        require(executable_sha256(sample) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "Empty-file SHA-256 failed");
        std::filesystem::remove(sample);
        rejected = false;
        try
        {
            executable_sha256(sample);
        }
        catch (const std::runtime_error&)
        {
            rejected = true;
        }
        require(rejected, "Unreadable executable must not produce a hash");
        std::cout << "Verified " << catalog.resources.size() << " embedded resources (" << bytes << " bytes), selection and fingerprints\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
