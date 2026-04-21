#pragma once

#include "Physics/Particle.h"
#include "Physics/Constraint.h"
#include "Physics/PhysicsWorld.h"
#include "olcPixelGameEngine.h"
#include <cmath>

// мотоцикл из 4 частиц + 6 ограничений
//
//  Узел 0: Переднее колесо (масса=3, радиус=18)
//  Узел 1: Заднее колесо   (масса=3, радиус=18)
//  Узел 2: Корпус/шасси    (масса=1, радиус=0  — нет коллизий, только инерция)
//  Узел 3: Голова водителя (масса=0.5, радиус=8 — КРИТИЧНО: касание земли = смерть)
//
//  Ограничения:
//    0-1  рама               (жесткость=0.9)
//    0-2  передняя вилка     (жесткость=0.7)
//    1-2  маятник            (жесткость=0.7)
//    2-3  шея                (жесткость=0.5)
//    0-3  распорка А         (жесткость=0.3)
//    1-3  распорка Б         (жесткость=0.3)

class Bike {
public:
    // Частицы (принадлежат Bike, регистрируются в PhysicsWorld)
    Particle frontWheel;
    Particle rearWheel;
    Particle body;
    Particle head;

    // Ограничения
    Constraint frame;       // 0-1
    Constraint frontFork;   // 0-2
    Constraint swingArm;    // 1-2
    Constraint neck;        // 2-3
    Constraint crossA;      // 0-3
    Constraint crossB;      // 1-3

    // Направление: +1 = смотрит вправо, -1 = смотрит влево
    float direction = 1.0f;

    // Настройки параметров
    float throttleForce   = 7500.0f;
    float brakeStrength   = 0.90f;
    float rotationForce   = 3500.0f;

    // Инициализация мотоцикла на позиции спавна
    void Init(olc::vf2d spawnPos) {
        float wheelRadius = 18.0f;
        float headRadius  = 8.0f;
        float wheelMass   = 1.8f;
        float bodyMass    = 1.0f;
        float headMass    = 0.5f;

        // Позиционируем узлы относительно точки спавна
        frontWheel = Particle(spawnPos + olc::vf2d(40.0f * direction, 0.0f),
                              wheelMass, wheelRadius);
        rearWheel  = Particle(spawnPos + olc::vf2d(-40.0f * direction, 0.0f),
                              wheelMass, wheelRadius);
        body       = Particle(spawnPos + olc::vf2d(0.0f, -30.0f),
                              bodyMass, 0.0f); // без коллизий
        head       = Particle(spawnPos + olc::vf2d(5.0f * direction, -55.0f),
                              headMass, headRadius);

        direction = 1.0f;

        // Создаем ограничения с измеренными длинами покоя
        frame     = Constraint(&frontWheel, &rearWheel, 0.9f, 0.05f);
        frontFork = Constraint(&frontWheel, &body,      0.7f, 0.1f);
        swingArm  = Constraint(&rearWheel,  &body,      0.7f, 0.1f);
        neck      = Constraint(&body,       &head,      0.5f, 0.15f);
        crossA    = Constraint(&frontWheel, &head,       0.3f, 0.05f);
        crossB    = Constraint(&rearWheel,  &head,       0.3f, 0.05f);
    }

    // Регистрация в физическом мире
    void RegisterWith(PhysicsWorld& world) {
        world.AddParticle(&frontWheel);
        world.AddParticle(&rearWheel);
        world.AddParticle(&body);
        world.AddParticle(&head);

        world.AddConstraint(&frame);
        world.AddConstraint(&frontFork);
        world.AddConstraint(&swingArm);
        world.AddConstraint(&neck);
        world.AddConstraint(&crossA);
        world.AddConstraint(&crossB);
    }

    // Газ: толкаем заднее колесо вдоль поверхности земли
    void Throttle(float dt) {
        olc::vf2d driveDir;

        if (rearWheel.grounded) {
            // Едем по направлению касательной к реальной поверхности земли
            driveDir = rearWheel.groundTangent;
            // Убеждаемся, что направление движения совпадает с направлением мотоцикла
            olc::vf2d bikeAxis = (frontWheel.pos - rearWheel.pos);
            if (bikeAxis.dot(driveDir) < 0) {
                driveDir = -driveDir;  // разворачиваем касательную для совпадения с ориентацией байка
            }
            driveDir = driveDir * direction;
        } else {
            // В воздухе: используем ось рамы мотоцикла
            driveDir = (frontWheel.pos - rearWheel.pos).norm() * direction;
        }

        olc::vf2d force = driveDir * throttleForce;
        rearWheel.ApplyForce(force);
        // Противодействующая сила на переднее колесо (3-й закон Ньютона, уменьшенная)
        frontWheel.ApplyForce(-force * 0.2f);

        // Угловая скорость для визуализации вращения колеса
        rearWheel.angularVel += throttleForce * dt / rearWheel.radius * direction;
    }

    // Тормоз: гасим скорость колес
    void Brake() {
        olc::vf2d rearVel  = rearWheel.GetVelocity();
        olc::vf2d frontVel = frontWheel.GetVelocity();
        rearWheel.SetVelocity(rearVel * brakeStrength);
        frontWheel.SetVelocity(frontVel * brakeStrength);
        rearWheel.angularVel  *= brakeStrength;
        frontWheel.angularVel *= brakeStrength;
    }

    // Наклон: создаем вращающий момент через пару сил
    void Lean(float dir, float dt) {
        olc::vf2d bikeAxis = (frontWheel.pos - rearWheel.pos);
        olc::vf2d perp = bikeAxis.perp().norm();

        // Основной крутящий момент: тело и голова толкаются по перпендикуляру
        body.ApplyForce(perp * rotationForce * dir);
        head.ApplyForce(perp * rotationForce * dir * 0.7f);

        // Контр-момент на колеса (пара сил = чистый крутящий момент)
        frontWheel.ApplyForce(-perp * rotationForce * dir * 0.45f);
        rearWheel.ApplyForce(-perp * rotationForce * dir * 0.45f);
    }

    // Смена направления
    void FlipDirection() {
        direction *= -1.0f;
    }

    // Получить центр масс (для цели камеры)
    olc::vf2d GetCenterOfMass() const {
        float totalMass = frontWheel.mass + rearWheel.mass + body.mass + head.mass;
        olc::vf2d com = (frontWheel.pos * frontWheel.mass
                       + rearWheel.pos  * rearWheel.mass
                       + body.pos       * body.mass
                       + head.pos       * head.mass) / totalMass;
        return com;
    }

    // Получить угол байка (для рендеринга ориентации)
    float GetAngle() const {
        olc::vf2d diff = frontWheel.pos - rearWheel.pos;
        return std::atan2(diff.y, diff.x);
    }

    // Проверить, коснулась ли голова земли (= смерть)
    bool IsHeadGrounded(const Level& level) const {
        for (int i = 0; i < level.segments.GetLength(); i++) {
            auto& seg = level.segments.Get(i);

            olc::vf2d ab = seg.b - seg.a;
            float abLen2 = ab.mag2();
            if (abLen2 < 1e-6f) continue;

            float t = std::clamp((head.pos - seg.a).dot(ab) / abLen2, 0.0f, 1.0f);
            olc::vf2d closest = seg.a + ab * t;
            float dist = (head.pos - closest).mag();

            if (dist < head.radius) return true;
        }
        return false;
    }
};
