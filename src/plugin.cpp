#include "Menu.h"
#include "Runtime.h"
#include "Settings.h"

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

SFSE_PLUGIN_LOAD(const SFSE::LoadInterface* a_sfse)
{
    SFSE::Init(a_sfse);
    const auto* messaging = SFSE::GetMessagingInterface();
    if (messaging && messaging->RegisterListener(MessageCallback)) {
        logger::info("Message listener registered");
        return true;
    }

    logger::error("Could not register SFSE message listener");
    return false;
}
