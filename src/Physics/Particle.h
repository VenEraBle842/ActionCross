#pragma once

#include "olcPixelGameEngine.h"

// точечная масса по Верле
struct Particle {
    olc::vf2d pos;                        // текущая позиция
    olc::vf2d oldPos;                     // предыдущая позиция (для Верле)
    olc::vf2d acceleration;               // накопленное ускорение в этом кадре
    float     mass          = 1.0f;
    float     inverseMass   = 1.0f;       // 1/масса (0 если закреплена)
    float     radius        = 0.0f;       // радиус коллизии (0 = нет коллизий)
    float     angle         = 0.0f;       // угол поворота (для колес)
    float     angularVel    = 0.0f;       // угловая скорость (рад/с)
    bool      pinned        = false;      // неподвижна?

    // Информация о контакте с землей (задается PhysicsWorld на каждом шаге)
    bool      grounded       = false;     // касается поверхности в этом кадре?
    olc::vf2d groundNormal   = {0, -1};   // нормаль поверхности (направлена от земли)
    olc::vf2d groundTangent  = {1,  0};   // касательная поверхности (вдоль поверхности)

    Particle() = default;

    Particle(olc::vf2d position, float m, float r = 0.0f)
        : pos(position), oldPos(position), acceleration({0, 0}),
          mass(m), inverseMass(m > 0 ? 1.0f / m : 0.0f), radius(r) {}

    // Применение силы (F = ma => a = F/m)
    void ApplyForce(olc::vf2d force) {
        if (pinned) return;
        acceleration += force * inverseMass;
    }

    // Интегрирование Верле
    //  newPos = 2*pos - oldPos + acc*dt^2
    //  с затуханием (damping) для предотвращения накопления энергии
    void Integrate(float dt, float damping = 0.999f) {
        if (pinned) return;

        olc::vf2d velocity = (pos - oldPos) * damping;
        olc::vf2d newPos = pos + velocity + acceleration * (dt * dt);

        oldPos = pos;
        pos    = newPos;

        // Обновление поворота колеса на основе угловой скорости
        if (radius > 0.01f) {
            if (grounded) {
                // На земле: синхронизируем визуальное вращение с реальной скоростью
                // чтобы ведомое колесо мгновенно отражало направление движения
                float tangentSpeed = velocity.dot(groundTangent);
                angularVel = tangentSpeed / (radius * dt);
            } else {
                // В воздухе: постепенно замедляем вращение
                angularVel *= 0.995f;
            }
            angle += angularVel * dt;
        }

        acceleration = {0.0f, 0.0f};

        // Сброс контакта с землей для следующего кадра (будет задано коллизиями)
        grounded = false;
    }

    // Получение неявной скорости
    olc::vf2d GetVelocity() const {
        return pos - oldPos;
    }

    // Установка скорости (путем изменения oldPos)
    void SetVelocity(olc::vf2d vel) {
        oldPos = pos - vel;
    }
};
