#pragma once

#include "Level/LevelLoader.h"
#include "Bike/Bike.h"
#include "Camera.h"
#include "GameState.h"
#include "GameRenderer.h"
#include "ProgressManager.h"
#include <algorithm>

class StateMenu;
class StateLevelSelect;
class StatePlay;
class StateCrash;
class StateFinish;

class Game : public olc::PixelGameEngine {
    PhysicsWorld physics;
    Level level;
    Bike bike;
    Camera camera;
    GameRenderer renderer;
    ProgressManager progress;

    IGameState* currentState = nullptr;
    IGameState* nextState = nullptr;

    float accumulator = 0.0f;
    const float PHYSICS_DT = 1.0f / 120.0f;

    int currentLevelIndex = 1;
    int totalLevels = 0;
    float gameTime = 0.0f;

public:
    Game() {
        sAppName = "ActionCross";
    }

    ~Game() override {
        if (currentState) {
            currentState->OnExit(this);
            delete currentState;
        }
            delete nextState;
    }

    bool OnUserCreate() override;
    bool OnUserUpdate(float fElapsedTime) override;

    void ChangeState(IGameState* newState) {
        nextState = newState;
    }

    void CountLevels() {
        totalLevels = 0;
        int i = 1;
        while (LevelLoader::Exists("assets/level_" + std::to_string(i) + ".aclvl")) {
            totalLevels++;
            i++;
        }
        if (totalLevels == 0) {
            std::cerr << "[WARNING] No levels found!" << std::endl;
        }
    }

    void LoadCurrentLevel() {
        std::string path = "assets/level_" + std::to_string(currentLevelIndex) + ".aclvl";
        LevelLoader::LoadFromAclvl(level, path);

        physics.Clear();
        physics.SetLevel(&level);

        bike.Init(level.GetSpawnPos());
        bike.RegisterWith(physics);

        camera.SetPosition(level.GetSpawnPos());
        camera.SetZoom(1.0f);

        gameTime = 0.0f;
        accumulator = 0.0f;
    }

    void ResetLevel() { LoadCurrentLevel(); }

    bool NextLevel() {
        progress.UnlockNextLevel(currentLevelIndex);
        if (currentLevelIndex < totalLevels) {
            currentLevelIndex++;
            LoadCurrentLevel();
            return true;
        }
        return false;
    }

    PhysicsWorld& GetPhysics() { return physics; }
    Level& GetLevel() { return level; }
    Bike& GetBike() { return bike; }
    Camera& GetCamera() { return camera; }
    GameRenderer& GetRenderer() { return renderer; }
    ProgressManager& GetProgress() { return progress; }

    float GetGameTime() const { return gameTime; }
    void AddGameTime(float dt) { gameTime += dt; }

    float GetPhysicsDt() const { return PHYSICS_DT; }
    float GetAccumulator() const { return accumulator; }
    void AddAccumulator(float dt) { accumulator += dt; }
    void SubAccumulator(float dt) { accumulator -= dt; }

    int GetCurrentLevelIndex() const { return currentLevelIndex; }
    void SetCurrentLevelIndex(int idx) { currentLevelIndex = idx; }
    int GetTotalLevels() const { return totalLevels; }
};

// ОБЪЯВЛЕНИЯ КЛАССОВ СОСТОЯНИЙ

class StateMenu : public IGameState {
public:
    void Update(Game* game, float dt) override;
    void Render(Game* game) override;
};

class StateLevelSelect : public IGameState {
public:
    void Update(Game* game, float dt) override;
    void Render(Game* game) override;
};

class StatePlay : public IGameState {
public:
    void Update(Game* game, float dt) override;
    void Render(Game* game) override;
};

class StateCrash : public IGameState {
public:
    void Update(Game* game, float dt) override;
    void Render(Game* game) override;
};

class StateFinish : public IGameState {
public:
    void Update(Game* game, float dt) override;
    void Render(Game* game) override;
};

// РЕАЛИЗАЦИЯ МЕТОДОВ

inline bool Game::OnUserCreate() {
    CountLevels();
    progress.LoadProgress();

    currentState = new StateMenu();
    return true;
}

inline bool Game::OnUserUpdate(float fElapsedTime) {
    if (nextState) {
        if (currentState) {
            currentState->OnExit(this);
            delete currentState;
        }
        currentState = nextState;
        nextState = nullptr;
        if (currentState) {
            currentState->OnEnter(this);
        }
    }

    if (currentState) {
        currentState->Update(this, fElapsedTime);
        currentState->Render(this);
    }
    return true;
}

// 1. STATE MENU
inline void StateMenu::Update(Game* game, float dt) {
    if (game->GetKey(olc::Key::ENTER).bPressed || game->GetKey(olc::Key::SPACE).bPressed) {
        game->GetProgress().LoadProgress();
        game->ChangeState(new StateLevelSelect());
    }
}

