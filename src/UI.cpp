#include "UI.h"

#include "Config.h"
#include "SyncScale.h"

#include <cstdio>
#include <map>
#include <string>

UIRenderer UIRenderer::Singleton;

UIRenderer& UIRenderer::GetSingleton() { return Singleton; }

namespace
{
	std::map<std::string, bool> g_dirty;

	void RenderRacePage(const char* a_raceName, const char* a_globEDID, const char* a_desc)
	{
		auto* form = RE::TESForm::LookupByEditorID(a_globEDID);
		auto* glob = form ? form->As<RE::TESGlobal>() : nullptr;
		if (!glob) {
			ImGui::Text("%s", _T("GLOB not found (enable RacePassives.esp)"));
			return;
		}

		ImGui::TextWrapped("%s", _T(a_desc));
		ImGui::Spacing();

		int v = static_cast<int>(glob->value + 0.5f);
		char label[128];
		std::snprintf(label, sizeof(label), "##%s", a_globEDID);
		ImGui::SetNextItemWidth(340.0f);
		const bool edited = ImGui::SliderInt(label, &v, 0, 200, "%d%%");
		const bool active = ImGui::IsItemActive();
		if (v == 0) {
			ImGui::SameLine();
			ImGui::TextUnformatted(_T("OFF"));
		}

		bool& dirty = g_dirty[a_globEDID];
		if (edited) {
			SyncScale::SetIntensity(a_globEDID, v);
			dirty = true;
		}
		if (dirty && !active) {
			SyncScale::Commit(a_globEDID, v);
			dirty = false;
		}

		// Live computed values at the current slider position.
		ImGui::Separator();
		auto items = SyncScale::GetDisplay(a_raceName, v);
		for (const auto& item : items) {
			ImGui::BulletText("%s: %s", item.label.c_str(), item.value.c_str());
		}
	}

	void RenderRaceStatus(const char* a_name, const char* a_globEDID)
	{
		int v = SyncScale::GetIntensity(a_globEDID);
		if (v == 0) {
			ImGui::Text("%s: %s", _T(a_name), _T("OFF"));
		} else {
			ImGui::Text("%s: %d%%", _T(a_name), v);
		}
	}
}

void UIRenderer::Register()
{
	if (!SKSEMenuFramework::IsInstalled()) {
		SKSE::log::warn("SKSEMenuFramework not installed, skipping menu page");
		return;
	}

	SKSEMenuFramework::SetSection(_T("RacePassives"));
	SKSEMenuFramework::AddSectionItem(_T("Overview"), RenderOverviewPage);
	SKSEMenuFramework::AddSectionItem(_T("Custom Races"), RenderCustomRacesPage);
	SKSEMenuFramework::AddSectionItem(_T("Nord"), RenderNordPage);
	SKSEMenuFramework::AddSectionItem(_T("Orc"), RenderOrcPage);
	SKSEMenuFramework::AddSectionItem(_T("Breton"), RenderBretonPage);
	SKSEMenuFramework::AddSectionItem(_T("Dunmer"), RenderDunmerPage);
	SKSEMenuFramework::AddSectionItem(_T("Altmer"), RenderAltmerPage);
	SKSEMenuFramework::AddSectionItem(_T("Khajiit"), RenderKhajiitPage);
	SKSEMenuFramework::AddSectionItem(_T("Argonian"), RenderArgonianPage);
	SKSEMenuFramework::AddSectionItem(_T("Redguard"), RenderRedguardPage);
	SKSEMenuFramework::AddSectionItem(_T("Bosmer"), RenderBosmerPage);
	SKSEMenuFramework::AddSectionItem(_T("Imperial"), RenderImperialPage);
	SKSE::log::info("Registered SMF pages");
}

