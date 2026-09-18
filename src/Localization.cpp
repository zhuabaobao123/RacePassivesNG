#include "Localization.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace Localization
{
	static std::mutex                                     s_mutex;
	static bool                                           s_loaded = false;
	static bool                                           s_failed = false;
	static std::unordered_map<std::string, std::string>   s_translations;
	static std::map<std::string, std::string>             s_cache;

	void Load()
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		if (s_loaded || s_failed) {
			return;
		}

		// <DLL>.dll -> <DLL>.json in the same folder
		REX::W32::HMODULE dllHandle = REX::W32::GetModuleHandleW(L"RacePassives.dll");
		if (!dllHandle) {
			SKSE::log::warn("Could not get DLL handle for translations");
			s_failed = true;
			return;
		}

		wchar_t dllPath[MAX_PATH];
		if (REX::W32::GetModuleFileNameW(dllHandle, dllPath, MAX_PATH) == 0) {
			SKSE::log::warn("Could not get DLL path for translations");
			s_failed = true;
			return;
		}

		std::filesystem::path jsonPath = std::filesystem::path(dllPath).replace_extension(L".json");

		if (!std::filesystem::exists(jsonPath)) {
			SKSE::log::info("Translation file not found (using built-in English): {}", jsonPath.string());
			s_failed = true;
			return;
		}

		std::ifstream file(jsonPath, std::ios::binary);
		if (!file.is_open()) {
			SKSE::log::warn("Could not open translation file: {}", jsonPath.string());
			s_failed = true;
			return;
		}

		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		// Strip UTF-8 BOM if present
		const char* jsonData = content.c_str();
		if (content.size() >= 3 && static_cast<unsigned char>(jsonData[0]) == 0xEF &&
			static_cast<unsigned char>(jsonData[1]) == 0xBB && static_cast<unsigned char>(jsonData[2]) == 0xBF) {
			jsonData += 3;
		}

		auto doc = nlohmann::json::parse(jsonData, nullptr, false);
		if (doc.is_discarded() || !doc.is_object()) {
			SKSE::log::error("JSON parse error in: {}", jsonPath.string());
			s_failed = true;
			return;
		}

		for (auto& [k, v] : doc.items()) {
			if (v.is_string()) {
				s_translations[k] = v.get<std::string>();
			}
		}

		s_loaded = true;
		SKSE::log::info("Loaded {} translations from {}", s_translations.size(), jsonPath.string());
	}

	const char* Translate(const char* a_key)
	{
		Load();

		std::lock_guard<std::mutex> lock(s_mutex);

		std::string key(a_key);

		auto it = s_cache.find(key);
		if (it != s_cache.end()) {
			return it->second.c_str();
		}

		auto transIt = s_translations.find(key);
		if (transIt != s_translations.end()) {
			auto result = s_cache.emplace(key, transIt->second);
			return result.first->second.c_str();
		}

		// Not found: cache and return the original key (built-in English)
		auto result = s_cache.emplace(key, key);
		return result.first->second.c_str();
	}
}
