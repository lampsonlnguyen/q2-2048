/* MIPS startup and syscall trampoline from Buddy by Diamond Bond.
 * See ../LICENSE.txt.
 */
#include "screen.h"

long sc(long n, long a, long b, long c, long d, long e, long f);
__asm__(".text\n.set noreorder\n.globl sc\nsc:\n"
        "move $2, $4\nmove $4, $5\nmove $5, $6\nmove $6, $7\n"
        "lw $7, 16($29)\nlw $8, 20($29)\nlw $9, 24($29)\n"
        "addiu $29, $29, -32\nsw $8, 16($29)\nsw $9, 20($29)\n"
        "syscall\naddiu $29, $29, 32\n"
        "beqz $7, 1f\nnop\nsubu $2, $0, $2\n1: jr $31\nnop\n"
        ".globl __start\n__start:\n"
        "lw $4, 0($29)\naddiu $5, $29, 4\nli $8, -8\nand $29, $29, $8\naddiu $29, $29, -32\n"
        "jal q2_main\nnop\nmove $4, $2\nli $2, 4246\nsyscall\n1: b 1b\nnop\n.set reorder\n");

#define SYS_read 4003
#define SYS_write 4004
#define SYS_open 4005
#define SYS_close 4006
#define SYS_unlink 4010
#define SYS_rename 4038
#define SYS_ioctl 4054
#define SYS_mmap 4090
#define SYS_munmap 4091
#define SYS_fsync 4118
#define SYS_bind 4169
#define SYS_recv 4175
#define SYS_socket 4183
#define SYS_poll 4188
#define SYS_clock_gettime 4263

#define FBIOGET_VSCREENINFO 0x4600
#define FBIOGET_FSCREENINFO 0x4602
#define FBIOPAN_DISPLAY 0x4606
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x100
#define O_TRUNC 0x200
#define O_CLOEXEC 0x80000
#define MSG_DONTWAIT 0x40

#define INPUT_SOCKET "/tmp/q2-2048.input.sock"
#define SAVE_FILE "/mnt/mmc/.2048/state"
#define SAVE_TEMP "/mnt/mmc/.2048/state.new"
#define SAVE_VERSION 0x51323031u

struct savefile {
    u32 version;
    /* Keep the existing card save format: tile values are stored as words. */
    u32 tile[CELLS];
    u32 score, best, won, continued;
};

static struct app app;

void *memset(void *dest, int value, __SIZE_TYPE__ length)
{
    unsigned char *p = dest;
    while (length--)
        *p++ = (unsigned char)value;
    return dest;
}

void *memcpy(void *dest, const void *src, __SIZE_TYPE__ length)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (length--)
        *d++ = *s++;
    return dest;
}

static void load(void)
{
    struct savefile file;
    int fd = (int)sc(SYS_open, (long)SAVE_FILE, O_CLOEXEC, 0, 0, 0, 0);
    if (fd >= 0) {
        long size = sc(SYS_read, fd, (long)&file, sizeof file, 0, 0, 0);
        sc(SYS_close, fd, 0, 0, 0, 0, 0);
        if (size == (long)sizeof file && file.version == SAVE_VERSION) {
            struct game game = { .score = file.score, .best = file.best,
                                 .won = file.won, .continued = file.continued };
            for (int i = 0; i < CELLS; ++i) {
                u32 tile = file.tile[i];
                if (tile == 1 || tile > (1u << MAX_EXPONENT) || (tile & (tile - 1))) {
                    game.tile[i] = MAX_EXPONENT + 1;
                    break;
                }
                while (tile > 1) {
                    ++game.tile[i];
                    tile >>= 1;
                }
            }
            if (game_valid(&game)) {
                app.game = game;
                if (game_can_move(&app.game))
                    return;
            }
        }
    }
    game_new(&app.game, &app.rng);
}

static void save(void)
{
    const struct game *game = &app.game;
    struct savefile file = { .version = SAVE_VERSION, .score = game->score, .best = game->best,
                             .won = game->won, .continued = game->continued };
    for (int i = 0; i < CELLS; ++i)
        file.tile[i] = game->tile[i] ? 1u << game->tile[i] : 0;
    int fd = (int)sc(SYS_open, (long)SAVE_TEMP, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
                     0644, 0, 0, 0);
    if (fd < 0) {
        app.save_failed = 1;
        return;
    }
    int ok = sc(SYS_write, fd, (long)&file, sizeof file, 0, 0, 0) == (long)sizeof file &&
             sc(SYS_fsync, fd, 0, 0, 0, 0, 0) == 0;
    if (sc(SYS_close, fd, 0, 0, 0, 0, 0))
        ok = 0;
    app.save_failed = !ok || sc(SYS_rename, (long)SAVE_TEMP, (long)SAVE_FILE, 0, 0, 0, 0) != 0;
    if (app.save_failed)
        sc(SYS_unlink, (long)SAVE_TEMP, 0, 0, 0, 0, 0);
}

