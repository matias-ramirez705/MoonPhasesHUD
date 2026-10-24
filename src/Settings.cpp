#include "Settings.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace {
    constexpr auto kIniPath = "Data/SKSE/Plugins/MoonPhaseHUD.ini";

    void Parse(const std::string& text, bool& out) { out = (text == "1" || text == "true" || text == "True"); }
    void Parse(const std::string& text, int& out) { out = std::stoi(text); }
    void Parse(const std::string& text, float& out) { out = std::stof(text); }

    std::string Print(bool v) { return v ? "1" : "0"; }
    std::string Print(int v) { return std::to_string(v); }
    std::string Print(float v) { return std::format("{:.3f}", v); }

    // Single place that lists every persisted field; used for both loading and saving.
    template <class Visitor>
    void VisitFields(Settings::Data& d, Visitor&& visit) {
        visit("Enabled", d.enabled);
        visit("Mode", d.mode);
        visit("Hotkey", d.hotkey);
        visit("ShowDuration", d.showDuration);
        visit("PeriodHours", d.periodHours);
        visit("HideInCombat", d.hideInCombat);
        visit("ShowInConsole", d.showInConsole);
        visit("HideInDialogue", d.hideInDialogue);
        visit("HideInFurnitureMenu", d.hideInFurnitureMenu);

        visit("Anchor", d.anchor);
        visit("MarginX", d.marginX);
        visit("MarginY", d.marginY);
        visit("Scale", d.scale);
        visit("Opacity", d.opacity);

        visit("ShowSky", d.showSky);
        visit("ShowStars", d.showStars);
        visit("CustomColor", d.customColor);
        visit("BadgeColorR", d.badgeColor[0]);
        visit("BadgeColorG", d.badgeColor[1]);
        visit("BadgeColorB", d.badgeColor[2]);

        visit("ShowMasser", d.showMasser);
        visit("MasserX", d.masserX);
        visit("MasserY", d.masserY);
        visit("MasserScale", d.masserScale);
        visit("ShowSecunda", d.showSecunda);
        visit("SecundaX", d.secundaX);
        visit("SecundaY", d.secundaY);
        visit("SecundaScale", d.secundaScale);

        visit("ShowLabel", d.showLabel);
        visit("LabelSource", d.labelSource);
        visit("LabelX", d.labelX);
        visit("LabelY", d.labelY);
        visit("LabelScale", d.labelScale);
        visit("Shadow", d.shadow);
        visit("TextR", d.text[0]);
        visit("TextG", d.text[1]);
        visit("TextB", d.text[2]);
    }

    void Sanitize(Settings::Data& d) {
        d.mode = std::clamp(d.mode, 0, 3);
        d.anchor = std::clamp(d.anchor, 0, 8);
        d.labelSource = std::clamp(d.labelSource, 0, 1);
        d.marginX = std::clamp(d.marginX, -400, 600);
        d.marginY = std::clamp(d.marginY, -400, 600);
        d.periodHours = std::clamp(d.periodHours, 1, 24);
        d.showDuration = std::clamp(d.showDuration, 1.0f, 60.0f);
        d.scale = std::clamp(d.scale, 40.0f, 250.0f);
        d.opacity = std::clamp(d.opacity, 10.0f, 100.0f);
        for (int* v : {&d.masserX, &d.masserY, &d.secundaX, &d.secundaY, &d.labelX, &d.labelY}) *v = std::clamp(*v, -300, 300);
        for (float* v : {&d.masserScale, &d.secundaScale}) *v = std::clamp(*v, 20.0f, 300.0f);
        d.labelScale = std::clamp(d.labelScale, 40.0f, 300.0f);
        for (auto& c : d.badgeColor) c = std::clamp(c, 0.0f, 1.0f);
        for (auto& c : d.text) c = std::clamp(c, 0.0f, 1.0f);
    }
}

void Settings::Load() {
    std::ifstream file(kIniPath);
    if (!file) {
        logger::info("No settings file found, using defaults");
        Save();
        return;
    }

    std::unordered_map<std::string, std::string> values;
    for (std::string line; std::getline(file, line);) {
        const auto eq = line.find('=');
        if (eq == std::string::npos || line.starts_with(';') || line.starts_with('[')) continue;
        auto trim = [](std::string s) {
            const auto b = s.find_first_not_of(" \t\r");
            const auto e = s.find_last_not_of(" \t\r");
            return b == std::string::npos ? std::string{} : s.substr(b, e - b + 1);
        };
        values[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }

    // String option handled outside Data (which must stay trivially copyable).
    if (const auto it = values.find("Language"); it != values.end()) LanguageOverride = it->second;

    Data loaded = Current;
    VisitFields(loaded, [&](const char* key, auto& field) {
        const auto it = values.find(key);
        if (it == values.end()) return;
        try {
            Parse(it->second, field);
        } catch (const std::exception&) {
            logger::warn("Ignoring invalid value '{}' for setting {}", it->second, key);
        }
    });
    Sanitize(loaded);
    Current = loaded;
    logger::info("Settings loaded");
}

void Settings::Save() {
    Data copy = Current;
    std::ostringstream out;
    out << "[MoonPhaseHUD]\n";
    out << "; Translation language override: english, spanish, french, ... Empty = follow the game language.\n";
    out << "Language=" << LanguageOverride << "\n";
    VisitFields(copy, [&](const char* key, auto& field) { out << key << '=' << Print(field) << '\n'; });

    std::ofstream file(kIniPath, std::ios::trunc);
    if (!file) {
        logger::error("Could not write settings to {}", kIniPath);
        return;
    }
    file << out.str();
}

void Settings::ResetToDefaults() { Current = Data{}; }
