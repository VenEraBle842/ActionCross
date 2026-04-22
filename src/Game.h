#pragma once

#include "olcPixelGameEngine.h"
#include "GameState.h"
#include "Physics/PhysicsWorld.h"
#include "Bike/Bike.h"
#include "Level/Level.h"
#include "Level/LevelLoader.h"
#include "Camera.h"
#include <string>
#include <cmath>

class Game : public olc::PixelGameEngine {
public:
    Game() {
        sAppName = "Action Cross";
    }

private:
    // Основные системы
    GameState    state = GameState::MENU;
    PhysicsWorld physics;
    Bike         bike;
    Level        level;
    Camera       camera;

    // Фиксированный шаг времени
    static constexpr float PHYSICS_DT   = 1.0f / 120.0f;
    static constexpr float MAX_FRAME_DT = 0.05f; // ограничение в 50мс
    float accumulator = 0.0f;

    // Игровой таймер
    float gameTime = 0.0f;

    // Цвета
    const olc::Pixel SKY_TOP       = olc::Pixel(100, 150, 220);
    const olc::Pixel SKY_BOTTOM    = olc::Pixel(180, 210, 240);
    const olc::Pixel GROUND_COLOR  = olc::Pixel(80, 60, 40);
    const olc::Pixel GROUND_EDGE   = olc::Pixel(50, 120, 50);
    const olc::Pixel WHEEL_COLOR   = olc::Pixel(40, 40, 40);
    const olc::Pixel FRAME_COLOR   = olc::Pixel(200, 50, 50);
    const olc::Pixel BODY_COLOR    = olc::Pixel(50, 50, 180);
    const olc::Pixel HEAD_COLOR    = olc::Pixel(255, 200, 150);
    const olc::Pixel APPLE_COLOR   = olc::Pixel(220, 30, 30);
    const olc::Pixel FINISH_COLOR  = olc::Pixel(255, 255, 100);
    const olc::Pixel HAZARD_COLOR  = olc::Pixel(200, 200, 200);

public:
    bool OnUserCreate() override {
        InitGame();
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        // Ограничиваем время кадра для предотвращения "спирали смерти"
        float dt = std::min(fElapsedTime, MAX_FRAME_DT);

        switch (state) {
            case GameState::MENU:
                UpdateMenu(dt);
                break;
            case GameState::PLAYING:
                UpdatePlaying(dt);
                break;
            case GameState::CRASHED:
                UpdateCrashed(dt);
                break;
            case GameState::LEVEL_CLEARED:
                UpdateLevelCleared(dt);
                break;
        }

        return true;
    }

private:
    // Инициализация / Сброс
    void InitGame() {
        LevelLoader::CreateDefaultLevel(level);
        ResetBike();
    }

    void ResetBike() {
        // Очистка физического мира
        physics.Clear();
        physics.SetLevel(&level);

        // Сброс байка
        bike.Init(level.spawnPos);
        bike.RegisterWith(physics);

        // Сброс объектов уровня
        level.Reset();

        // Сброс камеры
        camera.position = bike.GetCenterOfMass();
        camera.target   = camera.position;

        accumulator = 0.0f;
        gameTime   = 0.0f;
    }

    //  СОСТОЯНИЕ МЕНЮ

    void UpdateMenu(float dt) {
        if (GetKey(olc::Key::ENTER).bPressed || GetKey(olc::Key::SPACE).bPressed) {
            state = GameState::PLAYING;
            ResetBike();
        }

        // Отрисовка меню
        Clear(olc::Pixel(30, 30, 50));

        std::string title = "ACTION CROSS";
        DrawString(ScreenWidth() / 2 - (int)title.size() * 4, ScreenHeight() / 3,
                   title, olc::Pixel(255, 200, 50), 2);

        int y = ScreenHeight() / 2 + 20;
        DrawString(50, y,       "Controls:", olc::Pixel(150, 200, 255));
        DrawString(50, y + 15,  "  UP / W     - Throttle", olc::WHITE);
        DrawString(50, y + 27,  "  DOWN / S   - Brake", olc::WHITE);
        DrawString(50, y + 39,  "  SPACE      - Flip direction", olc::WHITE);
        DrawString(50, y + 51,  "  R          - Restart", olc::WHITE);

        DrawString(ScreenWidth() / 2 - 100, ScreenHeight() - 60,
                   "Press ENTER or SPACE to start",
                   olc::Pixel(200, 200, 200));
    }

