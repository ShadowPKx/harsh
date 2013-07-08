#ifndef GAME_H
#define GAME_H

#define NO_SOUND

#include "scene.h"

struct Game {
	Camera *camera;

    Game();
    ~Game();
    void pause();
    void update();
    void render();
    void touch(int id, int state, int x, int y);
};

#endif // GAME_H
