#pragma once

#include "olcPixelGameEngine.h"
#include "ArraySequence.h"
#include <unordered_map>
#include <cstdint>
#include <utility>
#include <algorithm>

// Ускорение коллизий в широкой фазе.
// Разделяет мир на ячейки. Каждая ячейка хранит индексы отрезков линий,
// которые перекрывают ее. Запрос по AABB (ограничивающему прямоугольнику)
// позволяет быстро получить кандидатов для проверки.
class SpatialHash {
public:
    float cellSize = 64.0f;

    SpatialHash() = default;
    explicit SpatialHash(float cs) : cellSize(cs) {}

    void Clear() { cells.clear(); }

    // Вставка отрезка линии (по индексу), охватывающего от p1 до p2 + отступ
    void Insert(int segIndex, olc::vf2d p1, olc::vf2d p2) {
        float margin = 2.0f;
        float minX = std::min(p1.x, p2.x) - margin;
        float minY = std::min(p1.y, p2.y) - margin;
        float maxX = std::max(p1.x, p2.x) + margin;
        float maxY = std::max(p1.y, p2.y) + margin;

        int x0 = (int)std::floor(minX / cellSize);
        int y0 = (int)std::floor(minY / cellSize);
        int x1 = (int)std::floor(maxX / cellSize);
        int y1 = (int)std::floor(maxY / cellSize);

        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
                cells[HashKey(x, y)].Append(segIndex);
    }

    // Запрос: возвращает все индексы отрезков, которые могут пересекаться с AABB
    MutableArraySequence<int> Query(olc::vf2d pos, float radius) const {
        MutableArraySequence<int> result;
        int x0 = (int)std::floor((pos.x - radius) / cellSize);
        int y0 = (int)std::floor((pos.y - radius) / cellSize);
        int x1 = (int)std::floor((pos.x + radius) / cellSize);
        int y1 = (int)std::floor((pos.y + radius) / cellSize);

        for (int y = y0; y <= y1; y++) {
            for (int x = x0; x <= x1; x++) {
                auto it = cells.find(HashKey(x, y));
                if (it != cells.end()) {
                    for (int i = 0; i < it->second.GetLength(); i++) {
                        result.Append(it->second.Get(i));
                    }
                }
            }
        }

        // Удаление дубликатов: сортировка и последующее сжатие
        int n = result.GetLength();
        if (n > 1) {
            // Простая сортировка вставками (число элементов в ячейках невелико)
            for (int i = 1; i < n; i++) {
                int key = result[i];
                int j = i - 1;
                while (j >= 0 && result[j] > key) {
                    result[j + 1] = result[j];
                    j--;
                }
                result[j + 1] = key;
            }
            // Удаление идущих подряд дубликатов
            MutableArraySequence<int> unique;
            unique.Append(result[0]);
            for (int i = 1; i < n; i++) {
                if (result[i] != result[i - 1]) {
                    unique.Append(result[i]);
                }
            }
            return unique;
        }

        return result;
    }

private:
    std::unordered_map<int64_t, MutableArraySequence<int>> cells;

    static int64_t HashKey(int x, int y) {
        return ((int64_t)x << 32) | (int64_t)(uint32_t)y;
    }
};
