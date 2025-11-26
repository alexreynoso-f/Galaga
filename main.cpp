#include "Game.h"

int main() {
    const unsigned int W = 800;
    const unsigned int H = 850;
    Game game(W, H);
    if (!game.init()) return 1;
    game.run();
    return 0;
}