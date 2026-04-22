#pragma once

#include "Particle.h"

// пружина/стержень между двумя частицами
struct Constraint {
    Particle* a         = nullptr;
    Particle* b         = nullptr;
    float     restLength = 0.0f;         // собственная длина в состоянии покоя
    float     stiffness  = 1.0f;         // 0..1 (1 = жесткий стержень)
    float     damping    = 0.0f;         // гашение скорости вдоль оси пружины

    Constraint() = default;

    Constraint(Particle* pa, Particle* pb, float stiff = 1.0f, float damp = 0.0f)
        : a(pa), b(pb), stiffness(stiff), damping(damp)
    {
        olc::vf2d delta = pb->pos - pa->pos;
        restLength = delta.mag();
    }

    Constraint(Particle* pa, Particle* pb, float rest, float stiff, float damp)
        : a(pa), b(pb), restLength(rest), stiffness(stiff), damping(damp) {}

    // Разрешение ограничения (на основе позиций)
    // Сдвигаем/тянем частицы к restLength
    void Solve() {
        olc::vf2d delta = b->pos - a->pos;
        float dist = delta.mag();
        if (dist < 1e-6f) return;        // избегаем деления на ноль

        float diff = (dist - restLength) / dist;
        olc::vf2d correction = delta * diff * stiffness;

        float totalInvMass = a->inverseMass + b->inverseMass;
        if (totalInvMass < 1e-6f) return; // обе закреплены

        float ratioA = a->inverseMass / totalInvMass;
        float ratioB = b->inverseMass / totalInvMass;

        if (!a->pinned) a->pos += correction * ratioA;
        if (!b->pinned) b->pos -= correction * ratioB;

        // уменьшение относительной скорости вдоль оси пружины
        if (damping > 0.0f) {
            olc::vf2d relVel = b->GetVelocity() - a->GetVelocity();
            olc::vf2d axis   = delta / dist; // нормализованный вектор
            float     velAlongAxis = relVel.dot(axis);
            olc::vf2d dampingImpulse = axis * velAlongAxis * damping;

            if (!a->pinned) a->oldPos -= dampingImpulse * ratioA;
            if (!b->pinned) b->oldPos += dampingImpulse * ratioB;
        }
    }
};
