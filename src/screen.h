#ifndef Q2_2048_SCREEN_H
#define Q2_2048_SCREEN_H

#include "game.h"

enum { WIDTH = 375, HEIGHT = 320, BOARD_X = 8, BOARD_Y = 53, BOARD_SIZE = 252 };
enum { MENU_X = 271, MENU_Y = 8, MENU_W = 96, MENU_H = 32 };

struct rect { int x, y, w, h; };
struct rect direction_rect(enum direction direction);
struct rect option_rect(int choice);
int app_options(enum mode mode, const char *const **labels);
const char *app_heading(enum mode mode);
void render(const struct app *app, u32 *pixels, unsigned stride);

#endif
