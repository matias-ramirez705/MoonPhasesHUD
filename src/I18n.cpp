#include "I18n.h"

#include "Settings.h"

#include <mutex>

namespace {
    // Built-in English defaults. The shipped MoonPhaseHUD_english.txt mirrors this table
    // exactly (checked by the repository's test script), so a missing or blocked
    // translation file never changes what the player sees.
    struct Entry {
        const char* key;
        const char* value;
    };
    constexpr Entry kDefaults[] = {
        {"phase.0", "New moon"},
        {"phase.1", "Waxing crescent"},
        {"phase.2", "First quarter"},
        {"phase.3", "Waxing gibbous"},
        {"phase.4", "Full moon"},
        {"phase.5", "Waning gibbous"},
        {"phase.6", "Last quarter"},
        {"phase.7", "Waning crescent"},

        {"moon.masser", "Masser"},
        {"moon.secunda", "Secunda"},

        {"settings", "Settings"},
        {"section.behaviour", "Behaviour"},
        {"section.moons", "Moons"},
        {"section.label", "Phase text"},
        {"section.placement", "Placement"},
        {"section.language", "Language"},
        {"section.preview", "Right now"},

        {"enabled", "Enabled"},
        {"mode", "Display mode"},
        {"mode.0", "Always"},
        {"mode.1", "Timed (hotkey)"},
        {"mode.2", "Toggle (hotkey)"},
        {"mode.3", "Periodic"},
        {"mode.tip", "Always: visible whenever no menu is open.\nTimed: visible for a few seconds after the hotkey.\nToggle: the hotkey shows and hides the widget.\nPeriodic: pops up every few in-game hours."},
        {"hotkey", "Hotkey:"},
        {"hotkey.click", "(click to change)"},
        {"hotkey.capture", "Press a key... (Esc to cancel)"},
        {"showDuration", "Visible for (seconds)"},
        {"periodHours", "Every (in-game hours)"},
        {"hideInCombat", "Hide in combat"},
        {"showInConsole", "Keep visible while the console is open"},
        {"showInConsole.tip", "The console pauses the game and the widget normally hides while paused.\nUncheck to hide it with the console too."},
        {"hideInDialogue", "Hide during conversations"},
        {"hideInDialogue.tip", "Hides the widget while you talk to someone (the dialogue menu is open) and brings it back when the conversation ends."},
        {"hideInFurnitureMenu", "Hide in furniture menus"},
        {"hideInFurnitureMenu.tip", "Hides the widget while a furniture menu is open: the prompt for beds, chairs and other furniture, and the crafting screens (forge, grindstone, workbench, alchemy, enchanting, cooking)."},

        {"showMasser", "Show Masser (greater moon)"},
        {"showMasser.tip", "Masser completes its cycle in 24 in-game days."},
        {"showSecunda", "Show Secunda (lesser moon)"},
        {"showSecunda.tip", "Secunda completes its cycle in 31 in-game days."},
        {"masser.x", "Masser X"},
        {"masser.y", "Masser Y"},
        {"masser.scale", "Masser size (%)"},
        {"secunda.x", "Secunda X"},
        {"secunda.y", "Secunda Y"},
        {"secunda.scale", "Secunda size (%)"},
        {"showSky", "Night-sky plate"},
        {"showSky.tip", "The dark circle with stars behind the moons.\nUncheck for two clean floating moons."},
        {"showStars", "Stars"},
        {"customColor", "Custom colour"},
        {"customColor.tip", "Off: the moons keep their natural colours (warm pale Masser, cool blue Secunda) on the night sky.\nOn: the whole picture is tinted with the colour you choose, keeping its lights and shadows."},
        {"badgeColor", "Badge colour"},

        {"showLabel", "Show the phase name"},
        {"labelSource", "Phase of"},
        {"labelSource.0", "Masser"},
        {"labelSource.1", "Secunda"},
        {"label.x", "Text X"},
        {"label.y", "Text Y"},
        {"label.scale", "Text size (%)"},
        {"shadow", "Text shadow"},
        {"textColor", "Text colour"},

        {"anchor", "Position on screen"},
        {"anchor.0", "Top left"},
        {"anchor.1", "Top centre"},
        {"anchor.2", "Top right"},
        {"anchor.3", "Middle left"},
        {"anchor.4", "Centre"},
        {"anchor.5", "Middle right"},
        {"anchor.6", "Bottom left"},
        {"anchor.7", "Bottom centre"},
        {"anchor.8", "Bottom right"},
        {"marginX", "Offset X"},
        {"marginY", "Offset Y"},
        {"scale", "Size (%)"},
        {"opacity", "Opacity (%)"},

        {"lang.active", "Active language:"},
        {"lang.file", "Translation file:"},
        {"lang.builtin", "(built-in English defaults)"},
        {"lang.reload", "Reload translation"},
        {"lang.reloadDone", "Translation reloaded:"},
        {"lang.hint", "To translate the mod, copy MoonPhaseHUD_template.txt to MoonPhaseHUD_<language>.txt\nin Data\\Interface\\Translations and edit the texts.\nThe language is taken from the game (SkyrimPrefs.ini, sLanguage); you can also force one\nwith Language=... in Data\\SKSE\\Plugins\\MoonPhaseHUD.ini. Missing keys fall back to English.\nChanges are read on game start, or right now with the Reload button."},

        {"preview.masser", "Masser:"},
        {"preview.secunda", "Secunda:"},

        {"saveNow", "Save now"},
        {"resetDefaults", "Reset to defaults"},
    };

