#pragma once

namespace Settings {
    enum DisplayMode : int { kAlways = 0, kTimed = 1, kToggle = 2, kPeriodic = 3 };

    // Plain data only (trivially copyable) so changes can be detected with a memcmp.
    // The string option (Language) lives outside this struct on purpose.
    struct Data {
        // General behaviour (same model as By Time A Clock HUD)
        bool enabled = true;
        int mode = kAlways;
        int hotkey = 0x41;  // F7 (DirectInput scan code; mouse buttons are 256+)
        float showDuration = 5.0f;  // seconds, Timed / Periodic
        int periodHours = 6;        // in-game hours, Periodic
        bool hideInCombat = false;
        bool showInConsole = true;         // keep the widget visible while the console is open
        bool hideInDialogue = false;       // while talking to an NPC
        bool hideInFurnitureMenu = false;  // crafting stations, beds and other furniture menus

        // Widget placement (the whole badge, on the visible stage)
        int anchor = 2;  // 0..8 = TL TC TR ML MC MR BL BC BR
        int marginX = 40;
        int marginY = 40;
        float scale = 100.0f;    // percent
        float opacity = 100.0f;  // percent

        // Night-sky plate behind the moons
        bool showSky = true;
        bool showStars = true;
        bool customColor = false;  // false = natural colours (warm pale Masser, cool blue Secunda)
        float badgeColor[3] = {0.85f, 0.80f, 0.70f};

        // Masser (greater moon), placed inside the badge
        bool showMasser = true;
        int masserX = 0;             // pixels, relative to the badge default position
        int masserY = 0;
        float masserScale = 100.0f;  // percent

        // Secunda (lesser moon)
        bool showSecunda = true;
        int secundaX = 0;
        int secundaY = 0;
        float secundaScale = 100.0f;

        // Phase-name text under the moons
        bool showLabel = true;
        int labelSource = 0;  // 0 = Masser, 1 = Secunda
        int labelX = 0;
        int labelY = 0;
        float labelScale = 100.0f;
        bool shadow = true;
        float text[3] = {0.95f, 0.92f, 0.87f};
    };

    inline Data Current;

    // Language override from the INI ("" = auto-detect from the game settings).
    inline std::string LanguageOverride;

    void Load();
    void Save();
    void ResetToDefaults();
}
