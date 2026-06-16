#pragma once

#include "olcPixelGameEngine.h"
#include "ArraySequence.h"

// Полиморфный базовый класс для интерактивных сущностей на уровне
class Interactable {
protected:
    olc::vf2d pos;
    float radius;
    bool active;

public:
    Interactable(olc::vf2d p, float r) : pos(p), radius(r), active(true) {}
    virtual ~Interactable() = default;

    virtual void OnInteract() = 0;

    olc::vf2d GetPos() const { return pos; }
    float GetRadius() const { return radius; }
    bool IsActive() const { return active; }
    void SetActive(bool a) { active = a; }
};

class Collectible : public Interactable {
public:
    explicit Collectible(olc::vf2d p) : Interactable(p, 15.0f) {}
    void OnInteract() override { SetActive(false); }
};

class Finish : public Interactable {
public:
    explicit Finish(olc::vf2d p) : Interactable(p, 15.0f) {
        SetActive(false); // Финиш активируется только после сбора всех collectibles
    }
    void OnInteract() override {}
};

class Hazard : public Interactable {
public:
    explicit Hazard(olc::vf2d p) : Interactable(p, 12.0f) {}
    void OnInteract() override {}
};

// Контейнер уровня
class Level {
public:
    struct Segment {
        olc::vf2d a, b;
    };

private:
    MutableArraySequence<olc::vf2d> vertices;
    MutableArraySequence<Segment> segments;

    MutableArraySequence<Collectible*> collectibles;
    MutableArraySequence<Hazard*> hazards;
    Finish* finish = nullptr;

    olc::vf2d spawnPos = {150.0f, 400.0f};

public:
    Level() = default;

    // Запрет копирования предотвращает двойное удаление памяти указателей
    Level(const Level&) = delete;
    Level& operator=(const Level&) = delete;

    ~Level() { ClearAll(); }

    // Очистка памяти при загрузке нового уровня
    void ClearAll() {
        while (vertices.GetLength() > 0) vertices.RemoveLast();
        while (segments.GetLength() > 0) segments.RemoveLast();

        for (int i = 0; i < collectibles.GetLength(); i++) delete collectibles.Get(i);
        while (collectibles.GetLength() > 0) collectibles.RemoveLast();

        for (int i = 0; i < hazards.GetLength(); i++) delete hazards.Get(i);
        while (hazards.GetLength() > 0) hazards.RemoveLast();

        if (finish != nullptr) {
            delete finish;
            finish = nullptr;
        }

        spawnPos = {150.0f, 400.0f};
    }

    // Построение отрезков коллизий по заданным вершинам
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

    bool AllCollected() const {
        for (int i = 0; i < collectibles.GetLength(); i++) {
            if (collectibles.Get(i)->IsActive()) return false;
        }
        return true;
    }

    void Reset() {
        for (int i = 0; i < collectibles.GetLength(); i++) {
            collectibles.Get(i)->SetActive(true);
        }
        if (finish) finish->SetActive(false);
    }

    void AddVertex(olc::vf2d v) { vertices.Append(v); }
    void AddCollectible(Collectible* c) { collectibles.Append(c); }
    void AddHazard(Hazard* h) { hazards.Append(h); }

    void SetFinish(Finish* f) {
        delete finish;
        finish = f;
    }

    void SetSpawnPos(olc::vf2d pos) { spawnPos = pos; }

    float GetLowestPoint() const {
        float max_y = 0.0f;
        for (int i = 0; i < vertices.GetLength(); i++) {
            if (vertices.Get(i).y > max_y) {
                max_y = vertices.Get(i).y;
            }
        }
        return max_y;
    }

    int GetVerticesCount() const { return vertices.GetLength(); }
    olc::vf2d GetVertex(int i) const { return vertices.Get(i); }

    int GetSegmentsCount() const { return segments.GetLength(); }
    Segment GetSegment(int i) const { return segments.Get(i); }

    int GetCollectiblesCount() const { return collectibles.GetLength(); }
    Collectible* GetCollectible(int i) const { return collectibles.Get(i); }

    int GetHazardsCount() const { return hazards.GetLength(); }
    Hazard* GetHazard(int i) const { return hazards.Get(i); }

    Finish* GetFinish() const { return finish; }
    olc::vf2d GetSpawnPos() const { return spawnPos; }
};
