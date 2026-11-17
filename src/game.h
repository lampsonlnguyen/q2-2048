#ifndef Q2_2048_GAME_H
#define Q2_2048_GAME_H

enum { SIDE = 4, CELLS = SIDE * SIDE, GOAL_EXPONENT = 11, MAX_EXPONENT = 17 };
enum direction { UP, RIGHT, DOWN, LEFT, DIRECTIONS };

typedef unsigned int u32;

struct game {
    unsigned char tile[CELLS]; /* 0 is empty, otherwise the exponent of two. */
    u32 score, best, won, continued;
};

enum mode { PLAY, MENU, RESTART, WON, LOST, EXIT };

struct app {
    struct game game;
    u32 rng;
    enum mode mode;
    enum direction direction;
    int choice, wheel, touch_x, touch_y, touching;
    int save_failed;
};

void game_new(struct game *game, u32 *rng);
int game_slide(struct game *game, enum direction direction);
int game_move(struct game *game, enum direction direction, u32 *rng);
int game_can_move(const struct game *game);
int game_valid(const struct game *game);
void app_start(struct app *app);
int app_input(struct app *app, const char *input);

#endif
