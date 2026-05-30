#pragma once

#include "Level/Level.h"
#include "Bike/Bike.h"
#include "Camera.h"
#include <string>
#include <cmath>

// Изолирует всю логику отрисовки. Ничего не знает о физике, только читает данные через константные ссылки
class GameRenderer {
    const olc::Pixel SKY_TOP = olc::Pixel(100, 150, 220);
    const olc::Pixel SKY_BOTTOM = olc::Pixel(180, 210, 240);
    const olc::Pixel GROUND_COLOR = olc::Pixel(80, 60, 40);
    const olc::Pixel GROUND_EDGE = olc::Pixel(50, 120, 50);
    const olc::Pixel WHEEL_COLOR = olc::Pixel(40, 40, 40);
    const olc::Pixel FRAME_COLOR = olc::Pixel(200, 50, 50);
    const olc::Pixel BODY_COLOR = olc::Pixel(50, 50, 180);
    const olc::Pixel HEAD_COLOR = olc::Pixel(255, 200, 150);
    const olc::Pixel APPLE_COLOR = olc::Pixel(220, 30, 30);
    const olc::Pixel HAZARD_COLOR = olc::Pixel(200, 200, 200);

public:
    void RenderSky(olc::PixelGameEngine* pge) {
        for (int y = 0; y < pge->ScreenHeight(); y++) {
            float t = static_cast<float>(y) / static_cast<float>(pge->ScreenHeight());
            olc::Pixel col = olc::PixelLerp(SKY_TOP, SKY_BOTTOM, t);
            pge->DrawLine(0, y, pge->ScreenWidth() - 1, y, col);
        }
    }

    void RenderGround(olc::PixelGameEngine* pge, const Level& level, const Camera& camera) {
        olc::vi2d screen = {pge->ScreenWidth(), pge->ScreenHeight()};
        for (int i = 0; i < level.GetSegmentsCount(); i++) {
            auto seg = level.GetSegment(i);

            olc::vf2d sa = camera.WorldToScreen(seg.a, screen);
            olc::vf2d sb = camera.WorldToScreen(seg.b, screen);

            olc::vf2d bottomA = camera.WorldToScreen({seg.a.x, seg.a.y + 400.0f}, screen);
            olc::vf2d bottomB = camera.WorldToScreen({seg.b.x, seg.b.y + 400.0f}, screen);

            pge->FillTriangle(static_cast<int32_t>(sa.x), static_cast<int32_t>(sa.y),
                              static_cast<int32_t>(sb.x), static_cast<int32_t>(sb.y),
                              static_cast<int32_t>(bottomA.x), static_cast<int32_t>(bottomA.y), GROUND_COLOR);

            pge->FillTriangle(static_cast<int32_t>(sb.x), static_cast<int32_t>(sb.y),
                              static_cast<int32_t>(bottomB.x), static_cast<int32_t>(bottomB.y),
                              static_cast<int32_t>(bottomA.x), static_cast<int32_t>(bottomA.y), GROUND_COLOR);

            pge->DrawLine(static_cast<int32_t>(sa.x), static_cast<int32_t>(sa.y),
                          static_cast<int32_t>(sb.x), static_cast<int32_t>(sb.y), GROUND_EDGE);
        }
    }

    void RenderCollectibles(olc::PixelGameEngine* pge, const Level& level, const Camera& camera) {
        olc::vi2d screen = {pge->ScreenWidth(), pge->ScreenHeight()};
        for (int i = 0; i < level.GetCollectiblesCount(); i++) {
            auto c = level.GetCollectible(i);
            if (!c->IsActive()) continue;

            olc::vf2d sp = camera.WorldToScreen(c->GetPos(), screen);
            int32_t radius = static_cast<int32_t>(c->GetRadius() * camera.GetZoom());

            pge->FillCircle(static_cast<int32_t>(sp.x), static_cast<int32_t>(sp.y), radius, APPLE_COLOR);
            pge->DrawCircle(static_cast<int32_t>(sp.x), static_cast<int32_t>(sp.y), radius, olc::Pixel(180, 20, 20));

            pge->DrawLine(static_cast<int32_t>(sp.x), static_cast<int32_t>(sp.y) - radius,
                          static_cast<int32_t>(sp.x) + 3, static_cast<int32_t>(sp.y) - radius - 5,
                          olc::Pixel(50, 150, 50));

            pge->FillCircle(static_cast<int32_t>(sp.x) + 4, static_cast<int32_t>(sp.y) - radius - 3, 3, olc::Pixel(50, 180, 50));
        }
    }

