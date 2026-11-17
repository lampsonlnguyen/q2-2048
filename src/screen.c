#include "screen.h"
#include "font.h"

enum {
    BACKGROUND = 0xfaf8ef, GRID = 0xbbada0, BROWN = 0x776e65,
    LIGHT = 0xf9f6f2, BUTTON = 0x8f7a66, EMPTY = 0xcdc1b4, GOLD = 0xedc53f
};

static void fill(u32 *pixels, unsigned stride, struct rect area, u32 color)
{
    /* Logical (x, y) becomes (HEIGHT - 1 - y, x) on the portrait panel. */
    int left = HEIGHT - area.y - area.h;
    int right = HEIGHT - area.y;
    for (int x = area.x; x < area.x + area.w; ++x) {
        u32 *row = pixels + x * stride;
        for (int column = left; column < right; ++column)
            row[column] = 0xff000000u | color;
    }
}

static void text(u32 *pixels, unsigned stride, struct rect area, const char *str, u32 color)
{
    int width = 0;
    for (const char *p = str; *p; ++p)
        width += f20_adv[*p - 32];
    int x = area.x + (area.w - width) / 2;
    int y = area.y + (area.h - F20_H) / 2;
    for (; *str; ++str) {
        int glyph = *str - 32;
        int w = f20_adv[glyph];
        int glyph_offset = f20_off[glyph];
        for (int row = 0; row < F20_H; ++row) {
            for (int column = 0; column < w; ++column) {
                int bit = glyph_offset + row * w + column;
                if (f20_px[bit / 8] & (1u << (bit % 8)))
                    pixels[(x + column) * stride + (HEIGHT - 1 - y - row)] = 0xff000000u | color;
            }
        }
        x += w;
    }
}

static void decimal(char *str, u32 value)
{
    char reverse[10];
    int n = 0;
    do {
        reverse[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (n)
        *str++ = reverse[--n];
    *str = '\0';
}

static void number(u32 *pixels, unsigned stride, struct rect area, u32 value, u32 color)
{
    char str[11];
    decimal(str, value);
    text(pixels, stride, area, str, color);
}

static void board(const struct game *game, u32 *pixels, unsigned stride)
{
    static const u32 tile_colors[] = {
        EMPTY, 0xeee4da, 0xede0c8, 0xf2b179, 0xf59563, 0xf67c5f, 0xf65e3b,
        0xedcf72, 0xedcc61, 0xedc850, 0xedc53f, 0xedc22e
    };
    enum { GAP = 4, TILE = (BOARD_SIZE - (SIDE + 1) * GAP) / SIDE };
    fill(pixels, stride, (struct rect){BOARD_X, BOARD_Y, BOARD_SIZE, BOARD_SIZE}, GRID);
    for (int i = 0; i < CELLS; ++i) {
        unsigned char power = game->tile[i];
        int x = BOARD_X + GAP + (i % SIDE) * (TILE + GAP);
        int y = BOARD_Y + GAP + (i / SIDE) * (TILE + GAP);
        struct rect tile = {x, y, TILE, TILE};
        int colors = (int)(sizeof tile_colors / sizeof tile_colors[0]);
        fill(pixels, stride, tile, power < colors ? tile_colors[power] : 0x3c3a32);
        if (power)
            number(pixels, stride, tile, 1u << power, power <= 2 ? BROWN : LIGHT);
    }
}

void render(const struct app *app, u32 *pixels, unsigned stride)
{
    const struct game *game = &app->game;
    fill(pixels, stride, (struct rect){0, 0, WIDTH, HEIGHT}, BACKGROUND);
    text(pixels, stride, (struct rect){8, 8, BOARD_SIZE, 32}, "2048", BROWN);
    struct rect menu = {MENU_X, MENU_Y, MENU_W, MENU_H};
    fill(pixels, stride, menu, BUTTON);
    text(pixels, stride, menu, "Menu", LIGHT);
    board(game, pixels, stride);

    text(pixels, stride, (struct rect){271, 49, 96, 24}, "Score", BROWN);
    number(pixels, stride, (struct rect){271, 72, 96, 24}, game->score, BROWN);
    text(pixels, stride, (struct rect){271, 96, 96, 24}, "Best", BROWN);
    number(pixels, stride, (struct rect){271, 119, 96, 24}, game->best, BROWN);

    static const char *const arrows[] = {"^", ">", "v", "<"};
    for (int i = 0; i < DIRECTIONS; ++i) {
        struct rect button = direction_rect((enum direction)i);
        int selected = i == (int)app->direction;
        fill(pixels, stride, button, selected ? GOLD : GRID);
        text(pixels, stride, button, arrows[i], BROWN);
    }
    if (app->save_failed) {
        text(pixels, stride, (struct rect){268, 269, 102, 24}, "Save failed", BROWN);
    } else {
        text(pixels, stride, (struct rect){268, 258, 102, 24}, "Wheel", BROWN);
        text(pixels, stride, (struct rect){268, 282, 102, 24}, "+ Centre", BROWN);
    }

    const char *const *labels;
    int count = app_options(app->mode, &labels);
    if (count) {
        fill(pixels, stride, (struct rect){70, 62, 235, 228}, BROWN);
        text(pixels, stride, (struct rect){70, 70, 235, 31}, app_heading(app->mode), LIGHT);
        for (int i = 0; i < count; ++i) {
            struct rect option = option_rect(i);
            fill(pixels, stride, option, i == app->choice ? GOLD : BUTTON);
            text(pixels, stride, option, labels[i], i == app->choice ? BROWN : LIGHT);
        }
    }
}
