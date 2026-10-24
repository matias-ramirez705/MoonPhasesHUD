#pragma once

namespace Widget {
    // Called when the HUD menu is (re)created, e.g. on game load.
    void OnHudOpened();

    // Starts the background pacemaker that updates the widget a few times per second.
    void Start();

    // Hotkey pressed in game: toggles or briefly shows the widget depending on the display mode.
    void OnHotkey();
}