    //  СОСТОЯНИЕ ИГРЫ

    void UpdatePlaying(float dt) {
        HandleInput(dt);

        // Физика с фиксированным шагом
        accumulator += dt;
        while (accumulator >= PHYSICS_DT) {
            physics.Step(PHYSICS_DT);
            accumulator -= PHYSICS_DT;
        }

        // Игровая логика
        gameTime += dt;
        CheckCollectibles();
        CheckHazards();
        CheckHeadCollision();

        // Обновление активации финиша
        level.finish.active = level.AllCollected();

        // Проверка финиша
        if (level.finish.active) {
            float distToFinish = (bike.GetCenterOfMass() - level.finish.pos).mag();
            if (distToFinish < level.finish.radius + 30.0f) {
                state = GameState::LEVEL_CLEARED;
            }
        }

        if (GetKey(olc::Key::R).bPressed) {
            ResetBike();
        }

        camera.target = bike.GetCenterOfMass();
        camera.Update(dt);

        RenderWorld();
        RenderHUD();
    }

    //  СОСТОЯНИЕ АВАРИИ

    void UpdateCrashed(float dt) {
        // Продолжение симуляции физики (эффект ragdoll)
        accumulator += dt;
        while (accumulator >= PHYSICS_DT) {
            physics.Step(PHYSICS_DT);
            accumulator -= PHYSICS_DT;
        }
        camera.target = bike.GetCenterOfMass();
        camera.Update(dt);

        if (GetKey(olc::Key::R).bPressed || GetKey(olc::Key::ENTER).bPressed) {
            state = GameState::PLAYING;
            ResetBike();
        }
        if (GetKey(olc::Key::ESCAPE).bPressed) {
            state = GameState::MENU;
        }

        RenderWorld();

        // Затемнение экрана
        FillRect(0, 0, ScreenWidth(), ScreenHeight(), olc::Pixel(0, 0, 0, 120));
        DrawString(ScreenWidth() / 2 - 40, ScreenHeight() / 2 - 20,
                   "CRASHED!", olc::Pixel(255, 80, 80), 2);
        DrawString(ScreenWidth() / 2 - 100, ScreenHeight() / 2 + 20,
                   "Press R or ENTER to restart", olc::WHITE);
        DrawString(ScreenWidth() / 2 - 80, ScreenHeight() / 2 + 35,
                   "Press ESC for menu", olc::GREY);
    }

    //  СОСТОЯНИЕ ПРОЙДЕННОГО УРОВНЯ

    void UpdateLevelCleared(float dt) {
        camera.Update(dt);

        if (GetKey(olc::Key::R).bPressed || GetKey(olc::Key::ENTER).bPressed) {
            state = GameState::PLAYING;
            ResetBike();
        }
        if (GetKey(olc::Key::ESCAPE).bPressed) {
            state = GameState::MENU;
        }

        RenderWorld();

        FillRect(0, 0, ScreenWidth(), ScreenHeight(), olc::Pixel(0, 0, 0, 120));
        DrawString(ScreenWidth() / 2 - 80, ScreenHeight() / 2 - 20,
                   "LEVEL CLEARED!", olc::Pixel(100, 255, 100), 2);

        // Показ времени
        int secs = (int)gameTime;
        int ms   = (int)((gameTime - secs) * 100);
        std::string timeStr = "Time: " + std::to_string(secs) + "." +
                              (ms < 10 ? "0" : "") + std::to_string(ms) + "s";
        DrawString(ScreenWidth() / 2 - (int)timeStr.size() * 4, ScreenHeight() / 2 + 20,
                   timeStr, olc::YELLOW);

        DrawString(ScreenWidth() / 2 - 100, ScreenHeight() / 2 + 45,
                   "Press R or ENTER to replay", olc::WHITE);
    }

