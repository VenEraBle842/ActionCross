#pragma once

#include "Level/Level.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

// Статический утилитный класс для работы с файлами уровней формата .aclvl
class LevelLoader {
public:
    // Утилита для динамической проверки наличия уровней
    static bool Exists(const std::string& filepath) {
        std::ifstream file(filepath);
        return file.is_open();
    }

    static void LoadFromAclvl(Level& level, const std::string& filepath) {
        level.ClearAll();

        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Could not load the level: " << filepath << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "SPAWN") {
                float x, y;
                if (ss >> x >> y) level.SetSpawnPos({x, y});
            } else if (type == "FINISH") {
                float x, y;
                // Level забирает владение выделенной памятью
                if (ss >> x >> y) level.SetFinish(new Finish({x, y}));
            } else if (type == "APPLE") {
                float x, y;
                if (ss >> x >> y) level.AddCollectible(new Collectible({x, y}));
            } else if (type == "SPIKE") {
                float x, y;
                if (ss >> x >> y) level.AddHazard(new Hazard({x, y}));
            } else if (type == "TERRAIN") {
                float x, y;
                while (ss >> x >> y) {
                    level.AddVertex({x, y});
                }
                level.BuildSegments();
            }
        }
    }
};
