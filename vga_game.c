/*
 * File: vga_game.c
 * Description: Side-scrolling obstacle jump game for CPulator (DE1-SoC)
 * Controls: Click inside the PS/2 keyboard window and press SPACEBAR to jump!
 */

#define VGA_BASE     0xC8000000
#define PS2_BASE     0xFF200100

#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240

#define COLOR_SKY      0x0313
#define COLOR_GROUND   0x528A
#define COLOR_PLAYER   0x07E0
#define COLOR_OBSTACLE 0xF800

#define GROUND_Y      200
#define PLAYER_X       40
#define BOX_SIZE       16
#define GRAVITY         1
#define JUMP_FORCE    -12

#define MAX_OBSTACLES   5

typedef struct {
    int x;
    int y;
    int active;
} Obstacle;

int player_y = GROUND_Y - BOX_SIZE;
int player_velocity_y = 0;
int is_jumping = 0;

Obstacle obstacles[MAX_OBSTACLES];
int obstacle_speed = 4;
int score = 0;
int game_over = 0;
int spawn_timer = 0;

void plot_pixel(int x, int y, short int color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        volatile short int *pixel_address = (volatile short int *)(VGA_BASE + (y << 10) + (x << 1));
        *pixel_address = color;
    }
}

void draw_background() {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        short int c = (y >= GROUND_Y) ? COLOR_GROUND : COLOR_SKY;
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            plot_pixel(x, y, c);
        }
    }
}

void draw_rect(int x, int y, int w, int h, short int color) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            plot_pixel(x + j, y + i, color);
        }
    }
}

void wait_frame() {
    volatile int delay = 30000;
    while (delay > 0) {
        delay--;
    }
}

int check_collision_box(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    if (x1 < x2 + w2 &&
        x1 + w1 > x2 &&
        y1 < y2 + h2 &&
        y1 + h1 > y2) {
        return 1;
    }
    return 0;
}

int check_collision() {
    int i;
    for (i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            if (check_collision_box(PLAYER_X, player_y, BOX_SIZE, BOX_SIZE,
                                    obstacles[i].x, obstacles[i].y, BOX_SIZE, BOX_SIZE)) {
                return 1;
            }
        }
    }
    return 0;
}

void reset_game() {
    int i;
    player_y = GROUND_Y - BOX_SIZE;
    player_velocity_y = 0;
    is_jumping = 0;
    obstacle_speed = 4;
    score = 0;
    game_over = 0;
    spawn_timer = 0;

    for (i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].active = 0;
    }

    obstacles[0].x = SCREEN_WIDTH;
    obstacles[0].y = GROUND_Y - BOX_SIZE;
    obstacles[0].active = 1;
}

void spawn_obstacle(int index, int x_pos) {
    obstacles[index].x = x_pos;
    obstacles[index].y = GROUND_Y - BOX_SIZE;
    obstacles[index].active = 1;
}

void update_difficulty() {
    obstacle_speed = 4 + (score / 3);
    if (obstacle_speed > 12) {
        obstacle_speed = 12;
    }
}

void poll_ps2(int *space_pressed) {
    volatile int *ps2_ptr = (volatile int *)PS2_BASE;
    int data;
    unsigned char break_code = 0;
    unsigned char extended = 0;

    while ((data = *ps2_ptr) & 0x00008000) {
        unsigned char key = data & 0xFF;

        if (key == 0xE0) {
            extended = 1;
        } else if (key == 0xF0) {
            break_code = 1;
        } else {
            if (!break_code && !extended && key == 0x29) {
                *space_pressed = 1;
            }
            break_code = 0;
            extended = 0;
        }
    }
}

int main() {
    int i;

    reset_game();
    draw_background();

    while (1) {
        int space_pressed = 0;

        poll_ps2(&space_pressed);

        if (space_pressed) {
            if (game_over) {
                reset_game();
            } else if (!is_jumping) {
                player_velocity_y = JUMP_FORCE;
                is_jumping = 1;
            }
        }

        if (!game_over) {
            player_y += player_velocity_y;
            player_velocity_y += GRAVITY;

            if (player_y >= GROUND_Y - BOX_SIZE) {
                player_y = GROUND_Y - BOX_SIZE;
                player_velocity_y = 0;
                is_jumping = 0;
            }

            spawn_timer++;
            if (spawn_timer >= 45) {
                for (i = 0; i < MAX_OBSTACLES; i++) {
                    if (!obstacles[i].active) {
                        obstacles[i].x = SCREEN_WIDTH + 40 + (score * 2);
                        obstacles[i].y = GROUND_Y - BOX_SIZE;
                        obstacles[i].active = 1;
                        break;
                    }
                }
                spawn_timer = 0;
            }

            update_difficulty();

            for (i = 0; i < MAX_OBSTACLES; i++) {
                if (obstacles[i].active) {
                    obstacles[i].x -= obstacle_speed;

                    if (obstacles[i].x < -BOX_SIZE) {
                        obstacles[i].active = 0;
                        score++;
                        update_difficulty();
                    }
                }
            }

            if (check_collision()) {
                game_over = 1;
            }

            draw_background();

            for (i = 0; i < MAX_OBSTACLES; i++) {
                if (obstacles[i].active) {
                    draw_rect(obstacles[i].x, obstacles[i].y, BOX_SIZE, BOX_SIZE, COLOR_OBSTACLE);
                }
            }

            draw_rect(PLAYER_X, player_y, BOX_SIZE, BOX_SIZE, COLOR_PLAYER);
        } else {
            draw_background();
            for (i = 0; i < MAX_OBSTACLES; i++) {
                if (obstacles[i].active) {
                    draw_rect(obstacles[i].x, obstacles[i].y, BOX_SIZE, BOX_SIZE, COLOR_OBSTACLE);
                }
            }
            draw_rect(PLAYER_X, player_y, BOX_SIZE, BOX_SIZE, COLOR_PLAYER);
        }

        wait_frame();
    }

    return 0;
}
