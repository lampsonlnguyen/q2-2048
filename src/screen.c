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

static void board(const struct game *game, u32 *pixels, unsigned stride, int light)
{
    /* Classic 2048 hues in light mode; muted jewel tones in dark mode. */
    static const u32 light_tiles[] = {
        EMPTY, 0xeee4da, 0xede0c8, 0xf2b179, 0xf59563, 0xf67c5f, 0xf65e3b,
        0xedcf72, 0xedcc61, 0xedc850, 0xedc53f, 0xedc22e
    };
    static const u32 dark_tiles[] = {
        0x303b4c, 0x495a70, 0x5b6882, 0x765c80, 0x87577b, 0x91546c,
        0x99505a, 0x9a704b, 0x9f8245, 0xb1944b, 0xb6a052, 0xc0a95a
    };
    const u32 *tiles = light ? light_tiles : dark_tiles;
    enum { GAP = 4, TILE = (BOARD_SIZE - (SIDE + 1) * GAP) / SIDE };
    fill(pixels, stride, (struct rect){BOARD_X, BOARD_Y, BOARD_SIZE, BOARD_SIZE},
         light ? GRID : 0x182432);
    for (int i = 0; i < CELLS; ++i) {
        unsigned char power = game->tile[i];
        int x = BOARD_X + GAP + (i % SIDE) * (TILE + GAP);
        int y = BOARD_Y + GAP + (i / SIDE) * (TILE + GAP);
        struct rect tile = {x, y, TILE, TILE};
        int colors = (int)(sizeof light_tiles / sizeof light_tiles[0]);
        fill(pixels, stride, tile, power < colors ? tiles[power] :
             (light ? 0x3c3a32 : 0xd1ae61));
        if (power)
            number(pixels, stride, tile, 1u << power,
                   light && power <= 2 ? BROWN : LIGHT);
    }
}
 

void render(const struct app *app, u32 *pixels, unsigned stride)
{
    const struct game *game = &app->game;
    int light = app->light;
    u32 background = light ? BACKGROUND : 0x101820;
    u32 text_color = light ? BROWN : 0xe7edf3;
    u32 button_color = light ? BUTTON : 0x34475c;
    u32 control_color = light ? GRID : 0x29394d;
    u32 selected_color = light ? GOLD : 0xe4b75e;
    u32 selected_text = light ? BROWN : 0x151d29;
    fill(pixels, stride, (struct rect){0, 0, WIDTH, HEIGHT}, background);
    text(pixels, stride, (struct rect){8, 8, BOARD_SIZE, 32}, "2048", text_color);
    struct rect menu = {MENU_X, MENU_Y, MENU_W, MENU_H};
    fill(pixels, stride, menu, button_color);
    text(pixels, stride, menu, "Menu", LIGHT);
    board(game, pixels, stride, light);

    text(pixels, stride, (struct rect){271, 49, 96, 24}, "Score", text_color);
    number(pixels, stride, (struct rect){271, 72, 96, 24}, game->score, text_color);
    text(pixels, stride, (struct rect){271, 96, 96, 24}, "Best", text_color);
    number(pixels, stride, (struct rect){271, 119, 96, 24}, game->best, text_color);

    static const char *const arrows[] = {"^", ">", "v", "<"};
    for (int i = 0; i < DIRECTIONS; ++i) {
        struct rect button = direction_rect((enum direction)i);
        int selected = i == (int)app->direction;
        fill(pixels, stride, button, selected ? selected_color : control_color);
        text(pixels, stride, button, arrows[i],
             selected ? selected_text : text_color);
    }
    if (app->save_failed) {
        text(pixels, stride, (struct rect){268, 269, 102, 24}, "Save failed", text_color);
    } else {
        text(pixels, stride, (struct rect){268, 258, 102, 24}, "Wheel", text_color);
        text(pixels, stride, (struct rect){268, 282, 102, 24}, "+ Centre", text_color);
    }

    const char *const *labels;
    int count = app_options(app->mode, &labels);
    if (count) {
        fill(pixels, stride, (struct rect){70, 62, 235, 228},
             light ? BROWN : 0x253447);
        text(pixels, stride, (struct rect){70, 70, 235, 31}, app_heading(app->mode), LIGHT);
        for (int i = 0; i < count; ++i) {
            struct rect option = option_rect(i);
            int selected = i == app->choice;
            fill(pixels, stride, option, selected ? selected_color : button_color);
            text(pixels, stride, option, labels[i],
                 selected ? selected_text : LIGHT);
        }
    }
}
