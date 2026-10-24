#!/usr/bin/env python3
"""Mirror tests for Moon Phases HUD.

Every check mirrors a piece of the shipped artefacts so the parts stay in sync:
  1. MoonPhaseHUD_english.txt  ==  the C++ built-in default table (I18n.cpp kDefaults)
  2. MoonPhaseHUD_spanish.txt  covers every english key (no gaps -> no English leaks)
  3. MoonPhaseHUD_template.txt has the same key set
  4. The txt parser mirror (BOM / CRLF / comments / later-wins / \\n unescape / first '=' split)
  5. Phase bucketing: C++ lround(p*8)%8 == AS2 floor(p*8+0.5)%8 over both moon cycles
  6. DLL -> SWF call contract: argument counts match between Widget.cpp and DoAction.as
  7. Shipped INI keys == the fields Settings.cpp reads/saves (+ Language)
  8. Widget anchor math mirror (all 9 anchors, label on/off, scale)
  9. SWF built-in English phase names == english.txt phase.0..7
"""
import math
import re
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent  # the repository root
TR_DIR = ROOT / "dist" / "Data" / "Interface" / "Translations"
FAILS = []


def check(name, ok, detail=""):
    print(f"  [{'OK' if ok else 'FAIL'}] {name}" + (f" - {detail}" if detail and not ok else ""))
    if not ok:
        FAILS.append(name)


# ---- 1. extract the C++ default table -------------------------------------------------------
def cpp_defaults():
    text = (ROOT / "src" / "I18n.cpp").read_text(encoding="utf-8")
    body = text.split("constexpr Entry kDefaults[] = {", 1)[1].split("};", 1)[0]
    out = {}
    for key, value in re.findall(r'\{\s*"((?:[^"\\]|\\.)*)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*\}', body):
        # unescape C++ string literals the same way the compiler does
        value = value.replace("\\n", "\n").replace("\\\\", "\\").replace('\\"', '"')
        out[key] = value
    return out


# ---- 4. parser mirror of I18n.cpp ParseFile --------------------------------------------------
def parse_file(text):
    if text.startswith("\ufeff"):
        text = text[1:]
    out = {}
    for line in text.splitlines():
        line = line.rstrip("\r").strip(" \t\r\n")
        if not line or line.startswith("#"):
            continue
        eq = line.find("=")
        if eq < 0:
            continue
        key = line[:eq].strip(" \t\r\n")
        value = line[eq + 1:].strip(" \t\r\n").replace("\\n", "\n")
        if key:
            out[key] = value
    return out


def phase_name_index(p):
    i = math.floor(p * 8 + 0.5)
    i %= 8
    if i < 0:
        i += 8
    return i


def lround(x):  # C++ std::lround for positive values: half away from zero
    return math.floor(x + 0.5)


