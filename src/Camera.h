#pragma once

#include "olcPixelGameEngine.h"
#include <cmath>

// 2D камера с плавным слежением (lerp)
class Camera {
public:
    olc::vf2d position = {0.0f, 0.0f};   // текущий центр камеры (мировые координаты)
    olc::vf2d target   = {0.0f, 0.0f};   // куда мы хотим смотреть
    float     zoom     = 1.0f;
    float     smoothness = 5.0f;           // скорость lerp (выше = резче)

    Camera() = default;

    // плавное слежение с использованием экспоненциального lerp
    void Update(float dt) {
        float t = 1.0f - std::exp(-smoothness * dt);
        position = position.lerp(target, t);
    }

    // Преобразование координат: Мир -> Экран
    olc::vf2d WorldToScreen(olc::vf2d worldPos, olc::vi2d screenSize) const {
        return (worldPos - position) * zoom
               + olc::vf2d((float)screenSize.x * 0.5f, (float)screenSize.y * 0.5f);
    }

    // Преобразование координат: Экран -> Мир
    olc::vf2d ScreenToWorld(olc::vf2d screenPos, olc::vi2d screenSize) const {
        return (screenPos - olc::vf2d((float)screenSize.x * 0.5f, (float)screenSize.y * 0.5f))
               / zoom + position;
    }
};
