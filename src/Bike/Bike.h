#pragma once

#include "Physics/Constraint.h"
#include "Physics/PhysicsWorld.h"

// Soft-body модель мотоцикла, состоящая из 4 частиц и 6 связей (пружин)
class Bike {
    Particle frontWheel;
    Particle rearWheel;
    Particle body;
    Particle head;

    Constraint frame;
    Constraint frontFork;
    Constraint swingArm;
    Constraint neck;
    Constraint crossA;
    Constraint crossB;

    float direction = 1.0f;
    float throttleForce = 7500.0f;
    float brakeStrength = 0.90f;
    float airThrottleSpin = 15.0f;

public:
    void Init(olc::vf2d spawnPos) {
        float wheelRadius = 18.0f;
        float headRadius = 8.0f;
        float wheelMass = 1.8f;
        float bodyMass = 1.0f;
        float headMass = 0.5f;

        direction = 1.0f;

        frontWheel = Particle(spawnPos + olc::vf2d(40.0f * direction, 0.0f), wheelMass, wheelRadius);
        rearWheel = Particle(spawnPos + olc::vf2d(-40.0f * direction, 0.0f), wheelMass, wheelRadius);
        body = Particle(spawnPos + olc::vf2d(0.0f, -30.0f), bodyMass, 0.0f);
        head = Particle(spawnPos + olc::vf2d(5.0f * direction, -55.0f), headMass, headRadius);

        frame = Constraint(&frontWheel, &rearWheel, 0.9f, 0.05f);
        frontFork = Constraint(&frontWheel, &body, 0.7f, 0.1f);
        swingArm = Constraint(&rearWheel, &body, 0.7f, 0.1f);
        neck = Constraint(&body, &head, 0.5f, 0.15f);
        crossA = Constraint(&frontWheel, &head, 0.3f, 0.05f);
        crossB = Constraint(&rearWheel, &head, 0.3f, 0.05f);
    }

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

    void Throttle(float dt) {
        if (rearWheel.IsGrounded()) {
            olc::vf2d driveDir = rearWheel.GetGroundTangent();
            olc::vf2d bikeAxis = (frontWheel.GetPos() - rearWheel.GetPos());

            if (bikeAxis.dot(driveDir) < 0) {
                driveDir = -driveDir;
            }
            driveDir = driveDir * direction;

            olc::vf2d force = driveDir * throttleForce;
            rearWheel.ApplyForce(force);
            frontWheel.ApplyForce(-force * 0.2f); // Эффект wheelie (поднятие носа)
        } else {
            rearWheel.AddAngularVel(direction * airThrottleSpin * dt); // Вращение в полете
        }
    }

    void Brake() {
        if (rearWheel.IsGrounded() || frontWheel.IsGrounded()) {
            olc::vf2d rearVel = rearWheel.GetVelocity();
            olc::vf2d frontVel = frontWheel.GetVelocity();
            rearWheel.SetVelocity(rearVel * brakeStrength);
            frontWheel.SetVelocity(frontVel * brakeStrength);
        }
        rearWheel.MultAngularVel(brakeStrength);
        frontWheel.MultAngularVel(brakeStrength);
    }

    void FlipDirection() {
        direction *= -1.0f;
    }

    void Jump(float impulse) {
        // Запись в oldPos создает мгновенную дельту, которая на следующем шаге превратится в скорость
        frontWheel.AddOldPos({0.0f, impulse});
        rearWheel.AddOldPos({0.0f, impulse});
        body.AddOldPos({0.0f, impulse});
    }

    olc::vf2d GetCenterOfMass() const {
        float totalMass = frontWheel.GetMass() + rearWheel.GetMass() + body.GetMass() + head.GetMass();
        return (frontWheel.GetPos() * frontWheel.GetMass() +
                rearWheel.GetPos() * rearWheel.GetMass() +
                body.GetPos() * body.GetMass() +
                head.GetPos() * head.GetMass()) / totalMass;
    }

    float GetAngle() const {
        olc::vf2d diff = frontWheel.GetPos() - rearWheel.GetPos();
        return std::atan2(diff.y, diff.x);
    }

    // Проверка коллизии головы с геометрией. Голова не является solid-объектом в движке.
    bool IsHeadGrounded() const {
        return head.IsGrounded();
    }

    // Проверка дистанции от всех узлов байка до целевой точки
    bool IsCloseTo(olc::vf2d targetPos, float threshold) const {
        float thresholdSq = threshold * threshold;
        if ((frontWheel.GetPos() - targetPos).mag2() < thresholdSq) return true;
        if ((rearWheel.GetPos() - targetPos).mag2() < thresholdSq) return true;
        if ((body.GetPos() - targetPos).mag2() < thresholdSq) return true;
        if ((head.GetPos() - targetPos).mag2() < thresholdSq) return true;
        return false;
    }

    const Particle& GetFrontWheel() const { return frontWheel; }
    const Particle& GetRearWheel() const { return rearWheel; }
    const Particle& GetBody() const { return body; }
    const Particle& GetHead() const { return head; }
    float GetDirection() const { return direction; }
};
