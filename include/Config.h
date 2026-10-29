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
