#include "screen.h"

enum { Q2_RETURN = 170, Q2_CENTRE = 218, WHEEL_STEP = 13, SWIPE_DISTANCE = 20 };

struct rect direction_rect(enum direction direction)
{
    static const struct rect buttons[] = {
        {304, 145, 30, 31}, {337, 180, 30, 31},
        {304, 215, 30, 31}, {271, 180, 30, 31}
    };
    return buttons[direction];
}

struct rect option_rect(int choice)
{
    return (struct rect){89, 112 + 46 * choice, 197, 38};
}

int app_options(enum mode mode, const char *const **labels)
{
    static const char *const menu[] = {"Resume", "New game", "Exit"};
    static const char *const restart[] = {"Cancel", "Start over"};
    static const char *const won[] = {"Keep playing", "New game", "Exit"};
    static const char *const lost[] = {"New game", "Exit"};
    switch (mode) {
    case MENU: *labels = menu; return 3;
    case RESTART: *labels = restart; return 2;
    case WON: *labels = won; return 3;
    case LOST: *labels = lost; return 2;
    default: *labels = 0; return 0;
    }
}

const char *app_heading(enum mode mode)
{
    switch (mode) {
    case MENU: return "Paused";
    case RESTART: return "Start a new game?";
    case WON: return "You reached 2048!";
    case LOST: return "No more moves";
    default: return "";
    }
}

static void mode(struct app *app, enum mode next)
{
    app->mode = next;
    app->choice = app->wheel = app->touching = 0;
}

static void update_mode(struct app *app)
{
    if (!game_can_move(&app->game))
        mode(app, LOST);
    else if (app->game.won && !app->game.continued)
        mode(app, WON);
    else
        mode(app, PLAY);
}

void app_start(struct app *app)
{
    app->direction = LEFT;
    update_mode(app);
}

static int move(struct app *app, enum direction direction)
{
    app->direction = direction;
    int changed = game_move(&app->game, direction, &app->rng);
    if (changed)
        update_mode(app);
    return changed;
}

static int select(struct app *app)
{
    int option = app->choice;
    switch (app->mode) {
    case PLAY: return move(app, app->direction);
    case MENU:
        mode(app, option == 0 ? PLAY : option == 1 ? RESTART : EXIT);
        return 0;
    case RESTART:
        if (!option) {
            update_mode(app);
            return 0;
        }
        break;
    case WON:
        if (option == 0) {
            app->game.continued = 1;
            mode(app, PLAY);
            return 1;
        }
        if (option == 2) {
            mode(app, EXIT);
            return 0;
        }
        mode(app, RESTART);
        return 0;
    case LOST:
        if (option == 1) {
            mode(app, EXIT);
            return 0;
        }
        break;
    default: return 0;
    }
    game_new(&app->game, &app->rng);
    mode(app, PLAY);
    return 1;
}

static int contains(struct rect area, int x, int y)
{
    return x >= area.x && x < area.x + area.w && y >= area.y && y < area.y + area.h;
}

static int touch_up(struct app *app, int x, int y)
{
    int start_x = app->touch_x, start_y = app->touch_y;
    if (!app->touching)
        return 0;
    app->touching = 0;
    if (app->mode == PLAY) {
        struct rect board = {BOARD_X, BOARD_Y, BOARD_SIZE, BOARD_SIZE};
        struct rect menu = {MENU_X, MENU_Y, MENU_W, MENU_H};
        if (contains(board, start_x, start_y)) {
            int dx = x - start_x, dy = y - start_y;
            int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
            if (ax >= SWIPE_DISTANCE || ay >= SWIPE_DISTANCE)
                return move(app, ax >= ay ? (dx < 0 ? LEFT : RIGHT) : (dy < 0 ? UP : DOWN));
        }
        if (contains(menu, start_x, start_y) && contains(menu, x, y))
            mode(app, MENU);
        for (int i = 0; app->mode == PLAY && i < DIRECTIONS; ++i) {
            struct rect button = direction_rect((enum direction)i);
            if (contains(button, start_x, start_y) && contains(button, x, y))
                return move(app, (enum direction)i);
        }
    } else {
        const char *const *labels;
        int count = app_options(app->mode, &labels);
        for (int i = 0; i < count; ++i) {
            struct rect button = option_rect(i);
            if (contains(button, start_x, start_y) && contains(button, x, y)) {
                app->choice = i;
                return select(app);
            }
        }
    }
    return 0;
}

static int number(const char **cursor)
{
    const char *p = *cursor;
    while (*p == ' ')
        ++p;
    int negative = *p == '-';
    if (negative)
        ++p;
    if (*p < '0' || *p > '9')
        return 10000;
    int value = 0;
    while (*p >= '0' && *p <= '9') {
        if (value > 999)
            return 10000;
        value = value * 10 + *p++ - '0';
    }
    *cursor = p;
    return negative ? -value : value;
}

int app_input(struct app *app, const char *input)
{
    if (!input[0] || input[1] != ' ')
        return 0;
    if (input[0] == 'w') {
        const char *p = input + 2;
        int value = number(&p);
        if (value < -2000 || value > 2000)
            return 0;
        app->wheel -= value;
        int steps = app->wheel / WHEEL_STEP;
        app->wheel %= WHEEL_STEP;
        if (app->mode == PLAY) {
            app->direction = (enum direction)(((int)app->direction + steps % DIRECTIONS +
                                               DIRECTIONS) % DIRECTIONS);
        } else {
            const char *const *labels;
            int count = app_options(app->mode, &labels);
            if (count)
                app->choice = (app->choice + steps % count + count) % count;
        }
        return 0;
    }
    if (!input[2] || input[3] != ' ')
        return 0;
    const char *p = input + 4;
    int a = number(&p);
    if (input[0] == 't') {
        int b = number(&p);
        if (a < 0 || a >= WIDTH || b < 0 || b >= HEIGHT) {
            app->touching = 0;
            return 0;
        }
        if (input[2] == 'd') {
            app->touch_x = a;
            app->touch_y = b;
            app->touching = 1;
        } else if (input[2] == 'u') {
            return touch_up(app, a, b);
        }
    } else if (input[0] == 'k' && input[2] == 'u') {
        if (a == Q2_CENTRE)
            return select(app);
        if (a == Q2_RETURN) {
            if (app->mode == PLAY)
                mode(app, MENU);
            else if (app->mode == MENU)
                mode(app, PLAY);
            else if (app->mode == RESTART)
                update_mode(app);
            else
                mode(app, EXIT);
        }
    }
    return 0;
}
