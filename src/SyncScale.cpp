#include "SyncScale.h"

#include "Config.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace
{
	using Func = RE::BGSEntryPointFunction::ENTRY_POINT_FUNCTION;
	using FuncData = RE::BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA;

	// How an entry decides whether it applies to the player.
	//   kRace    - the player's race, or the race its MorphRace points back at (a vampire variant),
	//              or a race mapped in [CustomRaces].
	//   kVampire - the player is a vampire (the Vampire keyword), in human form.
	//   kWerewolf- the player can turn into a werewolf (has the Beast Form power), in human form.
	enum class Match
	{
		kRace,
		kVampire,
		kWerewolf,
	};

	struct Entry
	{
		const char*              name;
		const char*              globEDID;
		std::uint32_t            raceFormID;
		const char*              spellEDID;
		std::vector<const char*> perkEDIDs;
		Match                    match = Match::kRace;
		const char*              excludeRaceEDID = nullptr;
	};

	const std::vector<Entry>& Table()
	{
		static const std::vector<Entry> t = {
			{ "Nord", "RPEnableNordFrost", 0x013746, "RPNordFrostAffinity", {} },
			{ "Orc", "RPEnableOrcRage", 0x013747, "RPOrcRage", { "RPOrcBerserkRage" } },
			{ "Breton", "RPEnableBreton", 0x013741, "RPBretonAncestry", {} },
			{ "Dunmer", "RPEnableDunmer", 0x013742, "RPDunmerAncestry", { "RPDunmerWrath" } },
			{ "Altmer", "RPEnableAltmer", 0x013743, "RPAltmerAncestry", {} },
			{ "Khajiit", "RPEnableKhajiit", 0x013745, "RPKhajiitAgility", {} },
			{ "Argonian", "RPEnableArgonian", 0x013740, "RPArgonianBlood", {} },
			{ "Redguard", "RPEnableRedguard", 0x013748, "RPRedguardVigor", {} },
			{ "Bosmer", "RPEnableBosmer", 0x013749, "RPBosmerHunter", {} },
			{ "Imperial", "RPEnableImperial", 0x013744, "RPImperialVirtue", { "RPImperialPrices", "RPImperialLearning", "RPImperialDiscipline" } },
		};
		return t;
	}

	// Skyrim.esm FormIDs (Skyrim.esm is always load index 0, so the runtime ID is the local one).
	constexpr std::uint32_t kVampireKeyword   = 0x000A82BB;  // KYWD "Vampire"
	constexpr std::uint32_t kWerewolfChange   = 0x00092C48;  // SPEL "WerewolfChange" (LesserPower - the Beast Form power)
	constexpr std::uint32_t kWerewolfImmunity = 0x000F5BA0;  // SPEL "WerewolfImmunity" (Ability - lycanthropy disease immunity)

	// A form the player takes, not a race: the state passives are for the human form only.
	// By editor ID because one of them lives in Dawnguard.esm, whose load index is not fixed.
	constexpr const char* kWerewolfBeastRaceEDID = "WerewolfBeastRace";
	constexpr const char* kVampireLordRaceEDID   = "DLC1VampireBeastRace";

	// Passives that come from what the player *is* rather than which race they picked, so they add
	// to a race's set instead of replacing it.
	const std::vector<Entry>& StateTable()
	{
		static const std::vector<Entry> t = {
			{ "Vampire", "RPEnableVampire", 0, "RPVampireBlood", {}, Match::kVampire, kVampireLordRaceEDID },
			{ "Werewolf", "RPEnableWerewolf", 0, "RPWerewolfBlood", { "RPWerewolfFury" }, Match::kWerewolf, kWerewolfBeastRaceEDID },
		};
		return t;
	}

	std::map<std::string, std::vector<float>> g_spellBase;
	std::map<std::string, std::vector<float>> g_perkBase;
	std::map<std::string, int>                g_lastApplied;
	std::map<RE::FormID, std::string>         g_mgefBaseName;
	std::map<std::string, std::vector<RE::FormID>> g_customRaceIds;  // lowercase set name -> race formIDs
	std::vector<std::pair<std::string, std::string>> g_customRacePairs;

	std::string ToLower(std::string a_s)
	{
		for (auto& c : a_s) {
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		return a_s;
	}

	bool IsCustomRaceOf(RE::FormID a_race, const char* a_setName)
	{
		auto it = g_customRaceIds.find(ToLower(a_setName));
		if (it == g_customRaceIds.end()) {
			return false;
		}
		return std::find(it->second.begin(), it->second.end(), a_race) != it->second.end();
	}

	// A vampire race (NordRaceVampire, and every mod's variant of it) points back at the race it
	// was made from through MorphRace. That is what lets a vampire keep its race's passives.
	RE::TESRace* EffectiveRace(RE::TESRace* a_race)
	{
		if (a_race && a_race->morphRace && a_race->morphRace != a_race) {
			return a_race->morphRace;
		}
		return a_race;
	}

	bool RaceMatches(const Entry& a_entry, RE::TESRace* a_race)
	{
		if (!a_race) {
			return false;
		}
		auto* effective = EffectiveRace(a_race);
		return effective->formID == a_entry.raceFormID ||
		       IsCustomRaceOf(effective->formID, a_entry.name) ||
		       IsCustomRaceOf(a_race->formID, a_entry.name);
	}

	// True while the player wears a form (Vampire Lord, Beast Form) rather than a race.
	bool InExcludedRace(RE::TESRace* a_race, const char* a_excludeEDID)
	{
		if (!a_race || !a_excludeEDID) {
			return false;
		}
		auto* form = RE::TESForm::LookupByEditorID(a_excludeEDID);
		auto* race = form ? form->As<RE::TESRace>() : nullptr;
		return race && race->formID == a_race->formID;
	}

	bool StateMatches(const Entry& a_entry, RE::PlayerCharacter* a_player, RE::TESRace* a_race)
	{
		if (InExcludedRace(a_race, a_entry.excludeRaceEDID)) {
			return false;
		}
		switch (a_entry.match) {
		case Match::kVampire:
			{
				auto* keyword = RE::TESForm::LookupByID<RE::BGSKeyword>(kVampireKeyword);
				if (!keyword) {
					return false;
				}
				// The keyword sits on the vampire race; some setups put it on the actor too.
				return (a_race && a_race->HasKeyword(keyword)) || a_player->HasKeyword(keyword);
			}
		case Match::kWerewolf:
			{
				// The Beast Form power stays in the spell list in human form. The immunity
				// ability is a second marker, for setups that hand out something else.
				for (auto formID : { kWerewolfChange, kWerewolfImmunity }) {
					auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
					if (spell && a_player->HasSpell(spell)) {
						return true;
					}
				}
				return false;
			}
		default:
			return false;
		}
	}

	bool EntryMatches(const Entry& a_entry, RE::PlayerCharacter* a_player)
	{
		if (!a_player) {
			return false;
		}
		auto* race = a_player->GetRace();
		if (a_entry.match == Match::kRace) {
			return RaceMatches(a_entry, race);
		}
		return StateMatches(a_entry, a_player, race);
	}

	constexpr int kDefaultIntensity = 100;
	constexpr int kMaxIntensity = 200;

	float ScaleValue(float a_base, float a_scale, bool a_multiply)
	{
		if (a_multiply) {
			// Multiplier values (e.g. shout cooldown 0.8 = -20%) must never exceed their
			// default: scaling past 100% would invert them (0.8 -> 0.6 -> 0.4 ... and
			// eventually negative), so cap the scale at 1.0.
			const float s = (a_scale > 1.0f) ? 1.0f : a_scale;
			return 1.0f + (a_base - 1.0f) * s;
		}
		return a_base * a_scale;
	}

	std::string FormatValue(float a_value, bool a_multiply)
	{
		char buf[64];
		if (a_multiply) {
			std::snprintf(buf, sizeof(buf), "%+.0f%%", (a_value - 1.0f) * 100.0f);
		} else {
			std::snprintf(buf, sizeof(buf), "+%.1f", a_value);
		}
		return std::string(buf);
	}

	// ShoutRecoveryMult magnitude is a multiplier (0.8 = 80% cooldown), not an additive bonus.
	bool IsMultiplierSpellEffect(const std::string& a_spellEDID, std::size_t a_index)
	{
		return a_spellEDID == "RPNordFrostAffinity" && a_index == 1;
	}

	bool IsMultiplierPerkFunction(Func a_func)
	{
		return a_func == Func::kMultiplyValue ||
		       a_func == Func::kMultiplyActorValueMult ||
		       a_func == Func::kMultiplyOnePlusActorValueMult;
	}

	void CaptureSpellBaseline(const Entry& a_entry)
	{
		auto* form = RE::TESForm::LookupByEditorID(a_entry.spellEDID);
		auto* spell = form ? form->As<RE::SpellItem>() : nullptr;
		if (!spell) {
			SKSE::log::warn("baseline: spell {} not found", a_entry.spellEDID);
			return;
		}
		auto& vec = g_spellBase[a_entry.spellEDID];
		vec.clear();
		for (auto* effect : spell->effects) {
			vec.push_back(effect ? effect->effectItem.magnitude : 0.0f);
			if (effect && effect->baseEffect) {
				g_mgefBaseName[effect->baseEffect->formID] = effect->baseEffect->fullName.c_str();
			}
		}
	}

	void CapturePerkBaseline(const char* a_perkEDID)
	{
		auto* form = RE::TESForm::LookupByEditorID(a_perkEDID);
		auto* perk = form ? form->As<RE::BGSPerk>() : nullptr;
		if (!perk) {
			SKSE::log::warn("baseline: perk {} not found", a_perkEDID);
			return;
		}
		auto& vec = g_perkBase[a_perkEDID];
		vec.clear();
		for (auto* pe : perk->perkEntries) {
			auto* ep = pe ? static_cast<RE::BGSEntryPointPerkEntry*>(pe) : nullptr;
			float v = 0.0f;
			if (ep && ep->functionData && ep->functionData->GetType() == FuncData::kOneValue) {
				v = static_cast<RE::BGSEntryPointFunctionDataOneValue*>(ep->functionData)->data;
			}
			vec.push_back(v);
		}
	}

	void ApplySpellScaling(const Entry& a_entry, float a_scale)
	{
		auto* form = RE::TESForm::LookupByEditorID(a_entry.spellEDID);
		auto* spell = form ? form->As<RE::SpellItem>() : nullptr;
		if (!spell) {
			return;
		}
		auto baseIt = g_spellBase.find(a_entry.spellEDID);
		if (baseIt == g_spellBase.end()) {
			return;
		}
		const auto& bases = baseIt->second;
		for (std::size_t i = 0; i < spell->effects.size() && i < bases.size(); ++i) {
			auto* effect = spell->effects[i];
			if (!effect) {
				continue;
			}
			const bool mult = IsMultiplierSpellEffect(a_entry.spellEDID, i);
			const float v = ScaleValue(bases[i], a_scale, mult);
			effect->effectItem.magnitude = v;

			// Show the live value in the Magic-menu Active Effects list (its rows are
			// the magic-effect names), e.g. "寒霜抗性 +37.5".
			if (auto* mgef = effect->baseEffect) {
				auto nameIt = g_mgefBaseName.find(mgef->formID);
				if (nameIt != g_mgefBaseName.end() && !nameIt->second.empty()) {
					std::string newName = nameIt->second;
					if (a_scale > 0.0f) {
						newName += " " + FormatValue(v, mult);
					}
					if (mgef->fullName.c_str() != newName) {
						mgef->fullName = newName;
					}
				}
			}
		}
	}

	void ApplyPerkScaling(const Entry& a_entry, float a_scale)
	{
		for (auto* perkEDID : a_entry.perkEDIDs) {
			auto* form = RE::TESForm::LookupByEditorID(perkEDID);
			auto* perk = form ? form->As<RE::BGSPerk>() : nullptr;
			if (!perk) {
				continue;
			}
			auto baseIt = g_perkBase.find(perkEDID);
			if (baseIt == g_perkBase.end()) {
				continue;
			}
			const auto& bases = baseIt->second;
			for (std::size_t i = 0; i < perk->perkEntries.size() && i < bases.size(); ++i) {
				auto* ep = perk->perkEntries[i] ? static_cast<RE::BGSEntryPointPerkEntry*>(perk->perkEntries[i]) : nullptr;
				if (!ep || !ep->functionData || ep->functionData->GetType() != FuncData::kOneValue) {
					continue;
				}
				auto* data = static_cast<RE::BGSEntryPointFunctionDataOneValue*>(ep->functionData);
				data->data = ScaleValue(bases[i], a_scale, IsMultiplierPerkFunction(ep->entryData.function.get()));
			}
		}
	}

	void SyncSpellPresence(const Entry& a_entry, RE::SpellItem* a_spell, int a_intensity, bool a_refresh)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !a_spell) {
			return;
		}
		bool hasIt = player->HasSpell(a_spell);
		bool wantIt = (a_intensity > 0) && EntryMatches(a_entry, player);
		if (a_refresh && hasIt) {
			player->RemoveSpell(a_spell);
			hasIt = false;
		}
		if (wantIt && !hasIt) {
			player->AddSpell(a_spell);
		} else if (!wantIt && hasIt) {
			player->RemoveSpell(a_spell);
		}
	}

	void ApplyEntry(const Entry& a_entry)
	{
		auto* globForm = RE::TESForm::LookupByEditorID(a_entry.globEDID);
		auto* glob = globForm ? globForm->As<RE::TESGlobal>() : nullptr;
		int intensity = glob ? static_cast<int>(glob->value + 0.5f) : kDefaultIntensity;
		if (intensity < 0) {
			intensity = 0;
		}
		if (intensity > kMaxIntensity) {
			intensity = kMaxIntensity;
		}
		const float scale = static_cast<float>(intensity) / 100.0f;

		auto* spellForm = RE::TESForm::LookupByEditorID(a_entry.spellEDID);
		auto* spell = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;

		auto lastIt = g_lastApplied.find(a_entry.spellEDID);
		const bool changed = (lastIt == g_lastApplied.end()) || (lastIt->second != intensity);

		ApplySpellScaling(a_entry, scale);
		ApplyPerkScaling(a_entry, scale);
		SyncSpellPresence(a_entry, spell, intensity, changed && intensity > 0);

		g_lastApplied[a_entry.spellEDID] = intensity;
	}

	void ApplyAllInternal()
	{
		for (const auto& entry : Table()) {
			ApplyEntry(entry);
		}
		// State sets add to whatever the race set did; they never replace it.
		for (const auto& entry : StateTable()) {
			ApplyEntry(entry);
		}
	}
}

