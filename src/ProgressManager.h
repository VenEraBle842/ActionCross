#pragma once
#include <fstream>

// Управляет сохранением и загрузкой пользовательского прогресса
class ProgressManager {
    int unlockedLevel = 1;

public:
    void LoadProgress() {
        if (std::ifstream file("save.dat"); file.is_open()) {
            file >> unlockedLevel;
        } else {
            unlockedLevel = 1;
        }
    }

    void SaveProgress() {
        if (std::ofstream file("save.dat"); file.is_open()) {
            file << unlockedLevel;
        }
    }

    int GetUnlockedLevel() const { return unlockedLevel; }

    void UnlockNextLevel(int currentLevel) {
        // Если игрок прошел свой самый последний доступный уровень — открываем следующий
        if (currentLevel == unlockedLevel) {
            unlockedLevel++;
            SaveProgress();
        }
    }

    void ResetProgress() {
        unlockedLevel = 1;
        SaveProgress();
    }
};
