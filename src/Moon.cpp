#include "Moon.h"

#include "I18n.h"

namespace {
    // Calendar globals (Skyrim.esm): GameDaysPassed drives the moon cycles.
    constexpr RE::FormID kGameHour = 0x38;
    constexpr RE::FormID kGameDaysPassed = 0x39;

    // Masser completes its cycle in 24 in-game days, Secunda in 31 (same values as
    // By Time A Clock HUD's Twin Moons style and moon badge, so the mods agree).
    constexpr float kMasserCycleDays = 24.0f;
    constexpr float kSecundaCycleDays = 31.0f;

    float GlobalValue(RE::FormID id, float fallback) {
        const auto* global = RE::TESForm::LookupByID<RE::TESGlobal>(id);
        return global ? global->value : fallback;
    }

    // Index = round(phase * 8) mod 8: 0 new, 2 first quarter, 4 full, 6 last quarter.
    int MoonPhaseIndex(float phase) {
        return static_cast<int>(std::lround(phase * 8.0f)) % 8;
    }
}

namespace Moon {
    Snapshot Build() {
        Snapshot snapshot;

        const float daysPassed = GlobalValue(kGameDaysPassed, 0.0f);
        snapshot.masserPhase = std::fmod(daysPassed, kMasserCycleDays) / kMasserCycleDays;
        snapshot.secundaPhase = std::fmod(daysPassed, kSecundaCycleDays) / kSecundaCycleDays;
        snapshot.masserIndex = MoonPhaseIndex(snapshot.masserPhase);
        snapshot.secundaIndex = MoonPhaseIndex(snapshot.secundaPhase);

        const float hour = std::clamp(GlobalValue(kGameHour, 12.0f), 0.0f, 23.9999f);
        snapshot.dayFraction = hour / 24.0f;
        snapshot.totalGameHours = daysPassed * 24.0f;
        return snapshot;
    }

    const char* PhaseName(int index) {
        const int safe = ((index % 8) + 8) % 8;
        return I18n::TR(std::format("phase.{}", safe).c_str());
    }
}