    //  ОБРАБОТКА ВВОДА

    void HandleInput(float dt) {
        // Газ
        if (GetKey(olc::Key::UP).bHeld || GetKey(olc::Key::W).bHeld) {
            bike.Throttle(dt);
        }

        // Тормоз
        if (GetKey(olc::Key::DOWN).bHeld || GetKey(olc::Key::S).bHeld) {
            bike.Brake();
        }



        // Смена направления
        if (GetKey(olc::Key::SPACE).bPressed) {
            bike.FlipDirection();
        }
    }

    //  ПРОВЕРКИ ИГРОВОЙ ЛОГИКИ

    void CheckCollectibles() {
        for (int i = 0; i < level.collectibles.GetLength(); i++) {
            if (level.collectibles[i].collected) continue;

            // Проверка дистанции до любого узла байка
            olc::vf2d cpos = level.collectibles[i].pos;
            float collectRadius = level.collectibles[i].radius + 25.0f;

            float distFront = (bike.frontWheel.pos - cpos).mag();
            float distRear  = (bike.rearWheel.pos  - cpos).mag();
            float distBody  = (bike.body.pos        - cpos).mag();

            if (distFront < collectRadius || distRear < collectRadius || distBody < collectRadius) {
                level.collectibles[i].collected = true;
            }
        }
    }

    void CheckHazards() {
        for (int i = 0; i < level.hazards.GetLength(); i++) {
            auto& h = level.hazards.Get(i);

            float distFront = (bike.frontWheel.pos - h.pos).mag();
            float distRear  = (bike.rearWheel.pos  - h.pos).mag();
            float distBody  = (bike.body.pos        - h.pos).mag();
            float distHead  = (bike.head.pos         - h.pos).mag();

            float hazardRadius = h.radius + 20.0f;
            if (distFront < hazardRadius || distRear < hazardRadius ||
                distBody < hazardRadius  || distHead < hazardRadius) {
                state = GameState::CRASHED;
                return;
            }
        }
    }

    void CheckHeadCollision() {
        if (bike.IsHeadGrounded(level)) {
            state = GameState::CRASHED;
        }
    }

    //  ОТРИСОВКА

    void RenderWorld() {
        olc::vi2d screen = {ScreenWidth(), ScreenHeight()};

        // Градиент неба
        for (int y = 0; y < ScreenHeight(); y++) {
            float t = (float)y / (float)ScreenHeight();
            olc::Pixel col = olc::PixelLerp(SKY_TOP, SKY_BOTTOM, t);
            DrawLine(0, y, ScreenWidth() - 1, y, col);
        }

        // Земля (залитый полигон через треугольники)
        RenderGround(screen);

        // Предметы для сбора (яблоки)
        RenderCollectibles(screen);

        // Опасности (шипы)
        RenderHazards(screen);

        RenderFinish(screen);

        RenderBike(screen);
    }

    void RenderGround(olc::vi2d screen) {
        // Рисуем залитую землю: для каждого отрезка рисуем четырехугольник вниз до экрана
        for (int i = 0; i < level.segments.GetLength(); i++) {
            auto& seg = level.segments.Get(i);
            olc::vf2d sa = camera.WorldToScreen(seg.a, screen);
            olc::vf2d sb = camera.WorldToScreen(seg.b, screen);

            // Заливка от отрезка вниз до низа экрана
            olc::vf2d bottomA = camera.WorldToScreen(
                {seg.a.x, seg.a.y + 400.0f}, screen);
            olc::vf2d bottomB = camera.WorldToScreen(
                {seg.b.x, seg.b.y + 400.0f}, screen);

            FillTriangle((int32_t)sa.x, (int32_t)sa.y,
                         (int32_t)sb.x, (int32_t)sb.y,
                         (int32_t)bottomA.x, (int32_t)bottomA.y, GROUND_COLOR);
            FillTriangle((int32_t)sb.x, (int32_t)sb.y,
                         (int32_t)bottomB.x, (int32_t)bottomB.y,
                         (int32_t)bottomA.x, (int32_t)bottomA.y, GROUND_COLOR);

            // Рисуем край поверхности (линия травы)
            DrawLine((int32_t)sa.x, (int32_t)sa.y,
                     (int32_t)sb.x, (int32_t)sb.y, GROUND_EDGE);
        }
    }

