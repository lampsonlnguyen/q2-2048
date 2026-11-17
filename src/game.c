/* Native port of Gabriele Cirulli's 2048 rules. See ../LICENSE.txt. */
#include "game.h"

static u32 random_word(u32 *rng)
{
    u32 x = *rng ? *rng : 1;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *rng = x;
}

static void spawn(struct game *game, u32 *rng)
{
    int empty[CELLS], count = 0;
    for (int i = 0; i < CELLS; ++i)
        if (!game->tile[i])
            empty[count++] = i;
    if (count) {
        u32 value = random_word(rng) % 10 == 0 ? 4 : 2;
        game->tile[empty[random_word(rng) % (u32)count]] = value;
    }
}

void game_new(struct game *game, u32 *rng)
{
    *game = (struct game){ .best = game->best };
    spawn(game, rng);
    spawn(game, rng);
}

static int cell(enum direction direction, int line, int offset)
{
    switch (direction) {
    case UP: return offset * SIDE + line;
    case RIGHT: return line * SIDE + SIDE - 1 - offset;
    case DOWN: return (SIDE - 1 - offset) * SIDE + line;
    default: return line * SIDE + offset;
    }
}

int game_slide(struct game *game, enum direction direction)
{
    int moved = 0;
    for (int line = 0; line < SIDE; ++line) {
        u32 values[SIDE], result[SIDE] = {0};
        int count = 0, out = 0;
        for (int i = 0; i < SIDE; ++i) {
            u32 value = game->tile[cell(direction, line, i)];
            if (value)
                values[count++] = value;
        }

        for (int i = 0; i < count; ++i) {
            u32 value = values[i];
            if (i + 1 < count && value == values[i + 1] && value < MAX_TILE) {
                value *= 2;
                ++i;
                game->score += value;
                if (value == 2048)
                    game->won = 1;
            }
            result[out++] = value;
        }

        for (int i = 0; i < SIDE; ++i) {
            int index = cell(direction, line, i);
            moved |= game->tile[index] != result[i];
            game->tile[index] = result[i];
        }
    }
    if (game->score > game->best)
        game->best = game->score;
    return moved;
}

int game_move(struct game *game, enum direction direction, u32 *rng)
{
    if (!game_slide(game, direction))
        return 0;
    spawn(game, rng);
    return 1;
}

int game_can_move(const struct game *game)
{
    for (int i = 0; i < CELLS; ++i) {
        u32 tile = game->tile[i];
        if (!tile)
            return 1;
        if (tile == MAX_TILE)
            continue;
        if (i % SIDE != SIDE - 1 && tile == game->tile[i + 1])
            return 1;
        if (i + SIDE < CELLS && tile == game->tile[i + SIDE])
            return 1;
    }
    return 0;
}

int game_valid(const struct game *game)
{
    int occupied = 0;
    if (game->score > game->best || game->won > 1 || game->continued > game->won)
        return 0;
    for (int i = 0; i < CELLS; ++i) {
        u32 tile = game->tile[i];
        if (tile == 1 || tile > MAX_TILE || (tile & (tile - 1)) ||
            (tile >= 2048 && !game->won))
            return 0;
        occupied += tile != 0;
    }
    return occupied >= 2;
}
