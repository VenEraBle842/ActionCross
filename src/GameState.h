#pragma once

// Опережающее объявление разрешает циклическую зависимость Game <-> IGameState
class Game;

// Базовый интерфейс для паттерна State
class IGameState {
public:
    virtual ~IGameState() = default;

    virtual void OnEnter(Game* game) {}
    virtual void OnExit(Game* game) {}

    virtual void Update(Game* game, float dt) = 0;
    virtual void Render(Game* game) = 0;
};