    void RenderCollectibles(olc::vi2d screen) {
        for (int i = 0; i < level.collectibles.GetLength(); i++) {
            auto& c = level.collectibles.Get(i);
            if (c.collected) continue;

            olc::vf2d sp = camera.WorldToScreen(c.pos, screen);

            // Яблоко: красный круг с зеленым черенком
            FillCircle((int32_t)sp.x, (int32_t)sp.y, (int32_t)c.radius, APPLE_COLOR);
            DrawCircle((int32_t)sp.x, (int32_t)sp.y, (int32_t)c.radius,
                       olc::Pixel(180, 20, 20));
            // Черенок
            DrawLine((int32_t)sp.x, (int32_t)sp.y - (int32_t)c.radius,
                     (int32_t)sp.x + 3, (int32_t)sp.y - (int32_t)c.radius - 5,
                     olc::Pixel(50, 150, 50));
            // Лист
            FillCircle((int32_t)sp.x + 4, (int32_t)sp.y - (int32_t)c.radius - 3,
                       3, olc::Pixel(50, 180, 50));
        }
    }

    void RenderHazards(olc::vi2d screen) {
        for (int i = 0; i < level.hazards.GetLength(); i++) {
            auto& h = level.hazards.Get(i);
            olc::vf2d sp = camera.WorldToScreen(h.pos, screen);

            // Шип: треугольник, указывающий вверх
            int r = (int)h.radius;
            for (int j = 0; j < 5; j++) {
                float angle = (float)j * 3.14159f * 2.0f / 5.0f - 3.14159f / 2.0f;

                olc::vf2d tip = sp + olc::vf2d(std::cos(angle), std::sin(angle)) * (float)(r + 5);
                olc::vf2d base1 = sp + olc::vf2d(std::cos(angle - 0.3f), std::sin(angle - 0.3f)) * (float)(r - 3);
                olc::vf2d base2 = sp + olc::vf2d(std::cos(angle + 0.3f), std::sin(angle + 0.3f)) * (float)(r - 3);

                FillTriangle((int32_t)tip.x, (int32_t)tip.y,
                             (int32_t)base1.x, (int32_t)base1.y,
                             (int32_t)base2.x, (int32_t)base2.y, HAZARD_COLOR);
            }
            FillCircle((int32_t)sp.x, (int32_t)sp.y, r - 3, olc::Pixel(180, 180, 180));
        }
    }

    void RenderFinish(olc::vi2d screen) {
        olc::vf2d sp = camera.WorldToScreen(level.finish.pos, screen);
        olc::Pixel color = level.finish.active
            ? olc::Pixel(255, 255, 50)        // ярко-желтый когда активен
            : olc::Pixel(100, 100, 80);       // тусклый когда заблокирован

        // Форма цветка: лепестки + центр
        int r = (int)level.finish.radius;
        for (int j = 0; j < 6; j++) {
            float angle = (float)j * 3.14159f / 3.0f;
            olc::vf2d petalPos = sp + olc::vf2d(std::cos(angle), std::sin(angle)) * (float)(r - 2);
            FillCircle((int32_t)petalPos.x, (int32_t)petalPos.y, r / 3, color);
        }
        FillCircle((int32_t)sp.x, (int32_t)sp.y, r / 3 + 1,
                   level.finish.active ? olc::Pixel(255, 200, 50) : olc::Pixel(80, 80, 60));
    }

