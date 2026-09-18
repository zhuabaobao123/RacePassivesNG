#pragma once

#include <string>
#include <utility>
#include <vector>

// Cross-save config: SKSE/Plugins/RacePassives.ini (ini wins).
// - Intensity 0 = off, 1-200 = strength percent (100 = default ESP values).
// - Load() runs once at kDataLoaded: reads ini, writes every GLOB.
// - Save() runs on menu change: writes ini (GLOB itself is written by the UI task).
// - Missing file/key defaults to 100. Legacy Enable=0/1 honored (1 -> 100).
namespace Config
{
	void Load();
	void Save(const char* a_globEDID, int a_intensity);

	// [CustomRaces] section: customRaceEditorID = passive set name
	// (Nord / Orc / Breton / Dunmer / Altmer / Khajiit / Argonian / Redguard / Bosmer / Imperial).
	std::vector<std::pair<std::string, std::string>> GetCustomRaces();
}
