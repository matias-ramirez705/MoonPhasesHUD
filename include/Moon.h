#pragma once

namespace Moon {
    struct Snapshot {
        float masserPhase = 0.0f;   // 0..1: 0 = new, 0.25 = first quarter, 0.5 = full (24-day cycle)
        float secundaPhase = 0.0f;  // 0..1 (31-day cycle)
        int masserIndex = 0;        // 0..7, the eight classical phase buckets
        int secundaIndex = 0;
        float dayFraction = 0.5f;   // 0 = midnight, 0.5 = noon
        float totalGameHours = 0.0f;
    };

    // Reads the in-game calendar globals and computes the current moon phases.
    Snapshot Build();

    // Phase name from the translation table ("phase.0" .. "phase.7").
    const char* PhaseName(int index);
}