    void RenderHazards(olc::PixelGameEngine* pge, const Level& level, const Camera& camera) {
        olc::vi2d screen = {pge->ScreenWidth(), pge->ScreenHeight()};
        for (int i = 0; i < level.GetHazardsCount(); i++) {
            auto h = level.GetHazard(i);
            olc::vf2d sp = camera.WorldToScreen(h->GetPos(), screen);
            int32_t r = static_cast<int32_t>(h->GetRadius() * camera.GetZoom());

            for (int j = 0; j < 5; j++) {
                float angle = static_cast<float>(j) * 3.14159f * 2.0f / 5.0f - 3.14159f / 2.0f;
                olc::vf2d tip = sp + olc::vf2d(std::cos(angle), std::sin(angle)) * static_cast<float>(r + 5);
                olc::vf2d base1 = sp + olc::vf2d(std::cos(angle - 0.3f), std::sin(angle - 0.3f)) * static_cast<float>(r - 3);
                olc::vf2d base2 = sp + olc::vf2d(std::cos(angle + 0.3f), std::sin(angle + 0.3f)) * static_cast<float>(r - 3);

                pge->FillTriangle(static_cast<int32_t>(tip.x), static_cast<int32_t>(tip.y),
                                  static_cast<int32_t>(base1.x), static_cast<int32_t>(base1.y),
                                  static_cast<int32_t>(base2.x), static_cast<int32_t>(base2.y), HAZARD_COLOR);
            }
            pge->FillCircle(static_cast<int32_t>(sp.x), static_cast<int32_t>(sp.y), r - 3, olc::Pixel(180, 180, 180));
        }
    }

    void RenderFinish(olc::PixelGameEngine* pge, const Level& level, const Camera& camera) {
        auto f = level.GetFinish();
        if (!f) return;

        olc::vi2d screen = {pge->ScreenWidth(), pge->ScreenHeight()};
        olc::vf2d sp = camera.WorldToScreen(f->GetPos(), screen);
        olc::Pixel color = f->IsActive() ? olc::Pixel(255, 255, 50) : olc::Pixel(100, 100, 80);

        int32_t r = static_cast<int32_t>(f->GetRadius() * camera.GetZoom());
        for (int j = 0; j < 6; j++) {
            float angle = static_cast<float>(j) * 3.14159f / 3.0f;
            olc::vf2d petalPos = sp + olc::vf2d(std::cos(angle), std::sin(angle)) * static_cast<float>(r - 2);
            pge->FillCircle(static_cast<int32_t>(petalPos.x), static_cast<int32_t>(petalPos.y), r / 3, color);
        }
        pge->FillCircle(static_cast<int32_t>(sp.x), static_cast<int32_t>(sp.y), r / 3 + 1,
                        f->IsActive() ? olc::Pixel(255, 200, 50) : olc::Pixel(80, 80, 60));
    }

