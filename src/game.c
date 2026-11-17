/* Native port of Gabriele Cirulli's 2048 rules. See ../LICENSE.txt.
 * Byte-sized tile exponents follow Maurits van der Schee's 2048.c. */
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
    unsigned char empty[CELLS];
    unsigned count = 0;
    for (int i = 0; i < CELLS; ++i)
        if (!game->tile[i])
            empty[count++] = (unsigned char)i;
    if (count) {
        unsigned char value = random_word(rng) % 10 == 0 ? 2 : 1;
        game->tile[empty[random_word(rng) % count]] = value;
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
        unsigned char values[SIDE], result[SIDE] = {0};
        int count = 0, out = 0;
        for (int i = 0; i < SIDE; ++i) {
            unsigned char value = game->tile[cell(direction, line, i)];
            if (value)
                values[count++] = value;
        }

        for (int i = 0; i < count; ++i) {
            unsigned char value = values[i];
            if (i + 1 < count && value == values[i + 1] && value < MAX_EXPONENT) {
                ++value;
                ++i;
                game->score += 1u << value;
                if (value == GOAL_EXPONENT)
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
    if ((game->won && !game->continued) || !game_slide(game, direction))
        return 0;
    spawn(game, rng);
    return 1;
}

int game_can_move(const struct game *game)
{
    for (int i = 0; i < CELLS; ++i) {
        unsigned char tile = game->tile[i];
        if (!tile)
            return 1;
        if (tile == MAX_EXPONENT)
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
        unsigned char tile = game->tile[i];
        if (tile > MAX_EXPONENT || (tile >= GOAL_EXPONENT && !game->won))
            return 0;
        occupied += tile != 0;
    }
    return occupied >= 2;
}
