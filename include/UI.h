// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 zhuabaobao123
//
// Race Passives (RacePassivesNG). This program is free software: you can
// redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version. It is distributed in
// the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See LICENSE.txt for the full text.

#pragma once
#include "SKSE-MCP/SKSEMenuFramework.h"

class UIRenderer
{
public:
    static UIRenderer& GetSingleton();
    static void Register();
    static void __stdcall RenderNordPage();
    static void __stdcall RenderOrcPage();
    static void __stdcall RenderBretonPage();
    static void __stdcall RenderDunmerPage();
    static void __stdcall RenderAltmerPage();
    static void __stdcall RenderKhajiitPage();
    static void __stdcall RenderArgonianPage();
    static void __stdcall RenderRedguardPage();
    static void __stdcall RenderBosmerPage();
    static void __stdcall RenderImperialPage();
    static void __stdcall RenderVampirePage();
    static void __stdcall RenderWerewolfPage();
    static void __stdcall RenderOverviewPage();
    static void __stdcall RenderCustomRacesPage();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer(UIRenderer&&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;
    UIRenderer& operator=(UIRenderer&&) = delete;

private:
    UIRenderer() = default;
    ~UIRenderer() = default;

    static UIRenderer Singleton;
};
