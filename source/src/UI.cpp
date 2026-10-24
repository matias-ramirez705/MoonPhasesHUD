#include "UI.h"

#include "Events.h"
#include "I18n.h"
#include "Moon.h"
#include "Settings.h"
#include "SKSEMenuFramework.h"

// Defined in Events.cpp
bool __stdcall CaptureHotkeyInput(RE::InputEvent* event);

namespace {
    void Tooltip(const char* text) {
        if (ImGuiMCP::IsItemHovered()) ImGuiMCP::SetTooltip("%s", text);
    }

    // Builds the "a\0b\0c\0\0" string ImGui combos expect, from translated keys.
    std::string ComboItems(std::initializer_list<const char*> keys) {
        std::string out;
        for (const char* key : keys) {
            out += I18n::TR(key);
            out += '\0';
        }
        out += '\0';
        return out;
    }

    void __stdcall OnMenuEvent(SKSEMenuFramework::Model::EventType eventType) {
        if (eventType == SKSEMenuFramework::Model::kCloseMenu) {
            Events::CancelHotkeyCapture();
            Settings::Save();
        }
    }

    void RenderBehaviour(Settings::Data& s) {
        ImGuiMCP::SeparatorText(I18n::TR("section.behaviour"));
        ImGuiMCP::Checkbox(I18n::TR("enabled"), &s.enabled);

        ImGuiMCP::Combo(I18n::TR("mode"), &s.mode, ComboItems({"mode.0", "mode.1", "mode.2", "mode.3"}).c_str());
        Tooltip(I18n::TR("mode.tip"));

        const bool usesHotkey = s.mode != Settings::kAlways;
        if (usesHotkey) {
            const auto label = Events::IsCapturingHotkey()
                                   ? std::string(I18n::TR("hotkey.capture"))
                                   : std::format("{} {}  {}", I18n::TR("hotkey"), Events::HotkeyName(s.hotkey), I18n::TR("hotkey.click"));
            if (ImGuiMCP::Button(label.c_str())) {
                if (Events::IsCapturingHotkey()) Events::CancelHotkeyCapture();
                else Events::BeginHotkeyCapture();
            }
        }
        if (s.mode == Settings::kTimed || s.mode == Settings::kPeriodic) {
            ImGuiMCP::SliderFloat(I18n::TR("showDuration"), &s.showDuration, 1.0f, 30.0f, "%.0f");
        }
        if (s.mode == Settings::kPeriodic) {
            ImGuiMCP::SliderInt(I18n::TR("periodHours"), &s.periodHours, 1, 24);
        }
        ImGuiMCP::Checkbox(I18n::TR("hideInCombat"), &s.hideInCombat);
        ImGuiMCP::Checkbox(I18n::TR("showInConsole"), &s.showInConsole);
        Tooltip(I18n::TR("showInConsole.tip"));
        ImGuiMCP::Checkbox(I18n::TR("hideInDialogue"), &s.hideInDialogue);
        Tooltip(I18n::TR("hideInDialogue.tip"));
        ImGuiMCP::Checkbox(I18n::TR("hideInFurnitureMenu"), &s.hideInFurnitureMenu);
        Tooltip(I18n::TR("hideInFurnitureMenu.tip"));
    }

    void RenderMoons(Settings::Data& s) {
        ImGuiMCP::SeparatorText(I18n::TR("section.moons"));
        ImGuiMCP::Checkbox(I18n::TR("showSky"), &s.showSky);
        Tooltip(I18n::TR("showSky.tip"));
        if (s.showSky) ImGuiMCP::Checkbox(I18n::TR("showStars"), &s.showStars);

        ImGuiMCP::Checkbox(I18n::TR("showMasser"), &s.showMasser);
        Tooltip(I18n::TR("showMasser.tip"));
        if (s.showMasser) {
            ImGuiMCP::SliderInt(I18n::TR("masser.x"), &s.masserX, -300, 300);
            ImGuiMCP::SliderInt(I18n::TR("masser.y"), &s.masserY, -300, 300);
            ImGuiMCP::SliderFloat(I18n::TR("masser.scale"), &s.masserScale, 20.0f, 300.0f, "%.0f");
        }
        ImGuiMCP::Checkbox(I18n::TR("showSecunda"), &s.showSecunda);
        Tooltip(I18n::TR("showSecunda.tip"));
        if (s.showSecunda) {
            ImGuiMCP::SliderInt(I18n::TR("secunda.x"), &s.secundaX, -300, 300);
            ImGuiMCP::SliderInt(I18n::TR("secunda.y"), &s.secundaY, -300, 300);
            ImGuiMCP::SliderFloat(I18n::TR("secunda.scale"), &s.secundaScale, 20.0f, 300.0f, "%.0f");
        }

        ImGuiMCP::Checkbox(I18n::TR("customColor"), &s.customColor);
        Tooltip(I18n::TR("customColor.tip"));
        if (s.customColor) ImGuiMCP::ColorEdit3(I18n::TR("badgeColor"), s.badgeColor);
    }

