#include "Events.h"
#include "I18n.h"
#include "Settings.h"
#include "UI.h"
#include "Widget.h"
#include "logger.h"

namespace {
    void OnMessage(SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            Settings::Load();   // reads MoonPhaseHUD.ini (including the Language override)
            I18n::Load();       // reads Data/Interface/Translations/MoonPhaseHUD_<lang>.txt
            UI::Register();
            Events::Register();
            Widget::Start();
        }
    }
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* skse) {
    // SKSE::Init must run first: it resolves the plugin name/handle and the SKSE interfaces.
    // It also sets up its own default log, which would reopen (and truncate) the same
    // <plugin>.log, so it is turned off here and SetupLog() below creates the log instead.
    SKSE::Init(skse, {.log = false});
    SetupLog();
    logger::info("Plugin loaded");
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
