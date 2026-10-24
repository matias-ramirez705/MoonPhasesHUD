#include "Widget.h"

#include "I18n.h"
#include "Moon.h"
#include "Settings.h"

namespace {
    constexpr auto kClipName = "MoonPhaseHUD_mc";
    constexpr auto kClipPath = "_root.MoonPhaseHUD_mc";
    constexpr auto kSwfFile = "MoonPhaseHUD.swf";  // Data/Interface/MoonPhaseHUD.swf
    // Different depth than By Time A Clock HUD (14100) so both widgets can coexist.
    constexpr double kClipDepth = 14103.0;
    constexpr auto kTickInterval = std::chrono::milliseconds(250);

    struct State {
        bool clipRequested = false;
        bool configured = false;
        bool stringsPushed = false;
        bool shownKnown = false;
        bool shown = false;
        bool warnedMissing = false;
        std::chrono::steady_clock::time_point requestedAt{};
        Settings::Data pushedSettings{};
        double pushedMasser = -1.0e9;
        double pushedSecunda = -1.0e9;
        int pushedStringsVersion = -1;
        double lastPeriodBucket = -1.0e9;
    };

    State state;  // only touched from the UI thread
    std::atomic<bool> toggledOn{true};
    std::atomic<long long> showUntilMs{0};
    std::atomic<bool> started{false};

    long long NowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    RE::GPtr<RE::GFxMovieView> GetHudMovie() {
        const auto ui = RE::UI::GetSingleton();
        if (!ui || !ui->IsMenuOpen(RE::HUDMenu::MENU_NAME)) return nullptr;
        const auto menu = ui->GetMenu(RE::HUDMenu::MENU_NAME);
        return menu ? menu->uiMovie : nullptr;
    }

