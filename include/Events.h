#pragma once

namespace Events {
    void Register();

    // Hotkey rebinding, used by the settings page.
    void BeginHotkeyCapture();
    void CancelHotkeyCapture();
    bool IsCapturingHotkey();
    std::string HotkeyName(int code);
}
