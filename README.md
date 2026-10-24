# Moon Phases HUD

A standalone Skyrim Special Edition HUD widget that shows the current phases of
Nirn's two moons, **Masser** and **Secunda**, drawn in the same visual style as the
moon badge of [By Time A Clock HUD](https://www.nexusmods.com/skyrimspecialedition/mods/195093)
— but as its own independent mod: no clock, no date lines, and **no dependency** on
the clock mod. Built on the same source architecture (CommonLibSSE plugin +
Scaleform widget + SKSE Menu Framework settings page), fully translatable from
plain `.txt` files.

## Features

- Night-sky badge with Masser and Secunda, each rendered in its **current phase**
  (Masser: 24-day cycle, Secunda: 31-day cycle, matching By Time A Clock HUD).
- Optional **phase-name text** under the moons (Masser's or Secunda's), with its own
  position, size, colour and shadow.
- Per-element placement like the clock mod: move each moon (X/Y), resize each moon,
  move and scale the text, move/scale/fade the whole widget, 9-screen-position anchor.
- Toggle the night-sky plate and stars for a clean "two floating moons" look.
- Natural colours, or a custom tint that keeps the badge's lights and shadows.
- Display modes: always / timed (hotkey) / toggle (hotkey) / periodic; auto-hide in
  combat, dialogue, furniture menus; keep-visible with console open.
- **Every text editable from txt files** — settings menu and widget — with automatic
  language detection and in-menu reload button.

## Requirements

- [Skyrim Special Edition](https://store.steampowered.com/app/489830/) + [SKSE64](https://skse.silverlock.org/)
- Optional (for the in-game settings menu): [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) —
  without it, everything is still configurable through `Data\SKSE\Plugins\MoonPhaseHUD.ini`.

## Install

1. Build the DLL once (see *Building*) or take `MoonPhaseHUD.dll` from a release.
2. Copy the `Data` folder contents (or install with MO2):
   - `Data\SKSE\Plugins\MoonPhaseHUD.dll` + `MoonPhaseHUD.ini`
   - `Data\Interface\MoonPhaseHUD.swf`
   - `Data\Interface\Translations\MoonPhaseHUD_english.txt` (+ `_spanish.txt`, `_template.txt`)
3. The widget appears with the HUD; open the settings with the Menu Framework
   (default **F1**) under the "Moon Phases HUD" section.

## Translations

- The active language is read from `SkyrimPrefs.ini` (`sLanguage`); override it with
  `Language=spanish` in `MoonPhaseHUD.ini`.
- Files: `Data\Interface\Translations\MoonPhaseHUD_<language>.txt` (UTF-8, `key = value`,
  `#` comments, `\n` for line breaks).
- Missing keys fall back to English (the file `MoonPhaseHUD_english.txt`, which is also
  compiled into the DLL as a safety net).
- To add a language: copy `MoonPhaseHUD_template.txt` to
  `MoonPhaseHUD_<language>.txt` and translate the values. That single file covers the
  whole mod — menu and widget.
- The *Language* section of the settings menu shows the active file and has a
  **Reload translation** button, so you can edit the txt and see changes immediately.

## Building

The DLL targets MSVC (Windows); two supported routes:

- **GitHub Actions**: push this repository (the workflow in
  `.github/workflows/build.yml` builds `MoonPhasesHUD.zip`: DLL + SWF + INI + translations).
- **Locally**: Visual Studio 2022 + [CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE)
  (`ng` branch) + vcpkg (baseline pinned in `vcpkg.json`), then
  `cmake --preset release && cmake --build build/release`.

The widget SWF is rebuilt from `swf_src/DoAction.as` with JPEXS FFDec
(`mkbase.py` creates the shell; see the repository history/scripts for the pipeline).
The repo already ships a built `dist/Data/Interface/MoonPhaseHUD.swf`.

## Repository layout

```
CMakeLists.txt / CMakePresets.json / vcpkg.json   build (CommonLibSSE plugin)
cmake/                                            helper cmake + vcpkg overlay ports
include/ src/                                     plugin sources (C++23, comments in English)
  plugin.cpp    SKSE entry point
  Settings.*    MoonPhaseHUD.ini load/save (plain data + Language override)
  I18n.*        translation loader (auto-detect, English fallback, reload)
  Moon.*        phase computation from GameDaysPassed
  Widget.*      HUD movie clip + GFx invoke loop (setConfig/setPhases/setStrings/setShown)
  UI.*          SKSE Menu Framework settings page, every label via I18n::TR()
  Events.*      hotkey + HUD open/close sinks
swf_src/                                          AS2 widget source (mkbase.py + DoAction.as)
dist/                                             ready-to-install Data tree (SWF, INI, translations)
test/                                             mirror tests (Python) keeping all parts in sync
```

## Credits

- Derived from the open source of **By Time A Clock HUD** by QY (Nexus 195093) —
  widget architecture, drawing style and moon phase model.
- Settings menu through the **SKSE Menu Framework** SDK header (Nexus 120352).
- `moon-mod/test` keeps the INI, translations, C++ defaults, AS2 and SWF consistent.