    void RenderBike(olc::vi2d screen) {
        olc::vf2d fw = camera.WorldToScreen(bike.frontWheel.pos, screen);
        olc::vf2d rw = camera.WorldToScreen(bike.rearWheel.pos, screen);
        olc::vf2d bd = camera.WorldToScreen(bike.body.pos, screen);
        olc::vf2d hd = camera.WorldToScreen(bike.head.pos, screen);

        // Рама (линии между узлами)
        DrawLine((int32_t)fw.x, (int32_t)fw.y, (int32_t)rw.x, (int32_t)rw.y, FRAME_COLOR);
        DrawLine((int32_t)fw.x, (int32_t)fw.y, (int32_t)bd.x, (int32_t)bd.y, FRAME_COLOR);
        DrawLine((int32_t)rw.x, (int32_t)rw.y, (int32_t)bd.x, (int32_t)bd.y, FRAME_COLOR);
        DrawLine((int32_t)bd.x, (int32_t)bd.y, (int32_t)hd.x, (int32_t)hd.y, BODY_COLOR);

        // Колеса (круги с линией спиц для видимости вращения)
        int wheelR = (int)(bike.frontWheel.radius * camera.zoom);

        // Переднее колесо
        DrawCircle((int32_t)fw.x, (int32_t)fw.y, wheelR, WHEEL_COLOR);
        FillCircle((int32_t)fw.x, (int32_t)fw.y, wheelR - 2, olc::Pixel(60, 60, 60));
        // Спицы
        for (int s = 0; s < 4; s++) {
            float a = bike.frontWheel.angle + (float)s * 3.14159f / 2.0f;
            olc::vf2d spoke = fw + olc::vf2d(std::cos(a), std::sin(a)) * (float)(wheelR - 2);
            DrawLine((int32_t)fw.x, (int32_t)fw.y, (int32_t)spoke.x, (int32_t)spoke.y,
                     olc::Pixel(120, 120, 120));
        }

        // Заднее колесо
        DrawCircle((int32_t)rw.x, (int32_t)rw.y, wheelR, WHEEL_COLOR);
        FillCircle((int32_t)rw.x, (int32_t)rw.y, wheelR - 2, olc::Pixel(60, 60, 60));
        for (int s = 0; s < 4; s++) {
            float a = bike.rearWheel.angle + (float)s * 3.14159f / 2.0f;
            olc::vf2d spoke = rw + olc::vf2d(std::cos(a), std::sin(a)) * (float)(wheelR - 2);
            DrawLine((int32_t)rw.x, (int32_t)rw.y, (int32_t)spoke.x, (int32_t)spoke.y,
                     olc::Pixel(120, 120, 120));
        }

        // Тело (торс водителя)
        FillCircle((int32_t)bd.x, (int32_t)bd.y, 5, BODY_COLOR);

        // Голова
        int headR = (int)(bike.head.radius * camera.zoom);
        FillCircle((int32_t)hd.x, (int32_t)hd.y, headR, HEAD_COLOR);
        DrawCircle((int32_t)hd.x, (int32_t)hd.y, headR, olc::Pixel(200, 160, 120));
    }

    void RenderHUD() {
        // Фоновая полоса
        FillRect(0, 0, ScreenWidth(), 22, olc::Pixel(0, 0, 0, 150));

        // Собранные яблоки
        int total = level.collectibles.GetLength();
        int collected = 0;
        for (int i = 0; i < total; i++) {
            if (level.collectibles.Get(i).collected) collected++;
        }
        DrawString(5, 5, "Apples: " + std::to_string(collected) + "/" + std::to_string(total),
                   collected == total ? olc::Pixel(100, 255, 100) : olc::WHITE);

        // Таймер
        int secs = (int)gameTime;
        int ms   = (int)((gameTime - secs) * 100);
        std::string timeStr = std::to_string(secs) + "." + (ms < 10 ? "0" : "") + std::to_string(ms) + "s";
        DrawString(ScreenWidth() - 80, 5, timeStr, olc::YELLOW);

        // Индикатор направления
        std::string dirStr = bike.direction > 0 ? ">>>" : "<<<";
        DrawString(ScreenWidth() / 2 - 12, 5, dirStr, olc::Pixel(150, 200, 255));
    }
};
