#pragma once

#include "Particle.h"

// Физическое ограничение (пружина), связывающее две частицы
class Constraint {
    Particle* a = nullptr;
    Particle* b = nullptr;
    float restLength = 0.0f; // Длина в состоянии покоя
    float stiffness = 1.0f;  // Жесткость пружины [0.0 - 1.0]
    float damping = 0.0f;    // Гашение колебаний

public:
    Constraint() = default;

    Constraint(Particle* pa, Particle* pb, float stiff = 1.0f, float damp = 0.0f)
        : a(pa), b(pb), stiffness(stiff), damping(damp)
    {
        olc::vf2d delta = pb->GetPos() - pa->GetPos();
        restLength = delta.mag();
    }

    // Релаксация связи: притягивает или расталкивает частицы для достижения restLength
    void Solve() {
        olc::vf2d delta = b->GetPos() - a->GetPos();
        float dist = delta.mag();
        if (dist < 1e-6f) return;

        float diff = (dist - restLength) / dist;
        olc::vf2d correction = delta * diff * stiffness;

        float totalInvMass = a->GetInverseMass() + b->GetInverseMass();
        if (totalInvMass < 1e-6f) return;

        // Распределение коррекции обратно пропорционально массе
        float ratioA = a->GetInverseMass() / totalInvMass;
        float ratioB = b->GetInverseMass() / totalInvMass;

        if (!a->IsPinned()) a->AddPos(correction * ratioA);
        if (!b->IsPinned()) b->SubPos(correction * ratioB);

        // Гашение относительной скорости вдоль оси пружины (Damping)
        if (damping > 0.0f) {
            olc::vf2d relVel = b->GetVelocity() - a->GetVelocity();
            olc::vf2d axis = delta / dist;
            float velAlongAxis = relVel.dot(axis);
            olc::vf2d dampingImpulse = axis * velAlongAxis * damping;

            if (!a->IsPinned()) a->SubOldPos(dampingImpulse * ratioA);
            if (!b->IsPinned()) b->AddOldPos(dampingImpulse * ratioB);
        }
    }

    Particle* GetParticleA() const { return a; }
    Particle* GetParticleB() const { return b; }
};