def main():
    print("== 1. english.txt == C++ kDefaults")
    defaults = cpp_defaults()
    english = parse_file((TR_DIR / "MoonPhaseHUD_english.txt").read_text(encoding="utf-8"))
    check("same key sets", set(defaults) == set(english),
          f"only-cpp={sorted(set(defaults) - set(english))[:8]} only-txt={sorted(set(english) - set(defaults))[:8]}")
    diff = {k: (defaults[k], english[k]) for k in defaults if k in english and defaults[k] != english[k]}
    check("same values", not diff, f"{len(diff)} differ, e.g. {list(diff.items())[:3]}")
    check("no empty values", all(english.values()))

    print("== 2. spanish.txt covers every english key")
    spanish = parse_file((TR_DIR / "MoonPhaseHUD_spanish.txt").read_text(encoding="utf-8"))
    missing = sorted(set(english) - set(spanish))
    check("no missing keys", not missing, f"missing: {missing[:10]}")
    check("no empty values", all(spanish.values()))
    check("spanish is actually translated", spanish["phase.4"] != english["phase.4"])

    print("== 3. template.txt has the same keys")
    template = parse_file((TR_DIR / "MoonPhaseHUD_template.txt").read_text(encoding="utf-8"))
    check("same key set", set(template) == set(english),
          f"diff: {sorted(set(template) ^ set(english))[:10]}")

    print("== 4. parser mirror (quirks of I18n.cpp ParseFile)")
    check("BOM skipped", parse_file("\ufeffa = 1\n") == {"a": "1"})
    check("CRLF handled", parse_file("a = 1\r\nb = 2\r\n") == {"a": "1", "b": "2"})
    check("comments skipped (# only)", parse_file("# c\nb = 2\n") == {"b": "2"})
    check("';' is not a comment (stored as key)", parse_file(";a = 9\nb = 2\n") == {";a": "9", "b": "2"})
    check("later wins", parse_file("a = 1\na = 2\n") == {"a": "2"})
    check("first = splits", parse_file("a = x = y\n") == {"a": "x = y"})
    check(r"\n unescaped", parse_file("k = line1\\nline2\n") == {"k": "line1\nline2"})
    check("backslashes kept", parse_file("k = Data\\Interface\\X\n") == {"k": "Data\\Interface\\X"})
    check("empty value falls back", parse_file("k =\n") == {"k": ""})

    print("== 5. phase buckets: C++ lround == AS2 floor(+0.5), both cycles")
    mismatches = []
    for cycle_days, name in ((24, "Masser"), (31, "Secunda")):
        for d in range(0, cycle_days * 3 + 1):
            p = (d % cycle_days) / cycle_days
            c_idx = lround(p * 8) % 8
            as_idx = phase_name_index(p)
            if c_idx != as_idx:
                mismatches.append((name, d, p, c_idx, as_idx))
    check("identical over 3 full cycles", not mismatches, str(mismatches[:5]))

    seen = {phase_name_index((d % 24) / 24) for d in range(24)}
    check("all 8 phases occur in a Masser cycle", seen == set(range(8)), str(sorted(seen)))
    # day granularity sanity: day 0 = new moon, day 12 of 24 = full
    check("day 0 -> New moon", phase_name_index(0.0) == 0)
    check("day 12/24 -> full", phase_name_index(12 / 24) == 4)

    print("== 6. DLL -> SWF call contract (arg counts)")
    as2 = (ROOT / "swf_src" / "DoAction.as").read_text(encoding="utf-8")
    widget = (ROOT / "src" / "Widget.cpp").read_text(encoding="utf-8")

    def as2_params(fn):
        m = re.search(rf"function {fn}\(([^)]*)\)", as2)
        return len(m.group(1).split(",")) if m and m.group(1).strip() else (0 if m else -1)

    def cpp_args(fn):
        m = re.search(rf'Invoke\("_root\.MoonPhaseHUD_mc\.{fn}", nullptr, arg[s]?, (\d+)\)', widget)
        return int(m.group(1)) if m else -1

    for fn, expected in [("setConfig", 25), ("setPhases", 2), ("setStrings", 8), ("setShown", 1)]:
        check(f"{fn}: AS2 {as2_params(fn)} == C++ {cpp_args(fn)} == {expected}",
              as2_params(fn) == cpp_args(fn) == expected)

    # the 25 setConfig parameters must arrive in the order the AS2 function expects
    cpp_push = re.search(r"PushConfig\(RE::GFxMovieView& movie, const Settings::Data& s\) \{(.*?)\n    \}", widget, re.S).group(1)
    fields = re.findall(r"s\.(\w+)", cpp_push)
    expected_order = ["anchor", "anchor", "marginX", "marginY", "scale", "opacity", "showSky", "showStars",
                      "customColor", "badgeColor", "showMasser", "masserX", "masserY", "masserScale",
                      "showSecunda", "secundaX", "secundaY", "secundaScale", "showLabel", "labelSource",
                      "labelX", "labelY", "labelScale", "text", "shadow"]
    check("setConfig arg order matches AS2 names", len(fields) == 25 and fields == expected_order,
          f"got {fields}")

    print("== 7. shipped INI keys == Settings.cpp fields (+ Language)")
    ini_text = (ROOT / "dist" / "Data" / "SKSE" / "Plugins" / "MoonPhaseHUD.ini").read_text(encoding="utf-8")
    ini_keys = {ln.split("=")[0].strip() for ln in ini_text.splitlines()
                if "=" in ln and not ln.lstrip().startswith(";") and not ln.lstrip().startswith("[")}
    settings_cpp = (ROOT / "src" / "Settings.cpp").read_text(encoding="utf-8")
    visit_keys = set(re.findall(r'visit\("(\w+)",', settings_cpp))
    check("INI == VisitFields + Language", ini_keys == visit_keys | {"Language"},
          f"ini-only={sorted(ini_keys - visit_keys - {'Language'})} visit-only={sorted(visit_keys - ini_keys)}")

    print("== 8. widget anchor math mirror (relayout)")
    def place(ax, ay, mx, my, scale, label, label_bottom=92):
        W = 72
        H = label_bottom if label else 72
        sc = scale / 100
        vr = {"x": 0, "y": 0, "width": 1280, "height": 720}
        if ax == 0:
            px = vr["x"] + mx
        elif ax == 1:
            px = vr["x"] + (vr["width"] - W * sc) / 2 + mx
        else:
            px = vr["x"] + vr["width"] - W * sc - mx
        if ay == 0:
            py = vr["y"] + my
        elif ay == 1:
            py = vr["y"] + (vr["height"] - H * sc) / 2 + my
        else:
            py = vr["y"] + vr["height"] - H * sc - my
        return px, py, W * sc, H * sc

    px, py, w, h = place(2, 0, 40, 40, 100, True)  # default: top right
    check("default TR anchor", (px, py) == (1280 - 72 - 40, 40))
    px, py, w, h = place(1, 1, 0, 0, 100, True)  # centre
    check("centre anchor centred", abs(px - (1280 - 72) / 2) < 0.001 and abs(py - (720 - 92) / 2) < 0.001)
    px, py, w, h = place(2, 2, 10, 10, 200, False)  # bottom right, 200%, no label
    check("scaled BR anchor", (px, py) == (1280 - 144 - 10, 720 - 144 - 10))
    # every anchor inside the visible stage at default scale
    inside = all(0 <= place(a % 3, a // 3, 40, 40, 100, True)[0] <= 1280
                 and 0 <= place(a % 3, a // 3, 40, 40, 100, True)[1] <= 720 for a in range(9))
    check("all 9 anchors inside stage", inside)

    print("== 9. SWF built-in English names == english.txt phases")
    names = re.search(r"var NAMES = \[([^]]*)\];", as2).group(1)
    swf_names = [s.strip().strip('"') for s in names.split(",")]
    txt_names = [english[f"phase.{i}"] for i in range(8)]
    check("same list", swf_names == txt_names, f"swf={swf_names} txt={txt_names}")

    print()
    if FAILS:
        print(f"FAILED: {len(FAILS)} -> {FAILS}")
        sys.exit(1)
    print("ALL CHECKS PASSED")


if __name__ == "__main__":
    main()
