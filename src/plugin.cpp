#include "Menu.h"
#include "Runtime.h"
#include "Settings.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

namespace O2BoostRecharge
{
    void InitializeLog();
}

void O2BoostRecharge::InitializeLog()
{
    try {
        if (const auto directory = SFSE::GetLogDirectory()) {
            const auto logFile = *directory / (PLUGIN_NAME ".log");
            const auto log = logger::basic_logger_mt(PLUGIN_NAME, logFile.string(), true);
            log->sinks().push_back(std::make_shared<logger::sinks::msvc_sink_mt>());
#ifdef NDEBUG
            constexpr auto level = logger::level::info;
#else
            constexpr auto level = logger::level::debug;
#endif
            log->set_level(level);
            log->flush_on(level);
            log->set_pattern("[%T.%e] [%=5t] [%L] %v");
            logger::set_default_logger(log);
        }
    } catch (const logger::spdlog_ex& error) {
        logger::warn("Could not start the file log: {}", error.what());
    }
}

namespace
{
    void MessageCallback(SFSE::MessagingInterface::Message* a_message)
    {
        if (a_message->type == SFSE::MessagingInterface::kPostLoad) {
            (void)O2BoostRecharge::Settings::Load();
            O2BoostRecharge::Menu::Register();
        } else if (a_message->type == SFSE::MessagingInterface::kPostDataLoad) {
            (void)O2BoostRecharge::Runtime::Install();
        }
    }
}

extern "C"
{
    __declspec(dllexport) constinit SFSE::PluginVersionData SFSEPlugin_Version{
        1,
        PLUGIN_VERSION_PACKED,
        PLUGIN_NAME,
        PLUGIN_AUTHOR,
        1U << 2,
        1U << 3,
        { SFSE::RUNTIME_SF_1_16_244 },
        0,
        0,
        0
    };
}

extern "C" __declspec(dllexport) bool SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
    O2BoostRecharge::InitializeLog();
    if (!a_sfse || !SFSE::Init(*a_sfse, 0)) {
        logger::error("Could not initialize SFSE for Starfield 1.16.244");
        return false;
    }
    const auto messaging = SFSE::GetMessagingInterface();
    if (messaging && messaging->RegisterListener(MessageCallback)) {
        logger::info("Message listener registered");
        return true;
    }

    logger::error("Could not register SFSE message listener");
    return false;
}
