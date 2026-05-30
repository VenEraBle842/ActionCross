#define OLC_PGE_APPLICATION

#include "Game.h"

int main() {
    Game game;
    if (game.Construct(800, 600, 1, 1))
        game.Start();
    return 0;
}
