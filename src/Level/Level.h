#pragma once

#include "olcPixelGameEngine.h"
#include "ArraySequence.h"

struct Collectible {
    olc::vf2d pos;
    float     radius    = 15.0f;
    bool      collected = false;
};

struct Finish {
    olc::vf2d pos;
    float     radius = 15.0f;
    bool      active = false;   // активен только когда собраны все яблоки
};

struct Hazard {
    olc::vf2d pos;
    float     radius = 12.0f;
};

// полигональный ландшафт + интерактивные объекты
class Level {
public:
    // Ландшафт: вершины полигона -> отрезки линий
    MutableArraySequence<olc::vf2d> vertices;

    struct Segment {
        olc::vf2d a, b;
    };
    MutableArraySequence<Segment> segments;

    // Интерактивные объекты
    MutableArraySequence<Collectible> collectibles;
    Finish                            finish;
    MutableArraySequence<Hazard>      hazards;

    // Точка спавна
    olc::vf2d spawnPos = {150.0f, 400.0f};

    Level() = default;
    Level(const Level&) = delete;
    Level& operator=(const Level&) = delete;

    // Очистка всех данных
    void ClearAll() {
        while (vertices.GetLength() > 0) vertices.RemoveLast();
        while (segments.GetLength() > 0) segments.RemoveLast();
        while (collectibles.GetLength() > 0) collectibles.RemoveLast();
        while (hazards.GetLength() > 0) hazards.RemoveLast();
        finish = Finish{};
        spawnPos = {150.0f, 400.0f};
    }

    // Построение отрезков из вершин (авто-замыкание полигона)
    void BuildSegments() {
        while (segments.GetLength() > 0) segments.RemoveLast();
        int n = vertices.GetLength();
        for (int i = 0; i < n; i++) {
            Segment seg;
            seg.a = vertices.Get(i);
            seg.b = vertices.Get((i + 1) % n);
            segments.Append(seg);
        }
    }

    // Проверка, собраны ли все предметы
    bool AllCollected() const {
        for (int i = 0; i < collectibles.GetLength(); i++) {
            if (!collectibles.Get(i).collected) return false;
        }
        return true;
    }

    // Сброс состояния уровня (для рестарта, геометрия сохраняется)
    void Reset() {
        for (int i = 0; i < collectibles.GetLength(); i++) {
            collectibles[i].collected = false;
        }
        finish.active = false;
    }
};