inline void StateMenu::Render(Game* game) {
    game->Clear(olc::Pixel(30, 30, 50));

    std::string title = "ACTION CROSS";
    int titleX = game->ScreenWidth() / 2 - (static_cast<int>(title.size()) * 11);

    game->DrawString(titleX, game->ScreenHeight() / 3, title, olc::Pixel(255, 200, 50), 3);

    int y = game->ScreenHeight() / 2 + 20;
    game->DrawString(50, y, "Controls:", olc::Pixel(150, 200, 255));
    game->DrawString(50, y + 15, " UP / W - Throttle", olc::WHITE);
    game->DrawString(50, y + 27, " DOWN / S - Brake", olc::WHITE);
    game->DrawString(50, y + 39, " SPACE - Flip direction", olc::WHITE);
    game->DrawString(50, y + 51, " SHIFT - Jump", olc::WHITE);
    game->DrawString(50, y + 63, " R - Restart", olc::WHITE);

    game->DrawString(titleX - 10, game->ScreenHeight() - 60,
                     "Press ENTER or SPACE to select level",
                     olc::Pixel(200, 200, 200));
}

// 2. STATE LEVEL SELECT
inline void StateLevelSelect::Update(Game* game, float dt) {
    ProgressManager& prog = game->GetProgress();
    int unlockedLvl = prog.GetUnlockedLevel();
    int curLvl = game->GetCurrentLevelIndex();

    int maxAvailable = std::min(unlockedLvl, game->GetTotalLevels());

    if (game->GetKey(olc::Key::LEFT).bPressed && curLvl > 1) {
        game->SetCurrentLevelIndex(curLvl - 1);
    }
    if (game->GetKey(olc::Key::RIGHT).bPressed && curLvl < maxAvailable) {
        game->SetCurrentLevelIndex(curLvl + 1);
    }

    if (game->GetKey(olc::Key::DEL).bPressed) {
        prog.ResetProgress();
        game->SetCurrentLevelIndex(1);
    }

    if (game->GetKey(olc::Key::ENTER).bPressed || game->GetKey(olc::Key::SPACE).bPressed) {
        if (game->GetTotalLevels() > 0) {
            game->LoadCurrentLevel();
            game->ChangeState(new StatePlay());
        }
    }

    if (game->GetKey(olc::Key::ESCAPE).bPressed) {
        game->ChangeState(new StateMenu());
    }
}

inline void StateLevelSelect::Render(Game* game) {
    game->Clear(olc::Pixel(30, 30, 50));
    game->DrawString(50, 30, "LEVEL SELECTION", olc::CYAN, 3);
    game->DrawString(50, 70, "Press ESC to return to main menu", olc::DARK_GREY, 1);
    game->DrawString(50, 90, "Press DEL to reset progress", olc::DARK_GREY, 1);

    if (game->GetTotalLevels() == 0) {
        std::string noLvl = "NO LEVELS FOUND";
        int noLvlX = game->ScreenWidth() / 2 - (static_cast<int>(noLvl.size()) * 16) / 2;
        game->DrawString(noLvlX, game->ScreenHeight() / 2, noLvl, olc::RED, 2);
        return;
    }

    int unlockedLvl = game->GetProgress().GetUnlockedLevel();
    int curLvl = game->GetCurrentLevelIndex();
    int maxAvailable = std::min(unlockedLvl, game->GetTotalLevels());

    std::string leftArrow = (curLvl > 1) ? "< " : "  ";
    std::string rightArrow = (curLvl < maxAvailable) ? " >" : "  ";

    std::string lvlStr = leftArrow + "LEVEL " + std::to_string(curLvl) + rightArrow;

    int lvlX = game->ScreenWidth() / 2 - (static_cast<int>(lvlStr.size()) * 24) / 2;
    game->DrawString(lvlX, game->ScreenHeight() / 2 - 30, lvlStr, olc::YELLOW, 3);

    std::string enterStr = "Press ENTER to Start";
    int enterX = game->ScreenWidth() / 2 - (static_cast<int>(enterStr.size()) * 16) / 2;
    game->DrawString(enterX, game->ScreenHeight() / 2 + 40, enterStr, olc::GREEN, 2);
}

