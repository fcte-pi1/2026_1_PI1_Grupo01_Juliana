#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define SIZE 16
#define INF 9999

typedef enum { NORTH=0, SOUTH, EAST, WEST } Direction;

typedef struct {
    int walls[4];
    int distance;
} Cell;

Cell maze[SIZE][SIZE];
int visited[SIZE][SIZE];

int dx[4] = {-1, 1, 0, 0};
int dy[4] = {0, 0, 1, -1};

int goalCells[4][2];

int opposite(int d) {
    if (d == NORTH) return SOUTH;
    if (d == SOUTH) return NORTH;
    if (d == EAST) return WEST;
    return EAST;
}

void shuffle(int *array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);

        int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

void initMaze(int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {

            for (int d = 0; d < 4; d++) {
                maze[i][j].walls[d] = 1;
            }

            maze[i][j].distance = INF;
            visited[i][j] = 0;
        }
    }
}

void generateMazeDFS(int x, int y, int size) {
    visited[x][y] = 1;

    int dirs[4] = {NORTH, SOUTH, EAST, WEST};
    shuffle(dirs, 4);

    for (int i = 0; i < 4; i++) {

        int d = dirs[i];

        int nx = x + dx[d];
        int ny = y + dy[d];

        if (nx >= 0 && nx < size &&
            ny >= 0 && ny < size &&
            !visited[nx][ny]) {

            maze[x][y].walls[d] = 0;
            maze[nx][ny].walls[opposite(d)] = 0;

            generateMazeDFS(nx, ny, size);
        }
    }
}

void setupGoalArea(int size) {

    int c1 = (size / 2) - 1;
    int c2 = size / 2;

    goalCells[0][0] = c1;
    goalCells[0][1] = c1;

    goalCells[1][0] = c1;
    goalCells[1][1] = c2;

    goalCells[2][0] = c2;
    goalCells[2][1] = c1;

    goalCells[3][0] = c2;
    goalCells[3][1] = c2;

    maze[c1][c1].walls[EAST] = 0;
    maze[c1][c2].walls[WEST] = 0;

    maze[c2][c1].walls[EAST] = 0;
    maze[c2][c2].walls[WEST] = 0;

    maze[c1][c1].walls[SOUTH] = 0;
    maze[c2][c1].walls[NORTH] = 0;

    maze[c1][c2].walls[SOUTH] = 0;
    maze[c2][c2].walls[NORTH] = 0;
}

int isGoal(int x, int y) {

    for (int i = 0; i < 4; i++) {

        if (goalCells[i][0] == x &&
            goalCells[i][1] == y) {

            return 1;
        }
    }

    return 0;
}

void floodFill(int size) {

    int qx[SIZE * SIZE];
    int qy[SIZE * SIZE];

    int front = 0;
    int rear = 0;

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            maze[i][j].distance = INF;
        }
    }

    for (int i = 0; i < 4; i++) {

        int gx = goalCells[i][0];
        int gy = goalCells[i][1];

        maze[gx][gy].distance = 0;

        qx[rear] = gx;
        qy[rear] = gy;

        rear++;
    }

    while (front < rear) {

        int x = qx[front];
        int y = qy[front];

        front++;

        for (int d = 0; d < 4; d++) {

            if (maze[x][y].walls[d]) {
                continue;
            }

            int nx = x + dx[d];
            int ny = y + dy[d];

            if (nx >= 0 && nx < size &&
                ny >= 0 && ny < size) {

                if (maze[nx][ny].distance >
                    maze[x][y].distance + 1) {

                    maze[nx][ny].distance =
                        maze[x][y].distance + 1;

                    qx[rear] = nx;
                    qy[rear] = ny;

                    rear++;
                }
            }
        }
    }
}

Direction getNextMove(int x, int y, int size) {

    int best = INF;
    Direction bestDir = NORTH;

    for (int d = 0; d < 4; d++) {

        if (maze[x][y].walls[d]) {
            continue;
        }

        int nx = x + dx[d];
        int ny = y + dy[d];

        if (nx >= 0 && nx < size &&
            ny >= 0 && ny < size) {

            if (maze[nx][ny].distance < best) {

                best = maze[nx][ny].distance;
                bestDir = d;
            }
        }
    }

    return bestDir;
}

void printMaze(int rx, int ry, int size) {

    system("clear");

    printf("\n");

    for (int i = 0; i < size; i++) {

        for (int j = 0; j < size; j++) {

            printf("+");

            if (maze[i][j].walls[NORTH]) {
                printf("---");
            } else {
                printf("   ");
            }
        }

        printf("+\n");

        for (int j = 0; j < size; j++) {

            if (maze[i][j].walls[WEST]) {
                printf("|");
            } else {
                printf(" ");
            }

            if (i == rx && j == ry) {

                printf(" R ");

            } else if (isGoal(i, j)) {

                printf(" X ");

            } else {

                printf("   ");
            }
        }

        if (maze[i][size - 1].walls[EAST]) {
            printf("|");
        } else {
            printf(" ");
        }

        printf("\n");
    }

    for (int j = 0; j < size; j++) {

        printf("+");

        if (maze[size - 1][j].walls[SOUTH]) {
            printf("---");
        } else {
            printf("   ");
        }
    }

    printf("+\n");

    printf("\n");
    printf("R = Robo\n");
    printf("X = Area objetivo central 2x2\n");

    usleep(450000);
}

void simulate(int size) {

    int x = 0;
    int y = 0;

    for (int step = 0;
         step < size * size * 4;
         step++) {

        floodFill(size);

        printMaze(x, y, size);

        if (isGoal(x, y)) {

            printf("\nChegou ao centro!\n");
            return;
        }

        Direction d = getNextMove(x, y, size);

        x += dx[d];
        y += dy[d];
    }

    printf("\nNao chegou ao objetivo.\n");
}

int main() {

    srand(time(NULL));

    int size;

    printf("Escolha o tamanho do labirinto (4, 8 ou 16): ");
    scanf("%d", &size);

    if (size != 4 &&
        size != 8 &&
        size != 16) {

        printf("Tamanho invalido.\n");
        return 1;
    }

    initMaze(size);

    generateMazeDFS(0, 0, size);

    setupGoalArea(size);

    simulate(size);

    return 0;
}