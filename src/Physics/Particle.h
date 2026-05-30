#pragma once

#include "olcPixelGameEngine.h"

// Представляет точечную массу в физическом движке
class Particle {
    olc::vf2d pos;
    olc::vf2d oldPos;
    olc::vf2d acceleration;
    float mass = 1.0f;
    float inverseMass = 1.0f; // Кешируется для оптимизации (умножение быстрее деления)
    float radius = 0.0f;

    // Параметры для вращения (качения) колес
    float angle = 0.0f;
    float angularVel = 0.0f;

    bool pinned = false; // Зафиксирована ли частица в пространстве
    bool grounded = false;
    olc::vf2d groundNormal = {0, -1};
    olc::vf2d groundTangent = {1, 0};

public:
    Particle() = default;

    Particle(olc::vf2d position, float m, float r = 0.0f)
        : pos(position), oldPos(position), acceleration({0, 0}),
          mass(m), inverseMass(m > 0 ? 1.0f / m : 0.0f), radius(r) {}

    // Добавление силы. F = m * a => a = F * (1 / m)
    void ApplyForce(olc::vf2d force) {
        if (pinned) return;
        acceleration += force * inverseMass;
    }

    // Интеграция Верле: скорость выводится из разности текущей и старой позиции
    void Integrate(float dt, float damping = 0.999f) {
        if (pinned) return;

        olc::vf2d velocity = (pos - oldPos) * damping;
        olc::vf2d newPos = pos + velocity + acceleration * (dt * dt);

        oldPos = pos;
        pos = newPos;

        // Связываем линейную скорость с угловой для эффекта качения (если есть радиус)
        if (radius > 0.01f) {
            if (grounded) {
                float tangentSpeed = velocity.dot(groundTangent);
                angularVel = tangentSpeed / (radius * dt);
            } else {
                angularVel *= 0.995f; // Воздушное сопротивление вращению
            }
            angle += angularVel * dt;
        }

        acceleration = {0.0f, 0.0f};
        grounded = false;
    }

    olc::vf2d GetVelocity() const { return pos - oldPos; }
    void SetVelocity(olc::vf2d vel) { oldPos = pos - vel; }

    // Геттеры
    olc::vf2d GetPos() const { return pos; }
    olc::vf2d GetOldPos() const { return oldPos; }
    float GetMass() const { return mass; }
    float GetInverseMass() const { return inverseMass; }
    float GetRadius() const { return radius; }
    float GetAngle() const { return angle; }
    float GetAngularVel() const { return angularVel; }
    bool IsPinned() const { return pinned; }
    bool IsGrounded() const { return grounded; }
    olc::vf2d GetGroundTangent() const { return groundTangent; }

    // Сеттеры
    void SetPos(olc::vf2d newPos) { pos = newPos; }
    void SetOldPos(olc::vf2d newOldPos) { oldPos = newOldPos; }
    void AddPos(olc::vf2d delta) { pos += delta; }
    void SubPos(olc::vf2d delta) { pos -= delta; }
    void AddOldPos(olc::vf2d delta) { oldPos += delta; }
    void SubOldPos(olc::vf2d delta) { oldPos -= delta; }
    void SetPinned(bool p) { pinned = p; }
    void SetGrounded(bool g) { grounded = g; }
    void SetGroundNormal(olc::vf2d normal) { groundNormal = normal; }
    void SetGroundTangent(olc::vf2d tangent) { groundTangent = tangent; }
    void SetAngularVel(float av) { angularVel = av; }
    void AddAngularVel(float delta) { angularVel += delta; }
    void MultAngularVel(float factor) { angularVel *= factor; }
};