void __stdcall UIRenderer::RenderOverviewPage()
{
	if (ImGui::Button(_T("Reload config from ini"))) {
		Config::Load();
		SyncScale::ReloadCustomRaces();
		SyncScale::ApplyAll();
	}
	ImGui::SameLine();
	if (ImGui::Button(_T("Disable all (before uninstalling)"))) {
		SyncScale::DisableAll();
	}
	ImGui::Separator();
	ImGui::TextWrapped("%s", _T("Drag a race slider to scale that race's passives (0 = off, 100 = default, 200 = double)."));
	ImGui::TextWrapped("%s", _T("Additive = flat bonus; multiplicative = a multiplier (e.g. 0.8 = -20%). Values scale with the intensity setting."));
	ImGui::Spacing();
	RenderRaceStatus("Nord", "RPEnableNordFrost");
	RenderRaceStatus("Orc", "RPEnableOrcRage");
	RenderRaceStatus("Breton", "RPEnableBreton");
	RenderRaceStatus("Dunmer", "RPEnableDunmer");
	RenderRaceStatus("Altmer", "RPEnableAltmer");
	RenderRaceStatus("Khajiit", "RPEnableKhajiit");
	RenderRaceStatus("Argonian", "RPEnableArgonian");
	RenderRaceStatus("Redguard", "RPEnableRedguard");
	RenderRaceStatus("Bosmer", "RPEnableBosmer");
	RenderRaceStatus("Imperial", "RPEnableImperial");
}

void __stdcall UIRenderer::RenderCustomRacesPage()
{
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		if (auto* race = player->GetRace()) {
			const char* eid = race->GetFormEditorID();
			ImGui::Text("%s: %s", _T("Current race EditorID"), (eid && *eid) ? eid : "?");
		}
	}
	ImGui::Separator();
	ImGui::TextWrapped("%s", _T("To give a custom race these passives, add a line under [CustomRaces] in SKSE/Plugins/RacePassives.ini, then hit \"Reload config from ini\" on the Overview page."));
	ImGui::Spacing();
	ImGui::TextUnformatted(_T("Example:"));
	ImGui::TextUnformatted("[CustomRaces]");
	ImGui::TextUnformatted("MyCustomNord=Nord");
	ImGui::Spacing();
	ImGui::TextWrapped("%s", _T("Passive sets: Nord, Orc, Breton, Dunmer, Altmer, Khajiit, Argonian, Redguard, Bosmer, Imperial."));
	ImGui::Separator();
	const auto& pairs = SyncScale::GetCustomRacePairs();
	if (pairs.empty()) {
		ImGui::TextUnformatted(_T("(no custom races loaded)"));
	} else {
		for (const auto& [editorid, setname] : pairs) {
			ImGui::BulletText("%s -> %s", editorid.c_str(), setname.c_str());
		}
	}
}

void __stdcall UIRenderer::RenderNordPage()
{
	RenderRacePage("Nord", "RPEnableNordFrost", "Nord: frost resist (additive), shout cooldown (multiplicative, fixed), health regen below half (additive). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderOrcPage()
{
	RenderRacePage("Orc", "RPEnableOrcRage", "Orc: health (additive), attack damage below half (multiplicative). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderBretonPage()
{
	RenderRacePage("Breton", "RPEnableBreton", "Breton: magic resist (additive), spell absorb (additive), magicka regen below half (additive). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderDunmerPage()
{
	RenderRacePage("Dunmer", "RPEnableDunmer", "Dunmer: fire resist (additive), attack damage below half (multiplicative). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderAltmerPage()
{
	RenderRacePage("Altmer", "RPEnableAltmer", "Altmer: magicka (additive), magicka regen (additive). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderKhajiitPage()
{
	RenderRacePage("Khajiit", "RPEnableKhajiit", "Khajiit: unarmed damage (additive), movement speed (additive), stamina regen (additive). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderArgonianPage()
{
	RenderRacePage("Argonian", "RPEnableArgonian", "Argonian: disease resist (additive), poison resist (additive), water health regen (additive). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderRedguardPage()
{
	RenderRacePage("Redguard", "RPEnableRedguard", "Redguard: stamina (additive), stamina regen (additive), poison resist (additive). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderBosmerPage()
{
	RenderRacePage("Bosmer", "RPEnableBosmer", "Bosmer: bow draw speed (additive), disease resist (additive), poison resist (additive). Values scale with the intensity setting.");
}

void __stdcall UIRenderer::RenderImperialPage()
{
	RenderRacePage("Imperial", "RPEnableImperial", "Imperial: buy/sell prices (multiplicative), skill learning (multiplicative), combat armor (additive). Values scale with the intensity setting.");
}