namespace SyncScale
{
	void InitBaselines()
	{
		g_spellBase.clear();
		g_perkBase.clear();
		g_lastApplied.clear();
		for (const auto& entry : Table()) {
			CaptureSpellBaseline(entry);
			for (auto* perkEDID : entry.perkEDIDs) {
				CapturePerkBaseline(perkEDID);
			}
		}
		for (const auto& entry : StateTable()) {
			CaptureSpellBaseline(entry);
			for (auto* perkEDID : entry.perkEDIDs) {
				CapturePerkBaseline(perkEDID);
			}
		}
		SKSE::log::info("Captured baselines for {} races and {} states", Table().size(), StateTable().size());
	}

	void ReloadCustomRaces()
	{
		g_customRaceIds.clear();
		g_customRacePairs.clear();
		for (const auto& [editorid, setname] : Config::GetCustomRaces()) {
			auto* form = RE::TESForm::LookupByEditorID(editorid.c_str());
			auto* race = form ? form->As<RE::TESRace>() : nullptr;
			if (!race) {
				SKSE::log::warn("CustomRaces: race '{}' not found (passive set '{}')", editorid, setname);
				continue;
			}
			g_customRaceIds[ToLower(setname)].push_back(race->formID);
			g_customRacePairs.emplace_back(editorid, setname);
			SKSE::log::info("CustomRaces: {} -> {}", editorid, setname);
		}
	}

