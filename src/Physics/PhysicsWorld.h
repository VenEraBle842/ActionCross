#pragma once

#include "Physics/Constraint.h"
#include "Physics/SpatialHash.h"
#include "Level/Level.h"
#include <cmath>

// Оркестратор физического движка
class PhysicsWorld {
    MutableArraySequence<Particle*> particles;
    MutableArraySequence<Constraint*> constraints;

    olc::vf2d gravity = {0.0f, 980.0f};
    float globalDamping = 0.999f;
    float groundFriction = 0.35f;
    float restitution = 0.05f; // Упругость при столкновении
    int constraintIter = 8;    // Чем больше итераций, тем стабильнее и жестче связи

    Level* level = nullptr;
    SpatialHash spatialHash;

    // Поиск ближайших сегментов земли через пространственный хеш
    void ResolveCollisions(Particle* p, float dt) {
        auto candidates = spatialHash.Query(p->GetPos(), p->GetRadius() + 5.0f);

        for (int ci = 0; ci < candidates.GetLength(); ci++) {
            int segIdx = candidates.Get(ci);
            if (segIdx < 0 || segIdx >= level->GetSegmentsCount()) continue;
            auto seg = level->GetSegment(segIdx);
            CollideCircleSegment(p, seg.a, seg.b, dt);
        }
    }

    // Обработка коллизии: окружность (колесо) с отрезком (землей)
    void CollideCircleSegment(Particle* p, olc::vf2d a, olc::vf2d b, float dt) {
        olc::vf2d ab = b - a;
        float abLen2 = ab.mag2();
        if (abLen2 < 1e-6f) return;
        float abLen = std::sqrt(abLen2);

        float t = std::clamp((p->GetPos() - a).dot(ab) / abLen2, 0.0f, 1.0f);
        olc::vf2d closest = a + ab * t;

        olc::vf2d diff = p->GetPos() - closest;
        float dist = diff.mag();

        if (dist < p->GetRadius() && dist > 1e-6f) {
            float penetration = p->GetRadius() - dist;
            olc::vf2d normal = diff / dist;

            // Выталкивание (Penetration Resolution)
            p->AddPos(normal * penetration);

            olc::vf2d tangent = ab / abLen;
            p->SetGrounded(true);
            p->SetGroundNormal(normal);
            p->SetGroundTangent(tangent);

            olc::vf2d velocity = p->GetVelocity();
            float vn = velocity.dot(normal);

            // Обработка трения и отскока, если частица движется в поверхность
            if (vn < 0) {
                olc::vf2d vnVec = normal * vn;
                olc::vf2d vtVec = velocity - vnVec;
                float vtMag = vtVec.mag();

                float normalForceMag = std::abs(vn);
                float maxFriction = normalForceMag * groundFriction;

                olc::vf2d newVt = vtVec;
                if (vtMag > 1e-6f) {
                    if (maxFriction >= vtMag) {
                        newVt = {0.0f, 0.0f}; // Статическое трение
                    } else {
                        newVt = vtVec * ((vtMag - maxFriction) / vtMag); // Динамическое трение
                    }
                }

                olc::vf2d newVel = newVt - vnVec * restitution;
                p->SetVelocity(newVel);

                if (p->GetRadius() > 0.01f) {
                    float tangentSpeed = newVel.dot(tangent);
                    p->SetAngularVel(tangentSpeed / (p->GetRadius() * dt));
                }
            }

            // Компенсация гравитации (анти-сползание)
            float gravTangent = gravity.dot(tangent) * p->GetMass();
            float gravNormal = std::abs(gravity.dot(normal)) * p->GetMass();
            float staticFrictionLimit = gravNormal * groundFriction;

            if (std::abs(gravTangent) <= staticFrictionLimit) {
                p->ApplyForce(-tangent * gravTangent);
            }
        }
    }

public:
    PhysicsWorld() : spatialHash(64.0f) {}

    void Clear() {
        while (particles.GetLength() > 0) particles.RemoveLast();
        while (constraints.GetLength() > 0) constraints.RemoveLast();
    }

    void SetLevel(Level* lvl) {
        level = lvl;
        RebuildSpatialHash();
    }

    void RebuildSpatialHash() {
        spatialHash.Clear();
        if (!level) return;
        for (int i = 0; i < level->GetSegmentsCount(); i++) {
            auto seg = level->GetSegment(i);
            spatialHash.Insert(i, seg.a, seg.b);
        }
    }

    void AddParticle(Particle* p) { particles.Append(p); }
    void AddConstraint(Constraint* c) { constraints.Append(c); }

    void Step(float dt) {
        for (int i = 0; i < particles.GetLength(); i++) {
            particles.Get(i)->ApplyForce(gravity * particles.Get(i)->GetMass());
        }

        for (int i = 0; i < particles.GetLength(); i++) {
            particles.Get(i)->Integrate(dt, globalDamping);
        }

        for (int iter = 0; iter < constraintIter; iter++) {
            for (int i = 0; i < constraints.GetLength(); i++) {
                constraints.Get(i)->Solve();
            }
        }

        if (level) {
            for (int i = 0; i < particles.GetLength(); i++) {
                Particle* p = particles.Get(i);
                if (p->GetRadius() < 0.01f) continue;
                ResolveCollisions(p, dt);
            }
        }
    }

    void SetGravity(olc::vf2d g) { gravity = g; }
    void SetIterations(int iters) { constraintIter = iters; }
    void SetFriction(float friction) { groundFriction = friction; }
};