    std::mutex g_mutex;
    std::unordered_map<std::string, std::string> g_loaded;
    std::string g_language = "english";
    std::string g_fileName;
    bool g_fileFound = false;
    std::atomic<int> g_version{0};

    std::string Trim(std::string s) {
        const auto b = s.find_first_not_of(" \t\r\n");
        const auto e = s.find_last_not_of(" \t\r\n");
        return b == std::string::npos ? std::string{} : s.substr(b, e - b + 1);
    }

    std::string Lower(std::string s) {
        for (auto& c : s)
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        return s;
    }

    // Literal "\n" sequences in the file become real newlines; other backslashes are kept.
    std::string Unescape(std::string s) {
        std::string out;
        out.reserve(s.size());
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'n') {
                out += '\n';
                ++i;
            } else {
                out += s[i];
            }
        }
        return out;
    }

    // Reads one "key = value" translation file into out. Returns the number of keys parsed.
    int ParseFile(const std::string& path, std::unordered_map<std::string, std::string>& out) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return 0;

        std::string line;
        bool first = true;
        int count = 0;
        while (std::getline(file, line)) {
            if (first) {
                // Skip a UTF-8 byte order mark if present.
                if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
                    static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF)
                    line = line.substr(3);
                first = false;
            }
            if (!line.empty() && line.back() == '\r') line.pop_back();
            line = Trim(std::move(line));
            if (line.empty() || line[0] == '#') continue;

            const auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            const auto key = Trim(line.substr(0, eq));
            const auto value = Unescape(Trim(line.substr(eq + 1)));
            if (key.empty()) continue;
            out[key] = value;  // later lines override earlier ones
            ++count;
        }
        return count;
    }

    // sLanguage from SkyrimPrefs.ini / Skyrim.ini [General]; both live in the game root
    // (the working directory of the SKSE process, also under MO2's virtual file system).
    std::string DetectGameLanguage() {
        for (const char* name : {"SkyrimPrefs.ini", "Skyrim.ini"}) {
            std::ifstream file(name);
            if (!file) continue;
            bool inGeneral = false;
            for (std::string line; std::getline(file, line);) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                const auto text = Trim(std::move(line));
                if (text.empty()) continue;
                if (text[0] == '[') {
                    inGeneral = Lower(text) == "[general]";
                    continue;
                }
                if (!inGeneral) continue;
                const auto eq = text.find('=');
                if (eq == std::string::npos) continue;
                if (Lower(Trim(text.substr(0, eq))) == "slanguage")
                    return Lower(Trim(text.substr(eq + 1)));
            }
        }
        return "english";
    }

    std::string LanguageFromSettings() {
        auto lang = Trim(Settings::LanguageOverride);
        if (lang.empty()) lang = DetectGameLanguage();
        if (lang.empty()) lang = "english";
        return Lower(std::move(lang));
    }

    // English file first, then the active language on top: keys missing from the
    // translation fall back to English, like the game's own translation system.
    void LoadInto(std::unordered_map<std::string, std::string>& out, std::string& fileName, bool& found) {
        const std::string lang = LanguageFromSettings();
        const std::string english = "Data/Interface/Translations/MoonPhaseHUD_english.txt";
        const std::string localized = "Data/Interface/Translations/MoonPhaseHUD_" + lang + ".txt";

        found = false;
        fileName.clear();

        int englishKeys = ParseFile(english, out);
        if (englishKeys > 0) {
            found = true;
            fileName = "MoonPhaseHUD_english.txt";
        }

        if (lang != "english") {
            const int langKeys = ParseFile(localized, out);
            if (langKeys > 0) {
                found = true;
                fileName = "MoonPhaseHUD_" + lang + ".txt";
            }
        }
    }
}

void I18n::Load() {
    std::unordered_map<std::string, std::string> fresh;
    std::string fileName;
    bool found = false;
    LoadInto(fresh, fileName, found);

    {
        const std::lock_guard lock(g_mutex);
        g_loaded = std::move(fresh);
        g_fileName = std::move(fileName);
        g_fileFound = found;
        g_language = LanguageFromSettings();
    }
    g_version.fetch_add(1);

    logger::info("Translations: language '{}' from {} ({} keys)", g_language,
                 g_fileFound ? g_fileName : std::string("built-in defaults"),
                 [&] {
                     const std::lock_guard lock(g_mutex);
                     return g_loaded.size();
                 }());
}

void I18n::Reload() { Load(); }

const char* I18n::TR(const char* key) {
    {
        const std::lock_guard lock(g_mutex);
        const auto it = g_loaded.find(key);
        if (it != g_loaded.end()) return it->second.c_str();  // valid until the next Reload
    }
    for (const auto& entry : kDefaults)
        if (std::string_view(entry.key) == key) return entry.value;
    return key;
}

std::string I18n::TRv(const char* key) { return TR(key); }

bool I18n::Has(const char* key) {
    const std::lock_guard lock(g_mutex);
    return g_loaded.contains(key);
}

const char* I18n::Language() { return g_language.c_str(); }

const char* I18n::FileName() {
    const std::lock_guard lock(g_mutex);
    return g_fileFound ? g_fileName.c_str() : "(built-in English defaults)";
}

bool I18n::FileFound() {
    const std::lock_guard lock(g_mutex);
    return g_fileFound;
}

int I18n::Version() { return g_version.load(); }