	const std::vector<std::pair<std::string, std::string>>& GetCustomRacePairs()
	{
		return g_customRacePairs;
	}

	void ApplyAll()
	{
		auto* task = SKSE::GetTaskInterface();
		if (!task) {
			return;
		}
		task->AddTask([]() { ApplyAllInternal(); });
	}

	int GetIntensity(const char* a_globEDID)
	{
		auto* form = RE::TESForm::LookupByEditorID(a_globEDID);
		auto* glob = form ? form->As<RE::TESGlobal>() : nullptr;
		return glob ? static_cast<int>(glob->value + 0.5f) : kDefaultIntensity;
	}

	void SetIntensity(const char* a_globEDID, int a_value)
	{
		if (a_value < 0) {
			a_value = 0;
		}
		if (a_value > kMaxIntensity) {
			a_value = kMaxIntensity;
		}
		auto* task = SKSE::GetTaskInterface();
		if (!task) {
			return;
		}
		task->AddTask([edid = std::string(a_globEDID), a_value]() {
			auto* form = RE::TESForm::LookupByEditorID(edid.c_str());
			auto* glob = form ? form->As<RE::TESGlobal>() : nullptr;
			if (glob) {
				glob->value = static_cast<float>(a_value);
			}
		});
	}

	void Commit(const char* a_globEDID, int a_value)
	{
		Config::Save(a_globEDID, a_value);
		SetIntensity(a_globEDID, a_value);
		ApplyAll();
	}

