#pragma once

#include "Level/Level.h"
#include "ArraySequence.h"
#include <fstream>
#include <sstream>
#include <string>

class LevelLoader {
public:
    static void LoadFromAclvl(Level& level, const std::string& filepath) {
        level.ClearAll(); // очищаем старые данные

        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Could not load the level: " << filepath << std::endl;
            return;
        } // если файла нет, останется пустой уровень

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "SPAWN") {
                float x, y;
                if (ss >> x >> y) level.spawnPos = {x, y};
            } else if (type == "FINISH") {
                float x, y;
                if (ss >> x >> y) level.finish.pos = {x, y};
            } else if (type == "APPLE") {
                float x, y;
                if (ss >> x >> y) {
                    Collectible c;
                    c.pos = {x, y};
                    level.collectibles.Append(c);
                }
            } else if (type == "SPIKE") {
                float x, y;
                if (ss >> x >> y) {
                    Hazard h;
                    h.pos = {x, y};
                    level.hazards.Append(h);
                }
            } else if (type == "TERRAIN") {
                float x, y;
                // очищаем вершины перед сборкой нового полигона
                while (level.vertices.GetLength() > 0) level.vertices.RemoveLast();

                while (ss >> x >> y) {
                    level.vertices.Append({x, y});
                }
                level.BuildSegments(); // преобразуем вершины в отрезки коллизий
            }
        }
    }
};
