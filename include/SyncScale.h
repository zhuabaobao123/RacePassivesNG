#pragma once

#include <string>
#include <utility>
#include <vector>

// Strength scaling for racial passives.
// Intensity GLOB: 0 = off, 1-200 = percent (100 = the ESP-authored value).
// SPEL effect magnitudes and Perk entry-point values are rescaled in memory
// from a baseline captured once at kDataLoaded; nothing is written to the ESP.
namespace SyncScale
{
	struct DisplayValue
	{
		std::string label;
		std::string value;
	};

	void InitBaselines();
	void ApplyAll();
	int  GetIntensity(const char* a_globEDID);
	void SetIntensity(const char* a_globEDID, int a_value);
	void Commit(const char* a_globEDID, int a_value);

	// Sets every race to 0, writes the ini, and removes all of our spells/perks from
	// the player -- the clean-up step before uninstalling the mod mid-save.
	void DisableAll();

	// Re-reads the [CustomRaces] section and resolves the editor IDs to races.
	// Call after the ini changes (load / "Reload config from ini").
	void ReloadCustomRaces();

	// The mappings currently loaded, as {editorID, set name} (for the menu).
	const std::vector<std::pair<std::string, std::string>>& GetCustomRacePairs();

	// Computed effect values at the given intensity (for the menu), e.g.
	// {"寒霜抗性", "+37.5"}, {"龙吼冷却", "-20%"}.
	std::vector<DisplayValue> GetDisplay(const char* a_raceName, int a_intensity);
}