	void DisableAll()
	{
		// 1) persist 0 for every set in the ini
		for (const auto& entry : Table()) {
			Config::Save(entry.globEDID, 0);
		}
		for (const auto& entry : StateTable()) {
			Config::Save(entry.globEDID, 0);
		}
		// 2) zero every GLOB and re-run the sync (removes spells + perks from the player)
		auto* task = SKSE::GetTaskInterface();
		if (!task) {
			return;
		}
		task->AddTask([]() {
			for (const auto& entry : Table()) {
				auto* form = RE::TESForm::LookupByEditorID(entry.globEDID);
				if (auto* glob = form ? form->As<RE::TESGlobal>() : nullptr) {
					glob->value = 0.0f;
				}
			}
			for (const auto& entry : StateTable()) {
				auto* form = RE::TESForm::LookupByEditorID(entry.globEDID);
				if (auto* glob = form ? form->As<RE::TESGlobal>() : nullptr) {
					glob->value = 0.0f;
				}
			}
			ApplyAllInternal();
			SKSE::log::info("DisableAll: every set to 0, spells removed");
		});
	}

	std::vector<DisplayValue> GetDisplay(const char* a_raceName, int a_intensity)
	{
		std::vector<DisplayValue> out;
		const Entry* entry = nullptr;
		for (const auto& e : Table()) {
			if (std::string(e.name) == a_raceName) {
				entry = &e;
				break;
			}
		}
		if (!entry) {
			for (const auto& e : StateTable()) {
				if (std::string(e.name) == a_raceName) {
					entry = &e;
					break;
				}
			}
		}
		if (!entry) {
			return out;
		}

		const float scale = static_cast<float>(a_intensity) / 100.0f;

		// Spell effects (skip hidden Perk carriers).
		auto* spellForm = RE::TESForm::LookupByEditorID(entry->spellEDID);
		auto* spell = spellForm ? spellForm->As<RE::SpellItem>() : nullptr;
		auto baseIt = g_spellBase.find(entry->spellEDID);
		if (spell && baseIt != g_spellBase.end()) {
			for (std::size_t i = 0; i < spell->effects.size() && i < baseIt->second.size(); ++i) {
				auto* effect = spell->effects[i];
				if (!effect || !effect->baseEffect) {
					continue;
				}
				if (effect->baseEffect->data.flags.all(RE::EffectSetting::EffectSettingData::Flag::kHideInUI)) {
					continue;
				}
				const bool mult = IsMultiplierSpellEffect(entry->spellEDID, i);
				const float v = ScaleValue(baseIt->second[i], scale, mult);
				auto nameIt = g_mgefBaseName.find(effect->baseEffect->formID);
				const std::string label = (nameIt != g_mgefBaseName.end() && !nameIt->second.empty())
				                              ? nameIt->second
				                              : effect->baseEffect->fullName.c_str();
				out.push_back({ label, FormatValue(v, mult) });
			}
		}

		// Perk entry-point values.
		for (auto* perkEDID : entry->perkEDIDs) {
			auto* perkForm = RE::TESForm::LookupByEditorID(perkEDID);
			auto* perk = perkForm ? perkForm->As<RE::BGSPerk>() : nullptr;
			auto perkBaseIt = g_perkBase.find(perkEDID);
			if (!perk || perkBaseIt == g_perkBase.end()) {
				continue;
			}
			for (std::size_t i = 0; i < perk->perkEntries.size() && i < perkBaseIt->second.size(); ++i) {
				auto* ep = perk->perkEntries[i] ? static_cast<RE::BGSEntryPointPerkEntry*>(perk->perkEntries[i]) : nullptr;
				if (!ep || !ep->functionData || ep->functionData->GetType() != FuncData::kOneValue) {
					continue;
				}
				const bool mult = IsMultiplierPerkFunction(ep->entryData.function.get());
				const float v = ScaleValue(perkBaseIt->second[i], scale, mult);
				out.push_back({ perk->fullName.c_str(), FormatValue(v, mult) });
			}
		}

		return out;
	}
}
