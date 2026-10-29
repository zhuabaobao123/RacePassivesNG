Race Passives (RacePassivesNG) 2.1
Copyright (C) 2026 zhuabaobao123

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.

The full licence text is LICENSE.txt in this archive - the GNU GPL v3 text as
shipped with CommonLibSSE-NG, unmodified - and online at
<https://www.gnu.org/licenses/gpl-3.0.txt>.


Source code
-----------
https://github.com/zhuabaobao123/RacePassivesNG

Everything needed to build RacePassives.dll is there: CMakeLists.txt, vcpkg.json,
src/, include/, and records/ (the build specs for the plugin's records).


Why this mod is GPL-3.0
-----------------------
It links CommonLibSSE-NG <https://github.com/alandtse/CommonLibSSE-NG>, which is
GPL-3.0-or-later. CommonLibSSE-NG's additional permissions are reproduced in
EXCEPTIONS.md in this archive.


Third-party components
----------------------
- CommonLibSSE-NG - GPL-3.0-or-later. Linking it is what puts this mod under the
  GPL. It is not redistributed here; the build fetches it.
- SKSE Menu Framework - its menu API is vendored as a single header,
  include/SKSE-MCP/SKSEMenuFramework.h, unmodified. See its mod page for its own
  terms: https://www.nexusmods.com/skyrimspecialedition/mods/120352
- Dear ImGui - MIT, bundled inside SKSE Menu Framework. This mod does not link
  or redistribute it separately. https://github.com/ocornut/imgui
- The ESP and the DLL contain no third-party assets.
