# Embedded compatibility profiles

The Wand fork compiles `assets/CustomGameConfigs/**/*.ini` and `**/*.lua` into
UE4SS.dll. Settings, member layouts, vtable layouts, and supported signature
scripts are read directly from memory. There is no extraction directory and no
per-game data to distribute alongside the DLL. Existing upstream consumers
still determine which assets are used; embedding a legacy script does not add
support for a signature type that upstream no longer consumes (for example,
`FText_Constructor.lua`).

Python 3 is required at build time. CMake regenerates the resource translation
unit when assets, the build manifest, or the generator change. Asset line endings
and UTF-8 BOMs are normalized. Runtime data stays UTF-8 until an INI is parsed.

## Selection

The default is `Auto`; no environment variable or INI change is required. UE4SS
recognizes executable basenames that match a complete embedded game profile name,
ignoring case, spaces, and punctuation, with an optional `-Win64-Shipping` suffix.
For example, `AbioticFactor-Win64-Shipping.exe` selects `Abiotic Factor` automatically.
Names must match completely; partial matches and ambiguous names are rejected.
Games with unrelated internal executable names are not yet recognized by this rule.
Unknown games retain stock discovery.

Exact executable/SHA-256 entries in `assets/CustomGameConfigs/validated-builds.json`
take priority. If an executable has such entries, an unknown build does not fall
back to name matching. Only these version-specific candidates require hashing.
The initial exact-build manifest is empty. Automatic name recognition selects the
bundled upstream tables; it does not establish that those tables work with every
game update. Abiotic Factor still needs an in-game test.

For troubleshooting only, `[Compatibility] Profile` in UE4SS-settings.ini or
`UE4SS_COMPATIBILITY_PROFILE` can select a specific profile. The environment wins;
`None` disables built-ins and `Auto` restores automatic selection. Ordinary users
do not need either override.

## External overrides

Embedded settings act as defaults. The existing external UE4SS-settings.ini is
applied afterward, so values explicitly present there win. A full stock INI can
therefore override a profile's settings: for deployment use only the settings
you intend to override, rather than copying every upstream default.

An external member/vtable layout or signature file in the working directory
replaces the corresponding embedded file. Layouts are replaced as whole files,
not merged. An empty external layout suppresses the embedded layout. Signature
scripts keep upstream's execution semantics; empty scripts are invalid.
The external legacy `FMemory_Free.lua` alias also takes precedence over an
embedded `GMalloc.lua`. Logs name the selected embedded profile and each
embedded signature that executes.

## Verification

These tests do not require Unreal or the restricted submodule:

```powershell
cmake -S tests/compatibility -B build/compatibility-tests
cmake --build build/compatibility-tests --config Release
ctest --test-dir build/compatibility-tests -C Release --output-on-failure
```

They check all compiled resources against their source files, case-insensitive
lookup, exact build matching, rejection of unknown builds, disabling profiles,
SHA-256 vectors, and malformed/ambiguous build manifests. Build UE4SS itself
using the main README instructions to verify integration.

Labs' runtime builder replaces UE4SS.dll inside a pinned base archive, preserving
WandPipe and the supporting files byte-for-byte. Build Lua as C++ to match that
plugin. The builder checks pinned interface headers before reusing the plugin and
regenerates the archive and release hashes. This preserves the existing external
settings, which still take precedence over embedded profile settings.

With the full dependency checkout, configure the main build with
`-DUE4SS_BUILD_COMPATIBILITY_TESTS=ON`, build the `compatibility_runtime_tests`
target, then run `build/Game__Shipping__Win64/bin/compatibility_runtime_tests.exe`.
This exercises the real INI parser and Lua runtime without injecting into a game,
and verifies that loading embedded resources creates no files.
