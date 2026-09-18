#include "PCH.h"

#include "Config.h"
#include "SyncScale.h"
#include "UI.h"

namespace
{
	void SetupLog()
	{
		auto logsFolder = SKSE::log::log_directory();
		if (!logsFolder) {
			SKSE::stl::report_and_fail("no log dir");
		}
		auto path = *logsFolder / "RacePassives.log";
		auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
		auto logger = std::make_shared<spdlog::logger>("log", std::move(fileSink));
		spdlog::set_default_logger(std::move(logger));
		spdlog::set_level(spdlog::level::trace);
		spdlog::flush_on(spdlog::level::trace);
	}

	class RaceSwitchSink : public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESSwitchRaceCompleteEvent* a_event,
			RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) override
		{
			if (a_event && a_event->subject && a_event->subject->formID == 0x14) {
				SyncScale::ApplyAll();
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	RaceSwitchSink g_raceSwitchSink;

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			SyncScale::InitBaselines();
			SyncScale::ReloadCustomRaces();
			Config::Load();
			SyncScale::ApplyAll();
			UIRenderer::Register();
			if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
				holder->AddEventSink(&g_raceSwitchSink);
			} else {
				SKSE::log::warn("no ScriptEventSourceHolder for race-switch sink");
			}
			break;
		case SKSE::MessagingInterface::kPostLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			SyncScale::ApplyAll();
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SetupLog();
	SKSE::log::info("RacePassives loading");
	// Thin shell: menu + record scaling, no hooks, no trampoline needed.
	SKSE::Init(a_skse, SKSE::InitInfo{ .log = true, .trampoline = false });

	if (!SKSE::GetMessagingInterface()->RegisterListener(OnMessage)) {
		SKSE::log::error("Failed to register message listener");
		return false;
	}

	SKSE::log::info("RacePassives loaded");
	return true;
}
