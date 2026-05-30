#pragma once

#include "olcPixelGameEngine.h"

// Отвечает за преобразование координат и плавное слежение за объектом
class Camera {
    olc::vf2d position = {0.0f, 0.0f};
    olc::vf2d target = {0.0f, 0.0f};

    float zoom = 1.0f;
    float targetZoom = 1.0f;

    // Коэффициенты линейной интерполяции (Lerp)
    float followSpeed = 5.0f;
    float zoomSpeed = 5.0f;

public:
    Camera() = default;

    void Update(float dt) {
        // Асимптотическое приближение текущей позиции к цели
        position += (target - position) * followSpeed * dt;
        zoom += (targetZoom - zoom) * zoomSpeed * dt;

        // Защита от инверсии экрана или деления на ноль
        if (zoom < 0.1f) zoom = 0.1f;
    }

    void SetTarget(olc::vf2d t) { target = t; }
    void SetPosition(olc::vf2d p) { position = p; target = p; }

    void SetTargetZoom(float z) { targetZoom = z; }
    void SetZoom(float z) { zoom = z; targetZoom = z; }

    olc::vf2d GetPosition() const { return position; }
    float GetZoom() const { return zoom; }

    // Проекция мировых координат в экранные с учетом масштаба
    olc::vf2d WorldToScreen(olc::vf2d worldPos, olc::vf2d screenSize) const {
        olc::vf2d halfScreen = screenSize / 2.0f;
        return (worldPos - position) * zoom + halfScreen;
    }

    // Обратная проекция (из пикселей экрана в физический мир)
    olc::vf2d ScreenToWorld(olc::vf2d screenPos, olc::vf2d screenSize) const {
        olc::vf2d halfScreen = screenSize / 2.0f;
        return (screenPos - halfScreen) / zoom + position;
    }
};