    double PackRgb(const float (&c)[3]) {
        const auto channel = [](float v) { return static_cast<int>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f)); };
        return static_cast<double>((channel(c[0]) << 16) | (channel(c[1]) << 8) | channel(c[2]));
    }

    void RequestClip(RE::GFxMovieView& movie) {
        RE::GFxValue existing;
        if (movie.GetVariable(&existing, kClipPath) && !existing.IsUndefined()) {
            state.clipRequested = true;
            return;
        }
        const RE::GFxValue createArgs[2]{kClipName, kClipDepth};
        movie.Invoke("_root.createEmptyMovieClip", nullptr, createArgs, 2);
        const RE::GFxValue loadArgs[1]{kSwfFile};
        movie.Invoke("_root.MoonPhaseHUD_mc.loadMovie", nullptr, loadArgs, 1);
        state.clipRequested = true;
        state.requestedAt = std::chrono::steady_clock::now();
        logger::info("Requested widget {}", kSwfFile);
    }

    bool IsSwfReady(RE::GFxMovieView& movie) {
        RE::GFxValue ready;
        return movie.GetVariable(&ready, "_root.MoonPhaseHUD_mc.ready") && ready.IsBool() && ready.GetBool();
    }

    void PushConfig(RE::GFxMovieView& movie, const Settings::Data& s) {
        const RE::GFxValue args[25]{static_cast<double>(s.anchor % 3), static_cast<double>(s.anchor / 3),
                                    static_cast<double>(s.marginX),         static_cast<double>(s.marginY),
                                    static_cast<double>(s.scale),           static_cast<double>(s.opacity),
                                    s.showSky,                              s.showStars,
                                    s.customColor,                          PackRgb(s.badgeColor),
                                    s.showMasser,                           static_cast<double>(s.masserX),
                                    static_cast<double>(s.masserY),         static_cast<double>(s.masserScale),
                                    s.showSecunda,                          static_cast<double>(s.secundaX),
                                    static_cast<double>(s.secundaY),        static_cast<double>(s.secundaScale),
                                    s.showLabel,                            static_cast<double>(s.labelSource),
                                    static_cast<double>(s.labelX),          static_cast<double>(s.labelY),
                                    static_cast<double>(s.labelScale),      PackRgb(s.text),
                                    s.shadow};
        movie.Invoke("_root.MoonPhaseHUD_mc.setConfig", nullptr, args, 25);
        state.pushedSettings = s;
        state.configured = true;
    }

    // The eight phase names come from the translation file; the SWF also holds English
    // defaults, so the widget is never blank even before this call.
    void PushStrings(RE::GFxMovieView& movie) {
        const RE::GFxValue args[8]{I18n::TR("phase.0"), I18n::TR("phase.1"), I18n::TR("phase.2"), I18n::TR("phase.3"),
                                   I18n::TR("phase.4"), I18n::TR("phase.5"), I18n::TR("phase.6"), I18n::TR("phase.7")};
        movie.Invoke("_root.MoonPhaseHUD_mc.setStrings", nullptr, args, 8);
        state.stringsPushed = true;
        state.pushedStringsVersion = I18n::Version();
    }

    void PushPhases(RE::GFxMovieView& movie, const Moon::Snapshot& snap) {
        const RE::GFxValue args[2]{static_cast<double>(snap.masserPhase), static_cast<double>(snap.secundaPhase)};
        movie.Invoke("_root.MoonPhaseHUD_mc.setPhases", nullptr, args, 2);
        state.pushedMasser = snap.masserPhase;
        state.pushedSecunda = snap.secundaPhase;
    }

    bool PhasesChanged(const Moon::Snapshot& snap) {
        return std::fabs(static_cast<double>(snap.masserPhase) - state.pushedMasser) > 0.0005 ||
               std::fabs(static_cast<double>(snap.secundaPhase) - state.pushedSecunda) > 0.0005;
    }

    bool ShouldBeVisible(const Settings::Data& s, const Moon::Snapshot& snap) {
        if (!s.enabled) return false;

        const auto ui = RE::UI::GetSingleton();
        if (!ui || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME)) return false;
        // The console pauses the game; let the widget stay visible there unless the player opted out.
        const bool consoleOnly = s.showInConsole && ui->IsMenuOpen("Console");
        if (ui->GameIsPaused() && !consoleOnly) return false;

        if (s.hideInDialogue && ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME)) return false;

        if (s.hideInFurnitureMenu) {
            // "Furniture Menu" is the prompt used by beds, chairs and crafting stations; "Crafting Menu" is the smithing/alchemy/enchanting screen.
            if (ui->IsMenuOpen("Furniture Menu") || ui->IsMenuOpen("Crafting Menu")) return false;
        }

        if (s.hideInCombat) {
            const auto player = RE::PlayerCharacter::GetSingleton();
            if (player && player->IsInCombat()) return false;
        }

        switch (s.mode) {
            case Settings::kToggle: return toggledOn.load();
            case Settings::kTimed: return NowMs() < showUntilMs.load();
            case Settings::kPeriodic: {
                const double bucket = std::floor(static_cast<double>(snap.totalGameHours) / std::max(1, s.periodHours));
                if (bucket != state.lastPeriodBucket) {
                    // The first bucket seen after load only seeds the counter, it does not pop the widget up.
                    if (state.lastPeriodBucket > -1.0e8) showUntilMs = NowMs() + static_cast<long long>(s.showDuration * 1000.0f);
                    state.lastPeriodBucket = bucket;
                }
                return NowMs() < showUntilMs.load();
            }
            default: return true;
        }
    }

    void Tick() {
        const auto movie = GetHudMovie();
        if (!movie) {
            state = State{};  // HUD gone (main menu / loading): start over when it returns
            return;
        }

        if (!state.clipRequested) RequestClip(*movie);

        if (!IsSwfReady(*movie)) {
            if (!state.warnedMissing && std::chrono::steady_clock::now() - state.requestedAt > std::chrono::seconds(10)) {
                logger::warn("{} did not report ready. Is Data/Interface/{} installed?", kSwfFile, kSwfFile);
                state.warnedMissing = true;
            }
            return;
        }

        const Settings::Data settings = Settings::Current;
        const bool settingsChanged = !state.configured || std::memcmp(&settings, &state.pushedSettings, sizeof(Settings::Data)) != 0;
        if (settingsChanged) PushConfig(*movie, settings);

        if (!state.stringsPushed || state.pushedStringsVersion != I18n::Version()) PushStrings(*movie);

        const auto snapshot = Moon::Build();
        if (settingsChanged || PhasesChanged(snapshot)) PushPhases(*movie, snapshot);

        const bool visible = ShouldBeVisible(settings, snapshot);
        if (!state.shownKnown || visible != state.shown) {
            const RE::GFxValue arg[1]{visible};
            movie->Invoke("_root.MoonPhaseHUD_mc.setShown", nullptr, arg, 1);
            state.shown = visible;
            state.shownKnown = true;
        }
    }
}

void Widget::OnHudOpened() {
    // Executed on the UI thread through the menu open event; the movie is brand new, so forget what we pushed.
    state = State{};
}

void Widget::OnHotkey() {
    const auto& s = Settings::Current;
    if (s.mode == Settings::kToggle) {
        toggledOn = !toggledOn.load();
    } else if (s.mode == Settings::kTimed || s.mode == Settings::kPeriodic) {
        showUntilMs = NowMs() + static_cast<long long>(s.showDuration * 1000.0f);
    }
}

void Widget::Start() {
    if (started.exchange(true)) return;
    std::thread([] {
        while (true) {
            std::this_thread::sleep_for(kTickInterval);
            if (const auto tasks = SKSE::GetTaskInterface()) tasks->AddUITask([] { Tick(); });
        }
    }).detach();
}