    void RenderBike(olc::PixelGameEngine* pge, const Bike& bike, const Camera& camera) {
        olc::vi2d screen = {pge->ScreenWidth(), pge->ScreenHeight()};

        olc::vf2d fw = camera.WorldToScreen(bike.GetFrontWheel().GetPos(), screen);
        olc::vf2d rw = camera.WorldToScreen(bike.GetRearWheel().GetPos(), screen);
        olc::vf2d bd = camera.WorldToScreen(bike.GetBody().GetPos(), screen);
        olc::vf2d hd = camera.WorldToScreen(bike.GetHead().GetPos(), screen);

        pge->DrawLine(static_cast<int32_t>(fw.x), static_cast<int32_t>(fw.y),
                      static_cast<int32_t>(rw.x), static_cast<int32_t>(rw.y), FRAME_COLOR);
        pge->DrawLine(static_cast<int32_t>(fw.x), static_cast<int32_t>(fw.y),
                      static_cast<int32_t>(bd.x), static_cast<int32_t>(bd.y), FRAME_COLOR);
        pge->DrawLine(static_cast<int32_t>(rw.x), static_cast<int32_t>(rw.y),
                      static_cast<int32_t>(bd.x), static_cast<int32_t>(bd.y), FRAME_COLOR);
        pge->DrawLine(static_cast<int32_t>(bd.x), static_cast<int32_t>(bd.y),
                      static_cast<int32_t>(hd.x), static_cast<int32_t>(hd.y), BODY_COLOR);

        int32_t wheelR = static_cast<int32_t>(bike.GetFrontWheel().GetRadius() * camera.GetZoom());

        auto renderWheel = [&](olc::vf2d pos, float angle) {
            pge->DrawCircle(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y), wheelR, WHEEL_COLOR);
            pge->FillCircle(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y), wheelR - 2, olc::Pixel(60, 60, 60));

            for (int s = 0; s < 4; s++) {
                float a = angle + static_cast<float>(s) * 3.14159f / 2.0f;
                olc::vf2d spoke = pos + olc::vf2d(std::cos(a), std::sin(a)) * static_cast<float>(wheelR - 2);
                pge->DrawLine(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y),
                              static_cast<int32_t>(spoke.x), static_cast<int32_t>(spoke.y), olc::Pixel(120, 120, 120));
            }
        };

        renderWheel(fw, bike.GetFrontWheel().GetAngle());
        renderWheel(rw, bike.GetRearWheel().GetAngle());

        pge->FillCircle(static_cast<int32_t>(bd.x), static_cast<int32_t>(bd.y),
                        static_cast<int32_t>(5.0f * camera.GetZoom()), BODY_COLOR);

        int32_t headR = static_cast<int32_t>(bike.GetHead().GetRadius() * camera.GetZoom());
        pge->FillCircle(static_cast<int32_t>(hd.x), static_cast<int32_t>(hd.y), headR, HEAD_COLOR);
        pge->DrawCircle(static_cast<int32_t>(hd.x), static_cast<int32_t>(hd.y), headR, olc::Pixel(200, 160, 120));
    }

    void RenderHUD(olc::PixelGameEngine* pge, const Level& level, const Bike& bike, float gameTime) {
        pge->FillRect(0, 0, pge->ScreenWidth(), 22, olc::Pixel(0, 0, 0, 150));

        int total = level.GetCollectiblesCount();
        int collected = 0;
        for (int i = 0; i < total; i++) {
            if (!level.GetCollectible(i)->IsActive()) collected++;
        }
        pge->DrawString(5, 5, "Apples: " + std::to_string(collected) + "/" + std::to_string(total),
                        collected == total ? olc::Pixel(100, 255, 100) : olc::WHITE);

        int secs = static_cast<int>(gameTime);
        int ms = static_cast<int>((gameTime - static_cast<float>(secs)) * 100.0f);
        std::string timeStr = std::to_string(secs) + "." + (ms < 10 ? "0" : "") + std::to_string(ms) + "s";
        pge->DrawString(pge->ScreenWidth() - 80, 5, timeStr, olc::YELLOW);

        std::string dirStr = bike.GetDirection() > 0 ? ">>>" : "<<<";
        pge->DrawString(pge->ScreenWidth() / 2 - 12, 5, dirStr, olc::Pixel(150, 200, 255));
    }

    void RenderWorld(olc::PixelGameEngine* pge, const Level& level, const Bike& bike, const Camera& camera) {
        RenderSky(pge);
        RenderGround(pge, level, camera);
        RenderCollectibles(pge, level, camera);
        RenderHazards(pge, level, camera);
        RenderFinish(pge, level, camera);
        RenderBike(pge, bike, camera);
    }
};