static int bind_input(void)
{
    struct { unsigned short family; char path[108]; } address = {1, INPUT_SOCKET};
    int fd = (int)sc(SYS_socket, 1, 1, 0, 0, 0, 0); /* AF_UNIX, MIPS SOCK_DGRAM */
    if (fd < 0)
        return -1;
    sc(SYS_unlink, (long)INPUT_SOCKET, 0, 0, 0, 0, 0);
    if (!sc(SYS_bind, fd, (long)&address, sizeof address, 0, 0, 0))
        return fd;
    sc(SYS_close, fd, 0, 0, 0, 0, 0);
    return -1;
}

int q2_main(void)
{
    unsigned var[40], fix[17];
    int fb = (int)sc(SYS_open, (long)"/dev/fb0", O_RDWR | O_CLOEXEC, 0, 0, 0, 0);
    if (fb < 0 || sc(SYS_ioctl, fb, FBIOGET_VSCREENINFO, (long)var, 0, 0, 0) ||
        sc(SYS_ioctl, fb, FBIOGET_FSCREENINFO, (long)fix, 0, 0, 0))
        return 1;
    unsigned stride = fix[11] / sizeof(u32), front = var[5];
    if (var[0] != HEIGHT || var[1] != WIDTH || var[4] || var[6] != 32 ||
        var[8] != 16 || fix[11] % sizeof(u32) || stride < HEIGHT)
        return 1;
    unsigned rows = fix[5] / fix[11];
    if (rows > var[3])
        rows = var[3];
    if (rows < WIDTH || front > rows - WIDTH)
        return 1;
    long mapped = sc(SYS_mmap, 0, (long)fix[5], 3, 1, fb, 0); /* RW, MAP_SHARED */
    if (mapped < 0 && mapped >= -4095)
        return 1;
    u32 *memory = (u32 *)mapped;
    int input = bind_input();
    if (input < 0)
        return 1;

    long now[2] = {0};
    sc(SYS_clock_gettime, 1, (long)now, 0, 0, 0, 0); /* CLOCK_MONOTONIC */
    app.rng = (u32)now[0] ^ (u32)now[1];
    load();
    app_start(&app);
    unsigned visible = front;
    int can_pan = rows >= 2 * WIDTH && (front == 0 || front == WIDTH);
    struct { int fd; short events, revents; } poll_input = {input, 1, 0};
    int redraw = 1;
    while (app.mode != EXIT) {
        if (redraw) {
            if (can_pan) {
                unsigned back = visible ? 0 : WIDTH;
                render(&app, memory + back * stride, stride);
                var[5] = back;
                if (sc(SYS_ioctl, fb, FBIOPAN_DISPLAY, (long)var, 0, 0, 0) == 0)
                    visible = back;
                else
                    can_pan = 0;
            }
            /* Some framebuffer modes reject page flips. Draw on the visible page. */
            if (!can_pan)
                render(&app, memory + visible * stride, stride);
            redraw = 0;
        }

        long ready = sc(SYS_poll, (long)&poll_input, 1, -1, 0, 0, 0);
        if (ready == 0 || ready == -4) /* No events or EINTR. */
            continue;
        if (ready < 0 || !(poll_input.revents & 1))
            break;
        char message[64];
        long bytes = sc(SYS_recv, input, (long)message, sizeof message - 1, MSG_DONTWAIT, 0, 0);
        if (bytes > 0) {
            message[bytes] = '\0';
            enum mode previous_mode = app.mode;
            enum direction previous_direction = app.direction;
            int previous_choice = app.choice;
            int changed = app_input(&app, message);
            if (changed)
                save();
            redraw = changed || app.mode != previous_mode ||
                     app.direction != previous_direction || app.choice != previous_choice;
        }
    }

    if (visible != front) {
        var[5] = front;
        sc(SYS_ioctl, fb, FBIOPAN_DISPLAY, (long)var, 0, 0, 0);
    }
    sc(SYS_close, input, 0, 0, 0, 0, 0);
    sc(SYS_unlink, (long)INPUT_SOCKET, 0, 0, 0, 0, 0);
    sc(SYS_munmap, mapped, (long)fix[5], 0, 0, 0, 0);
    sc(SYS_close, fb, 0, 0, 0, 0, 0);
    return app.mode == EXIT ? 0 : 1;
}
