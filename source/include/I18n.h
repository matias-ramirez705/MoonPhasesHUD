#pragma once

// Runtime translation support.
//
// All user-visible texts of the mod (settings menu + widget phase names) come from
//   Data\Interface\Translations\MoonPhaseHUD_<language>.txt   (UTF-8, "key = value")
// The active language is auto-detected from SkyrimPrefs.ini [General] sLanguage,
// unless MoonPhaseHUD.ini sets Language=... . Missing keys fall back to the English
// defaults compiled into the DLL, so the mod always works even without any txt file.

namespace I18n {
    // Loads the translation for the detected/configured language (call once at startup).
    void Load();

    // Re-reads the translation file (menu button); bumps Version() so the widget re-pushes texts.
    void Reload();

    // Translated string for a key. Falls back to the built-in English default, then to the key.
    // The returned pointer stays valid for the lifetime of the process.
    const char* TR(const char* key);

    // Same lookup, but returns "" when unknown (for values appended to other strings).
    std::string TRv(const char* key);

    // True when the key exists in the loaded translation file (used to fall back cleanly).
    bool Has(const char* key);

    // Status shown in the settings menu.
    const char* Language();    // active language code, e.g. "spanish"
    const char* FileName();    // file that provided the texts, or "<built-in defaults>"
    bool FileFound();          // whether a translation file was read

    // Increments on every Load/Reload; the widget compares it to re-push the phase names.
    int Version();
}
