#include "screen.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static u32 pixels[WIDTH * HEIGHT];

static void check_merges(void)
{
    struct game game = {.tile = {2, 2, 2, 2, 2, 2, 4}};
    assert(game_slide(&game, LEFT));
    assert(game.tile[0] == 4 && game.tile[1] == 4 && !game.tile[2]);
    assert(game.tile[4] == 4 && game.tile[5] == 4 && !game.tile[6]);
    assert(game.score == 12 && game.best == 12);

    game = (struct game){.tile = {2, 2, 2}};
    assert(game_slide(&game, RIGHT));
    assert(!game.tile[1] && game.tile[2] == 2 && game.tile[3] == 4);

    game = (struct game){.tile = {[4] = 16, [12] = 16}};
    assert(game_slide(&game, UP));
    assert(game.tile[0] == 32 && !game.tile[4] && !game.tile[12]);

    game = (struct game){.tile = {[3] = 8, [7] = 8, [11] = 8}};
    assert(game_slide(&game, DOWN));
    assert(!game.tile[7] && game.tile[11] == 8 && game.tile[15] == 16);

    game = (struct game){.tile = {2, 4}};
    u32 rng = 127;
    assert(!game_move(&game, LEFT, &rng));
    assert(rng == 127 && game.tile[0] == 2 && game.tile[1] == 4);
    assert(game_move(&game, RIGHT, &rng));
    int occupied = 0;
    for (int i = 0; i < CELLS; ++i)
        occupied += game.tile[i] != 0;
    assert(occupied == 3 && rng != 127);
}

static void check_controls(void)
{
    struct app app = {.game = {.tile = {1024, 1024}}, .rng = 9};
    app_start(&app);
    assert(app.mode == PLAY && app.direction == LEFT);
    app_input(&app, "w -13");
    assert(app.direction == UP);
    app_input(&app, "w 13");
    assert(app.direction == LEFT);
    assert(!app_input(&app, "k d 218"));
    assert(app_input(&app, "k u 218"));
    assert(app.mode == WON && app.game.score == 2048);

    render(&app, pixels, HEIGHT);
    assert(app_input(&app, "k u 218"));
    assert(app.mode == PLAY && app.game.continued);
    app_input(&app, "t d 327 16");
    app_input(&app, "t u 326 17");
    assert(app.mode == MENU);
    app_input(&app, "w -13");
    assert(app.choice == 1);
    app_input(&app, "k u 218");
    assert(app.mode == RESTART);
    app_input(&app, "k u 170");
    assert(app.mode == PLAY && app.game.best == 2048);

    app.game = (struct game){.tile = {2, 4}};
    app_input(&app, "t d 90 100");
    assert(app_input(&app, "t u 170 101"));
    assert(app.direction == RIGHT && app.game.tile[2] == 2 && app.game.tile[3] == 4);
    render(&app, pixels, HEIGHT);

    app.game = (struct game){.tile = {2,4,2,4, 4,2,4,2, 2,4,2,4, 4,2,4,2}};
    assert(!game_can_move(&app.game));
    app_start(&app);
    assert(app.mode == LOST);
    assert(app_input(&app, "k u 218"));
    assert(app.mode == PLAY && game_valid(&app.game));
    app.game.tile[0] = 3;
    assert(!game_valid(&app.game));
}

int main(void)
{
    check_merges();
    check_controls();
    puts("Q2 2048 checks passed");
    return 0;
}
