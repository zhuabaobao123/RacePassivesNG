# RacePassivesNG

Configurable racial passives for Skyrim Special Edition.

Every playable race gets 2-3 passive abilities, and each race has an intensity slider in an in-game menu (0-200%). Nothing vanilla is overwritten, so it sits happily next to other race mods.

Nexus page: https://www.nexusmods.com/skyrimspecialedition/mods/192233


## What's in the plugin

* 10 passives sets, one per playable race. Each set is a couple of magic effects plus, where it makes sense, a perk for conditional stuff (below half health, in combat, and so on).
* An SKSE Menu Framework page (F1) with one slider per race. 0 turns a race off, 100 is the default, 200 doubles the additive effects.
* Multiplicative effects (shout cooldown, attack damage, prices, skill learning) cap at 100%, because going past that inverts them.
* Settings live in `SKSE/Plugins/RacePassives.ini` and are re-applied on every load, so they carry across saves.
* Custom races can be mapped onto any of the ten sets via the `[CustomRaces]` section of that ini.
* Localisation: the menu strings come from a JSON next to the DLL (`RacePassives.json`); drop in a translated one, or delete it and you get the built-in English.

The plugin scales things at runtime by writing the effect magnitudes and perk values from a baseline it captures on load. The ESP is only data - no scripts, no quest, no SEQ.


## Requirements

Runtime (what users need):

* [SKSE64](https://skse.silverlock.org/)
* [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
* [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) - optional, only for the menu


## Building

You need:

* Visual Studio 2022/2026 with the C++ workload
* CMake 3.21+
* [vcpkg](https://github.com/microsoft/vcpkg)
* [CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG) (the alandtse NG fork) checked out locally

`CMakeLists.txt` points at CommonLibSSE-NG through `COMMONLIB_SSE_FOLDER`. Override it for your setup - and note that the path must not contain non-ASCII characters, or MSVC trips over the generated PCH wrapper.

```bat
set VCPKG_ROOT=C:\path\to\vcpkg

cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md ^
  -DCOMMONLIB_SSE_FOLDER=C:/path/to/CommonLibSSE-NG

cmake --build build --config Release
```

The DLL lands in `build/Release/`. SE and AE are both enabled by default (one DLL for both); pass `-DENABLE_SKYRIM_VR=ON` if you want a VR build too.

There are no Papyrus scripts to compile — no `.psc`, no CK compile step, nothing to deploy under `Scripts\`.


## Layout

```
include/           headers (PCH, Config, Localization, SyncScale, UI)
src/               sources (main, Config, Localization, SyncScale, UI)
include/SKSE-MCP/  vendored SKSE Menu Framework header (LGPL)
dist/en/           English ESP
dist/zh/           Chinese ESP + menu JSON
dist/              cover art and the store-page copy
```


## Configuration

`SKSE/Plugins/RacePassives.ini` holds the per-race intensity and the custom-race map:

```ini
[Nord]
Intensity=100

[CustomRaces]
MyCustomNord=Nord
```

Edit it by hand and hit "Reload config from ini" in the menu, or just drag the sliders. The plugin rewrites the file when a slider moves, and it keeps your `[CustomRaces]` entries when it does.


## Credits

* [CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG)
* [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) for the menu
