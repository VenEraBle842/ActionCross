#pragma once

#include "Level/Level.h"

namespace LevelLoader {
    inline void CreateDefaultLevel(Level& level) {
        level.ClearAll();

        // Холмистый полигональный ландшафт (по часовой стрелке = земля снаружи)
        level.vertices.Append({-200.0f, 800.0f});
        level.vertices.Append({-200.0f, 500.0f});
        level.vertices.Append({0.0f,    480.0f});
        level.vertices.Append({100.0f,  470.0f});
        level.vertices.Append({200.0f,  460.0f});
        level.vertices.Append({350.0f,  450.0f});
        level.vertices.Append({500.0f,  430.0f});
        level.vertices.Append({600.0f,  400.0f});
        level.vertices.Append({700.0f,  420.0f});
        level.vertices.Append({800.0f,  440.0f});
        level.vertices.Append({900.0f,  430.0f});
        level.vertices.Append({1000.0f, 400.0f});
        level.vertices.Append({1100.0f, 380.0f});
        level.vertices.Append({1200.0f, 410.0f});
        level.vertices.Append({1350.0f, 450.0f});
        level.vertices.Append({1500.0f, 420.0f});
        level.vertices.Append({1650.0f, 390.0f});
        level.vertices.Append({1800.0f, 350.0f});
        level.vertices.Append({1950.0f, 400.0f});
        level.vertices.Append({2100.0f, 440.0f});
        level.vertices.Append({2300.0f, 460.0f});
        level.vertices.Append({2500.0f, 460.0f});
        level.vertices.Append({2600.0f, 500.0f});
        level.vertices.Append({2600.0f, 800.0f});

        level.BuildSegments();

        level.spawnPos = {120.0f, 420.0f};

        // Яблоки
        Collectible a1; a1.pos = {400.0f, 400.0f}; level.collectibles.Append(a1);
        Collectible a2; a2.pos = {800.0f, 390.0f}; level.collectibles.Append(a2);
        Collectible a3; a3.pos = {1200.0f, 360.0f}; level.collectibles.Append(a3);
        Collectible a4; a4.pos = {1800.0f, 300.0f}; level.collectibles.Append(a4);
        Collectible a5; a5.pos = {2200.0f, 420.0f}; level.collectibles.Append(a5);

        level.finish.pos = {2400.0f, 420.0f};

        Hazard h; h.pos = {650.0f,  420.0f}; level.hazards.Append(h);
    }
};
