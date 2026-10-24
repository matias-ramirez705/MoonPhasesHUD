#include "Events.h"

#include "Settings.h"
#include "Widget.h"

namespace {
    std::atomic<bool> capturing{false};

    constexpr int kMouseOffset = 256;
    constexpr int kEscape = 0x01;

    // Turns a button event into one of our key codes (DirectInput scan code, or 256+ for mouse buttons).
    int KeyCodeOf(const RE::ButtonEvent& button) {
        switch (button.GetDevice()) {
            case RE::INPUT_DEVICE::kKeyboard: return static_cast<int>(button.GetIDCode());
            case RE::INPUT_DEVICE::kMouse: return static_cast<int>(button.GetIDCode()) + kMouseOffset;
            default: return -1;
        }
    }

    class HotkeySink : public RE::BSTEventSink<RE::InputEvent*> {
    public:
        static HotkeySink* GetSingleton() {
            static HotkeySink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* events, RE::BSTEventSource<RE::InputEvent*>*) override {
            if (!events || capturing.load()) return RE::BSEventNotifyControl::kContinue;

            const auto ui = RE::UI::GetSingleton();
            if (!ui || ui->GameIsPaused()) return RE::BSEventNotifyControl::kContinue;

            for (auto event = *events; event; event = event->next) {
                if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) continue;
                const auto button = static_cast<RE::ButtonEvent*>(event);
                if (!button->IsDown()) continue;
                if (KeyCodeOf(*button) == Settings::Current.hotkey) Widget::OnHotkey();
            }
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    class HudSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static HudSink* GetSingleton() {
            static HudSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (event && event->opening && event->menuName == RE::HUDMenu::MENU_NAME.data()) Widget::OnHudOpened();
            return RE::BSEventNotifyControl::kContinue;
        }
    };
}

// Called by the Menu Framework (via UI.cpp) while the settings window is open; true = swallow the event.
bool __stdcall CaptureHotkeyInput(RE::InputEvent* event) {
    if (!capturing.load() || !event || event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) return false;
    const auto button = static_cast<RE::ButtonEvent*>(event);
    if (!button->IsDown()) return false;

    const int code = KeyCodeOf(*button);
    if (code < 0) return false;

    if (code != kEscape) Settings::Current.hotkey = code;
    capturing = false;
    return true;
}

void Events::Register() {
    if (const auto input = RE::BSInputDeviceManager::GetSingleton()) input->AddEventSink(HotkeySink::GetSingleton());
    if (const auto ui = RE::UI::GetSingleton()) ui->AddEventSink(HudSink::GetSingleton());
}

void Events::BeginHotkeyCapture() { capturing = true; }
void Events::CancelHotkeyCapture() { capturing = false; }
bool Events::IsCapturingHotkey() { return capturing.load(); }

std::string Events::HotkeyName(int code) {
    static const std::unordered_map<int, const char*> names{
        {0x01, "Escape"}, {0x02, "1"}, {0x03, "2"}, {0x04, "3"}, {0x05, "4"}, {0x06, "5"}, {0x07, "6"}, {0x08, "7"},
        {0x09, "8"}, {0x0A, "9"}, {0x0B, "0"}, {0x0C, "-"}, {0x0D, "="}, {0x0E, "Backspace"}, {0x0F, "Tab"},
        {0x10, "Q"}, {0x11, "W"}, {0x12, "E"}, {0x13, "R"}, {0x14, "T"}, {0x15, "Y"}, {0x16, "U"}, {0x17, "I"},
        {0x18, "O"}, {0x19, "P"}, {0x1A, "["}, {0x1B, "]"}, {0x1C, "Enter"}, {0x1D, "Left Ctrl"}, {0x1E, "A"},
        {0x1F, "S"}, {0x20, "D"}, {0x21, "F"}, {0x22, "G"}, {0x23, "H"}, {0x24, "J"}, {0x25, "K"}, {0x26, "L"},
        {0x27, ";"}, {0x28, "'"}, {0x29, "`"}, {0x2A, "Left Shift"}, {0x2B, "\\"}, {0x2C, "Z"}, {0x2D, "X"},
        {0x2E, "C"}, {0x2F, "V"}, {0x30, "B"}, {0x31, "N"}, {0x32, "M"}, {0x33, ","}, {0x34, "."}, {0x35, "/"},
        {0x36, "Right Shift"}, {0x37, "Numpad *"}, {0x38, "Left Alt"}, {0x39, "Space"}, {0x3A, "Caps Lock"},
        {0x3B, "F1"}, {0x3C, "F2"}, {0x3D, "F3"}, {0x3E, "F4"}, {0x3F, "F5"}, {0x40, "F6"}, {0x41, "F7"},
        {0x42, "F8"}, {0x43, "F9"}, {0x44, "F10"}, {0x45, "Num Lock"}, {0x46, "Scroll Lock"}, {0x47, "Numpad 7"},
        {0x48, "Numpad 8"}, {0x49, "Numpad 9"}, {0x4A, "Numpad -"}, {0x4B, "Numpad 4"}, {0x4C, "Numpad 5"},
        {0x4D, "Numpad 6"}, {0x4E, "Numpad +"}, {0x4F, "Numpad 1"}, {0x50, "Numpad 2"}, {0x51, "Numpad 3"},
        {0x52, "Numpad 0"}, {0x53, "Numpad ."}, {0x57, "F11"}, {0x58, "F12"}, {0x9C, "Numpad Enter"},
        {0x9D, "Right Ctrl"}, {0xB5, "Numpad /"}, {0xB8, "Right Alt"}, {0xC7, "Home"}, {0xC8, "Up"}, {0xC9, "Page Up"},
        {0xCB, "Left"}, {0xCD, "Right"}, {0xCF, "End"}, {0xD0, "Down"}, {0xD1, "Page Down"}, {0xD2, "Insert"},
        {0xD3, "Delete"}, {256, "Mouse 1"}, {257, "Mouse 2"}, {258, "Mouse 3"}, {259, "Mouse 4"}, {260, "Mouse 5"}};
    const auto it = names.find(code);
    return it != names.end() ? it->second : std::format("Key {}", code);
}