// 3. STATE PLAY
inline void StatePlay::Update(Game* game, float dt) {
    Bike& bike = game->GetBike();
    Level& level = game->GetLevel();
    PhysicsWorld& physics = game->GetPhysics();
    Camera& camera = game->GetCamera();

    game->AddGameTime(dt);

    if (game->GetKey(olc::Key::SPACE).bPressed) bike.FlipDirection();

    if (game->GetKey(olc::Key::SHIFT).bPressed) {
        if (bike.GetFrontWheel().IsGrounded() || bike.GetRearWheel().IsGrounded()) {
            bike.Jump(3.0f);
        }
    }

    if (game->GetKey(olc::Key::R).bPressed) {
        game->ResetLevel();
        return;
    }
    if (game->GetKey(olc::Key::ESCAPE).bPressed) {
        game->ChangeState(new StateLevelSelect());
        return;
    }

    if (game->GetKey(olc::Key::UP).bHeld || game->GetKey(olc::Key::W).bHeld) {
        bike.Throttle(dt);
    }
    if (game->GetKey(olc::Key::DOWN).bHeld || game->GetKey(olc::Key::S).bHeld) {
        bike.Brake();
    }

    game->AddAccumulator(dt);
    while (game->GetAccumulator() >= game->GetPhysicsDt()) {
        physics.Step(game->GetPhysicsDt());
        game->SubAccumulator(game->GetPhysicsDt());
    }

    camera.SetTarget(bike.GetCenterOfMass());
    camera.Update(dt);

    float fallDeathY = level.GetLowestPoint() + 400.0f;
    if (bike.GetCenterOfMass().y > fallDeathY) {
        game->ChangeState(new StateCrash());
        return;
    }

    if (bike.IsHeadGrounded()) {
        game->ChangeState(new StateCrash());
        return;
    }

    for (int i = 0; i < level.GetCollectiblesCount(); i++) {
        auto* c = level.GetCollectible(i);
        if (c->IsActive() && bike.IsCloseTo(c->GetPos(), c->GetRadius() + 25.0f)) {
            c->OnInteract();
        }
    }

    if (level.AllCollected() && level.GetFinish() && !level.GetFinish()->IsActive()) {
        level.GetFinish()->SetActive(true);
    }

    if (level.GetFinish() && level.GetFinish()->IsActive()) {
        float distToFinish = (bike.GetCenterOfMass() - level.GetFinish()->GetPos()).mag();
        if (distToFinish < level.GetFinish()->GetRadius() + 30.0f) {
            game->ChangeState(new StateFinish());
            return;
        }
    }

    for (int i = 0; i < level.GetHazardsCount(); i++) {
        auto* h = level.GetHazard(i);
        if (bike.IsCloseTo(h->GetPos(), h->GetRadius() + 20.0f)) {
            game->ChangeState(new StateCrash());
            return;
        }
    }
}

inline void StatePlay::Render(Game* game) {
    game->Clear(olc::BLACK);
    game->GetRenderer().RenderWorld(game, game->GetLevel(), game->GetBike(), game->GetCamera());
    game->GetRenderer().RenderHUD(game, game->GetLevel(), game->GetBike(), game->GetGameTime());
}

// 4. STATE CRASH
inline void StateCrash::Update(Game* game, float dt) {
    game->AddAccumulator(dt);
    while (game->GetAccumulator() >= game->GetPhysicsDt()) {
        game->GetPhysics().Step(game->GetPhysicsDt());
        game->SubAccumulator(game->GetPhysicsDt());
    }

    game->GetCamera().SetTarget(game->GetBike().GetCenterOfMass());
    game->GetCamera().Update(dt);

    if (game->GetKey(olc::Key::ENTER).bPressed || game->GetKey(olc::Key::R).bPressed) {
        game->ResetLevel();
        game->ChangeState(new StatePlay());
    }
    if (game->GetKey(olc::Key::ESCAPE).bPressed) {
        game->ChangeState(new StateLevelSelect());
    }
}

inline void StateCrash::Render(Game* game) {
    game->GetRenderer().RenderWorld(game, game->GetLevel(), game->GetBike(), game->GetCamera());
    game->FillRect(0, game->ScreenHeight() / 2 - 20, game->ScreenWidth(), 40, olc::Pixel(0, 0, 0, 200));
    game->DrawString(game->ScreenWidth() / 2 - 80, game->ScreenHeight() / 2 - 8, "CRASHED! Press ENTER", olc::RED);
}

// 5. STATE FINISH
inline void StateFinish::Update(Game* game, float dt) {
    game->GetBike().Brake();

    game->AddAccumulator(dt);
    while (game->GetAccumulator() >= game->GetPhysicsDt()) {
        game->GetPhysics().Step(game->GetPhysicsDt());
        game->SubAccumulator(game->GetPhysicsDt());
    }

    game->GetCamera().SetTarget(game->GetBike().GetCenterOfMass());
    game->GetCamera().Update(dt);

    if (game->GetKey(olc::Key::ENTER).bPressed) {
        if (game->NextLevel()) {
            game->ChangeState(new StatePlay());
        } else {
            game->ChangeState(new StateLevelSelect());
        }
    }
}

inline void StateFinish::Render(Game* game) {
    game->GetRenderer().RenderWorld(game, game->GetLevel(), game->GetBike(), game->GetCamera());
    game->FillRect(0, game->ScreenHeight() / 2 - 20, game->ScreenWidth(), 40, olc::Pixel(0, 0, 0, 200));
    game->DrawString(game->ScreenWidth() / 2 - 100, game->ScreenHeight() / 2 - 8, "LEVEL CLEARED! Press ENTER", olc::GREEN);
}