    void RenderLabel(Settings::Data& s) {
        ImGuiMCP::SeparatorText(I18n::TR("section.label"));
        ImGuiMCP::Checkbox(I18n::TR("showLabel"), &s.showLabel);
        if (!s.showLabel) return;
        ImGuiMCP::Combo(I18n::TR("labelSource"), &s.labelSource, ComboItems({"labelSource.0", "labelSource.1"}).c_str());
        ImGuiMCP::SliderInt(I18n::TR("label.x"), &s.labelX, -300, 300);
        ImGuiMCP::SliderInt(I18n::TR("label.y"), &s.labelY, -300, 300);
        ImGuiMCP::SliderFloat(I18n::TR("label.scale"), &s.labelScale, 40.0f, 300.0f, "%.0f");
        ImGuiMCP::Checkbox(I18n::TR("shadow"), &s.shadow);
        ImGuiMCP::ColorEdit3(I18n::TR("textColor"), s.text);
    }

    void RenderPlacement(Settings::Data& s) {
        ImGuiMCP::SeparatorText(I18n::TR("section.placement"));
        ImGuiMCP::Combo(I18n::TR("anchor"), &s.anchor,
                        ComboItems({"anchor.0", "anchor.1", "anchor.2", "anchor.3", "anchor.4", "anchor.5", "anchor.6", "anchor.7", "anchor.8"}).c_str());
        ImGuiMCP::SliderInt(I18n::TR("marginX"), &s.marginX, -200, 400);
        ImGuiMCP::SliderInt(I18n::TR("marginY"), &s.marginY, -200, 400);
        ImGuiMCP::SliderFloat(I18n::TR("scale"), &s.scale, 40.0f, 250.0f, "%.0f");
        ImGuiMCP::SliderFloat(I18n::TR("opacity"), &s.opacity, 10.0f, 100.0f, "%.0f");
    }

    void RenderLanguage() {
        ImGuiMCP::SeparatorText(I18n::TR("section.language"));
        ImGuiMCP::Text("%s %s", I18n::TR("lang.active"), I18n::Language());
        ImGuiMCP::Text("%s %s", I18n::TR("lang.file"), I18n::FileName());
        static bool reloaded = false;
        if (ImGuiMCP::Button(I18n::TR("lang.reload"))) {
            I18n::Reload();
            reloaded = true;
        }
        if (reloaded) ImGuiMCP::Text("%s %s", I18n::TR("lang.reloadDone"), I18n::Language());
        ImGuiMCP::Spacing();
        ImGuiMCP::TextWrapped("%s", I18n::TR("lang.hint"));
    }

    void RenderPreview() {
        ImGuiMCP::SeparatorText(I18n::TR("section.preview"));
        const auto snap = Moon::Build();
        ImGuiMCP::Text("%s %s", I18n::TR("preview.masser"), Moon::PhaseName(snap.masserIndex));
        ImGuiMCP::Text("%s %s", I18n::TR("preview.secunda"), Moon::PhaseName(snap.secundaIndex));
    }

    void __stdcall RenderSettings() {
        auto& s = Settings::Current;
        RenderBehaviour(s);
        RenderMoons(s);
        RenderLabel(s);
        RenderPlacement(s);
        RenderLanguage();
        RenderPreview();

        ImGuiMCP::Spacing();
        if (ImGuiMCP::Button(I18n::TR("saveNow"))) Settings::Save();
        ImGuiMCP::SameLine();
        if (ImGuiMCP::Button(I18n::TR("resetDefaults"))) Settings::ResetToDefaults();
    }
}

void UI::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        logger::warn("SKSE Menu Framework not found, settings are only available through MoonPhaseHUD.ini");
        return;
    }
    SKSEMenuFramework::SetSection("Moon Phases HUD");
    SKSEMenuFramework::AddSectionItem(I18n::TR("settings"), RenderSettings);
    SKSEMenuFramework::AddEvent(OnMenuEvent, 0.0f);
    SKSEMenuFramework::AddInputEvent(CaptureHotkeyInput);
}
