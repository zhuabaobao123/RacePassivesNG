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
