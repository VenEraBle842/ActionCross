#pragma once

#include "Physics/Particle.h"
#include "Physics/Constraint.h"
#include "Physics/SpatialHash.h"
#include "Level/Level.h"
#include "ArraySequence.h"
#include <cmath>
#include <algorithm>

// управляет всеми частицами, ограничениями, столкновениями
class PhysicsWorld {
public:
    MutableArraySequence<Particle*> particles;
    MutableArraySequence<Constraint*> constraints;

    olc::vf2d gravity       = {0.0f, 980.0f};   // пиксели/с^2
    float     globalDamping  = 0.999f;
    float     groundFriction = 0.35f;            // коэффициент кулоновского трения
    float     restitution    = 0.05f;            // упругость (низкая = прилипание к земле)
    int       constraintIter = 8;                // итерации солвера

    Level*    level = nullptr;
    SpatialHash spatialHash;

    PhysicsWorld() : spatialHash(64.0f) {}

    // Очистка всех частиц и ограничений
    void Clear() {
        while (particles.GetLength() > 0) particles.RemoveLast();
        while (constraints.GetLength() > 0) constraints.RemoveLast();
    }

    // Регистрация уровня и построение пространственного хеша
    void SetLevel(Level* lvl) {
        level = lvl;
        RebuildSpatialHash();
    }

    void RebuildSpatialHash() {
        spatialHash.Clear();
        if (!level) return;
        for (int i = 0; i < level->segments.GetLength(); i++) {
            auto& seg = level->segments.Get(i);
            spatialHash.Insert(i, seg.a, seg.b);
        }
    }

    // Добавление частицы / ограничения
    void AddParticle(Particle* p) { particles.Append(p); }
    void AddConstraint(Constraint* c) { constraints.Append(c); }

    // Основной шаг физики
    void Step(float dt) {
        // 1. Применяем гравитацию
        for (int i = 0; i < particles.GetLength(); i++) {
            Particle* p = particles.Get(i);
            p->ApplyForce(gravity * p->mass);
        }

        // 2. Интегрируем
        for (int i = 0; i < particles.GetLength(); i++) {
            particles.Get(i)->Integrate(dt, globalDamping);
        }

        // 3. Разрешаем ограничения (несколько итераций для стабильности)
        for (int iter = 0; iter < constraintIter; iter++) {
            for (int i = 0; i < constraints.GetLength(); i++) {
                constraints.Get(i)->Solve();
            }
        }

        // 4. Обнаружение и разрешение столкновений
        if (level) {
            for (int i = 0; i < particles.GetLength(); i++) {
                Particle* p = particles.Get(i);
                if (p->radius < 0.01f) continue; // нет формы для коллизий

                ResolveCollisions(p, dt);
            }
        }
    }

private:
    // Разрешение столкновений: круг против отрезков линий
    void ResolveCollisions(Particle* p, float dt) {
        // Широкая фаза: запрос к пространственному хешу
        auto candidates = spatialHash.Query(p->pos, p->radius + 5.0f);

        for (int ci = 0; ci < candidates.GetLength(); ci++) {
            int segIdx = candidates.Get(ci);
            if (segIdx < 0 || segIdx >= level->segments.GetLength()) continue;
            auto& seg = level->segments.Get(segIdx);

            CollideCircleSegment(p, seg.a, seg.b, dt);
        }
    }

    // Узкая фаза: круг против отрезка линии
    void CollideCircleSegment(Particle* p, olc::vf2d a, olc::vf2d b, float dt) {
        olc::vf2d ab = b - a;
        float abLen2 = ab.mag2();
        if (abLen2 < 1e-6f) return;
        float abLen = std::sqrt(abLen2);

        // Проецируем p на линию AB => параметр t из [0,1]
        float t = std::clamp((p->pos - a).dot(ab) / abLen2, 0.0f, 1.0f);
        olc::vf2d closest = a + ab * t;

        olc::vf2d diff = p->pos - closest;
        float dist = diff.mag();

        if (dist < p->radius && dist > 1e-6f) {
            // Проникновение
            float penetration = p->radius - dist;
            olc::vf2d normal = diff / dist; // направлена от поверхности

            // Выталкиваем частицу (полная коррекция)
            p->pos += normal * penetration;

            // Сохраняем информацию о контакте с землей
            olc::vf2d tangent = ab / abLen;
            p->grounded      = true;
            p->groundNormal   = normal;
            p->groundTangent  = tangent;

            // Реакция скорости
            olc::vf2d velocity = p->GetVelocity();
            float vn = velocity.dot(normal);

            if (vn < 0) { // двигается вглубь поверхности
                olc::vf2d vnVec = normal * vn;
                olc::vf2d vtVec = velocity - vnVec;
                float vtMag = vtVec.mag();

                // Кулоновское трение: импульс трения = коэффициент трения * |нормальный импульс|
                // Нормальный импульс, который будет применен, это |vn| (убирает скорость проникновения)
                // Сила трения противодействует тангенциальному движению пропорционально нормальной силе
                float normalForceMag = std::abs(vn);
                float maxFriction = normalForceMag * groundFriction;

                olc::vf2d newVt = vtVec;
                if (vtMag > 1e-6f) {
                    if (maxFriction >= vtMag) {
                        // Статическое трение: полностью убивает тангенциальную скорость
                        newVt = {0.0f, 0.0f};
                    } else {
                        // Кинетическое трение: уменьшает тангенциальную скорость
                        newVt = vtVec * ((vtMag - maxFriction) / vtMag);
                    }
                }

                // Применяем упругость к нормальной компоненте (отскок)
                olc::vf2d newVel = newVt - vnVec * restitution;
                p->SetVelocity(newVel);

                // Обновляем угловую скорость колеса от контакта с землей
                if (p->radius > 0.01f) {
                    float tangentSpeed = newVel.dot(tangent);
                    p->angularVel = tangentSpeed / (p->radius * dt);
                }
            }

            // Статическое трение: предотвращение скатывания под действием гравитации
            // При остановке на склоне гравитационная тангенциальная компонента вызывает
            // медленное скатывание, которое кулоновское трение в одиночку не может остановить (vn ~= 0).
            // Противодействуем этому, прикладывая прямо противоположную силу.
            float gravTangent = gravity.dot(tangent) * p->mass;
            float gravNormal  = std::abs(gravity.dot(normal)) * p->mass;
            float staticFrictionLimit = gravNormal * groundFriction;

            if (std::abs(gravTangent) <= staticFrictionLimit) {
                // Компонента гравитации на склоне находится в пределах конуса статического трения
                // Отменяем ее, чтобы частица не скользила
                p->ApplyForce(-tangent * gravTangent);
            }
        }
    }
};
