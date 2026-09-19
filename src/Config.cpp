#include "Config.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace
{
	// section -> GLOB EditorID. Section is the name shown in SMF: a race, or a state (Vampire / Werewolf).
	constexpr std::pair<const char*, const char*> kEntries[] = {
		{ "Nord", "RPEnableNordFrost" },
		{ "Orc", "RPEnableOrcRage" },
		{ "Breton", "RPEnableBreton" },
		{ "Dunmer", "RPEnableDunmer" },
		{ "Altmer", "RPEnableAltmer" },
		{ "Khajiit", "RPEnableKhajiit" },
		{ "Argonian", "RPEnableArgonian" },
		{ "Redguard", "RPEnableRedguard" },
		{ "Bosmer", "RPEnableBosmer" },
		{ "Imperial", "RPEnableImperial" },
		{ "Vampire", "RPEnableVampire" },
		{ "Werewolf", "RPEnableWerewolf" },
	};

	constexpr int kDefaultIntensity = 100;
	constexpr int kMinIntensity = 0;
	constexpr int kMaxIntensity = 200;

	std::filesystem::path IniPath()
	{
		REX::W32::HMODULE dllHandle = REX::W32::GetModuleHandleW(L"RacePassives.dll");
		if (!dllHandle) {
			return {};
		}
		wchar_t dllPath[MAX_PATH];
		if (REX::W32::GetModuleFileNameW(dllHandle, dllPath, MAX_PATH) == 0) {
			return {};
		}
		return std::filesystem::path(dllPath).replace_extension(L".ini");
	}

	std::string Trim(const std::string& s)
	{
		std::size_t b = s.find_first_not_of(" \t\r\n");
		if (b == std::string::npos) {
			return {};
		}
		std::size_t e = s.find_last_not_of(" \t\r\n");
		return s.substr(b, e - b + 1);
	}

	int ClampIntensity(int v)
	{
		if (v < kMinIntensity) return kMinIntensity;
		if (v > kMaxIntensity) return kMaxIntensity;
		return v;
	}

	// section -> intensity 0-200. Absent defaults to 100.
	// Legacy "Enable=0/1" is honored when "Intensity" is absent (1 -> 100, 0 -> 0).
	std::map<std::string, int> ReadAll()
	{
		std::map<std::string, int> out;
		for (auto& [section, glob] : kEntries) {
			(void)glob;
			out[section] = kDefaultIntensity;
		}
		auto path = IniPath();
		if (path.empty()) {
			return out;
		}
		std::ifstream file(path);
		if (!file.is_open()) {
			return out;
		}
		std::map<std::string, std::string> raw;
		std::string line, section;
		while (std::getline(file, line)) {
			line = Trim(line);
			if (line.empty() || line[0] == ';' || line[0] == '#') {
				continue;
			}
			if (line.front() == '[' && line.back() == ']') {
				section = Trim(line.substr(1, line.size() - 2));
				continue;
			}
			auto eq = line.find('=');
			if (eq == std::string::npos || section.empty()) {
				continue;
			}
			std::string key = Trim(line.substr(0, eq));
			std::string val = Trim(line.substr(eq + 1));
			raw[section + "." + key] = val;
		}
		for (auto& [section, glob] : kEntries) {
			(void)glob;
			auto it = raw.find(std::string(section) + ".Intensity");
			if (it != raw.end()) {
				try {
					out[section] = ClampIntensity(std::stoi(it->second));
				} catch (...) {
					out[section] = kDefaultIntensity;
				}
				continue;
			}
			auto itOld = raw.find(std::string(section) + ".Enable");
			if (itOld != raw.end()) {
				out[section] = (Trim(itOld->second) != "0") ? 100 : 0;
			}
		}
		return out;
	}

	void WriteAll(const std::map<std::string, int>& values)
	{
		auto path = IniPath();
		if (path.empty()) {
			return;
		}
		// Read the user's custom-race mappings BEFORE truncating, so the rewrite keeps them.
		const auto customRaces = Config::GetCustomRaces();

		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file.is_open()) {
			SKSE::log::warn("Could not write config: {}", path.string());
			return;
		}
		file << "; RacePassives cross-save config (CRLF, UTF-8 no BOM)\r\n";
		file << "; 0 = off, 1-200 = intensity percent (100 = default)\r\n";
		for (auto& [section, glob] : kEntries) {
			(void)glob;
			auto it = values.find(section);
			int intensity = (it != values.end()) ? ClampIntensity(it->second) : kDefaultIntensity;
			file << "\r\n[" << section << "]\r\n";
			file << "Intensity=" << intensity << "\r\n";
		}
		file << "\r\n[CustomRaces]\r\n";
		file << "; Custom race EditorID = passive set to apply, e.g. MyCustomNord=Nord\r\n";
		file << "; Sets: Nord, Orc, Breton, Dunmer, Altmer, Khajiit, Argonian, Redguard, Bosmer, Imperial\r\n";
		for (const auto& [editorid, setname] : customRaces) {
			file << editorid << "=" << setname << "\r\n";
		}
		file.flush();
		SKSE::log::info("Wrote config: {}", path.string());
	}
}

namespace Config
{
	void Load()
	{
		auto values = ReadAll();
		auto* task = SKSE::GetTaskInterface();
		if (!task) {
			SKSE::log::error("no task interface for config apply");
			return;
		}
		task->AddTask([values = std::move(values)]() {
			int applied = 0;
			for (auto& [section, globEDID] : kEntries) {
				(void)section;
				auto it = values.find(section);
				int intensity = (it != values.end()) ? ClampIntensity(it->second) : kDefaultIntensity;
				auto* form = RE::TESForm::LookupByEditorID(globEDID);
				auto* glob = form ? form->As<RE::TESGlobal>() : nullptr;
				if (!glob) {
					continue;
				}
				glob->value = static_cast<float>(intensity);
				++applied;
			}
			SKSE::log::info("Applied cross-save config to {} GLOBs", applied);
		});
	}

	void Save(const char* a_globEDID, int a_intensity)
	{
		std::string target(a_globEDID ? a_globEDID : "");
		if (target.empty()) {
			return;
		}
		auto values = ReadAll();
		for (auto& [section, glob] : kEntries) {
			if (target == glob) {
				values[section] = ClampIntensity(a_intensity);
				break;
			}
		}
		WriteAll(values);
	}

	std::vector<std::pair<std::string, std::string>> GetCustomRaces()
	{
		std::vector<std::pair<std::string, std::string>> out;
		auto path = IniPath();
		if (path.empty()) {
			return out;
		}
		std::ifstream file(path);
		if (!file.is_open()) {
			return out;
		}
		std::string line, section;
		while (std::getline(file, line)) {
			line = Trim(line);
			if (line.empty() || line[0] == ';' || line[0] == '#') {
				continue;
			}
			if (line.front() == '[' && line.back() == ']') {
				section = Trim(line.substr(1, line.size() - 2));
				continue;
			}
			if (section != "CustomRaces") {
				continue;
			}
			auto eq = line.find('=');
			if (eq == std::string::npos) {
				continue;
			}
			std::string key = Trim(line.substr(0, eq));
			std::string val = Trim(line.substr(eq + 1));
			if (!key.empty() && !val.empty()) {
				out.emplace_back(key, val);
			}
		}
		return out;
	}
}
