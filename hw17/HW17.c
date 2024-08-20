#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <GL/glut.h>

#define SIZE 4

int board[SIZE][SIZE];
int score = 0;

void initialize_board() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = 0;
        }
    }
    add_random_tile();
    add_random_tile();
}

void add_random_tile() {
    int empty_tiles[SIZE * SIZE][2];
    int empty_count = 0;

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == 0) {
                empty_tiles[empty_count][0] = i;
                empty_tiles[empty_count][1] = j;
                empty_count++;
            }
        }
    }

    if (empty_count > 0) {
        int r = rand() % empty_count;
        board[empty_tiles[r][0]][empty_tiles[r][1]] = (rand() % 10) < 9 ? 2 : 4;
    }
}

bool slide_row(int row[SIZE]) {
    bool moved = false;
    for (int i = 0; i < SIZE - 1; i++) {
        if (row[i] == 0 && row[i + 1] != 0) {
            row[i] = row[i + 1];
            row[i + 1] = 0;
            moved = true;
        }
    }
    return moved;
}

bool combine_row(int row[SIZE]) {
    bool moved = false;
    for (int i = 0; i < SIZE - 1; i++) {
        if (row[i] != 0 && row[i] == row[i + 1]) {
            row[i] *= 2;
            score += row[i];
            row[i + 1] = 0;
            moved = true;
        }
    }
    return moved;
}

bool slide_and_combine_row(int row[SIZE]) {
    bool moved = false;
    for (int i = 0; i < SIZE - 1; i++) {
        if (slide_row(row)) {
            moved = true;
        }
    }
    if (combine_row(row)) {
        moved = true;
    }
    for (int i = 0; i < SIZE - 1; i++) {
        if (slide_row(row)) {
            moved = true;
        }
    }
    return moved;
}

bool move_left() {
    bool moved = false;
    for (int i = 0; i < SIZE; i++) {
        if (slide_and_combine_row(board[i])) {
            moved = true;
        }
    }
    return moved;
}

void rotate_board() {
    int temp[SIZE][SIZE];
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            temp[i][j] = board[j][SIZE - i - 1];
        }
    }
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = temp[i][j];
        }
    }
}

bool move_right() {
    rotate_board();
    rotate_board();
    bool moved = move_left();
    rotate_board();
    rotate_board();
    return moved;
}

bool move_up() {
    rotate_board();
    rotate_board();
    rotate_board();
    bool moved = move_left();
    rotate_board();
    return moved;
}

bool move_down() {
    rotate_board();
    bool moved = move_left();
    rotate_board();
    rotate_board();
    rotate_board();
    return moved;
}

bool can_move() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == 0) {
                return true;
            }
            if (i < SIZE - 1 && board[i][j] == board[i + 1][j]) {
                return true;
            }
            if (j < SIZE - 1 && board[i][j] == board[i][j + 1]) {
                return true;
            }
        }
    }
    return false;
}

void draw_tile(int x, int y, int value) {
    float colors[12][3] = {
        {0.93f, 0.89f, 0.85f}, // 0
        {0.93f, 0.89f, 0.85f}, // 2
        {0.93f, 0.88f, 0.79f}, // 4
        {0.95f, 0.69f, 0.47f}, // 8
        {0.96f, 0.58f, 0.39f}, // 16
        {0.96f, 0.48f, 0.37f}, // 32
        {0.96f, 0.37f, 0.23f}, // 64
        {0.93f, 0.81f, 0.45f}, // 128
        {0.93f, 0.80f, 0.39f}, // 256
        {0.93f, 0.78f, 0.31f}, // 512
        {0.93f, 0.76f, 0.25f}, // 1024
        {0.93f, 0.74f, 0.18f}  // 2048
    };

    int log2_value = 0;
    while (value > 1) {
        value /= 2;
        log2_value++;
    }

    glColor3f(colors[log2_value][0], colors[log2_value][1], colors[log2_value][2]);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + 1, y);
    glVertex2f(x + 1, y + 1);
    glVertex2f(x, y + 1);
    glEnd();

    if (board[y][x] != 0) {
        char str[8];
        snprintf(str, sizeof(str), "%d", board[y][x]);
        glColor3f(0.0f, 0.0f, 0.0f);
        glRasterPos2f(x + 0.5f - 0.1f * strlen(str), y + 0.5f - 0.1f);
        for (char *c = str; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            draw_tile(j, i, board[i][j]);
        }
    }

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    bool moved = false;

    switch (key) {
        case 'w':
        case 'W':
            moved = move_up();
            break;
        case 's':
        case 'S':
            moved = move_down();
            break;
        case 'a':
        case 'A':
            moved = move_left();
            break;
        case 'd':
        case 'D':
            moved = move_right();
            break;
        case 27: // Esc
            exit(0);
            break;
        default:
            return;
    }

    if (moved) {
        add_random_tile();
    }

    if (!can_move()) {
        printf("Game Over! Final Score: %d\n", score);
        exit(0);
    }

    glutPostRedisplay();
}

void special_input(int key, int x, int y) {
    bool moved = false;

    switch (key) {
        case GLUT_KEY_UP:
            moved = move_up();
            break;
        case GLUT_KEY_DOWN:
            moved = move_down();
            break;
        case GLUT_KEY_LEFT:
            moved = move_left();
            break;
        case GLUT_KEY_RIGHT:
            moved = move_right();
            break;
        default:
            return;
    }

    if (moved) {
        add_random_tile();
    }

    if (!can_move()) {
        printf("Game Over! Final Score: %d\n", score);
        exit(0);
    }

    glutPostRedisplay();
}

int main(int argc, char** argv) {
    srand(time(NULL));
    initialize_board();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(400, 400);
    glutCreateWindow("2048 Game");

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, SIZE, 0.0, SIZE, -1.0, 1.0);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_input); // Добавление обработки клавиш-стрелок

    glutMainLoop();

    return 0;
}
