#ifndef ODOMETRIA_H
#define ODOMETRIA_H

#define L_CELULA_CM 0.180 // lado da celula do labirinto [m]
#define RAIO_R 0.034 // raio das rodas [m]
#define W_EIXOS 0.110 // distancia entre os eixos [m]

/* typedef enum{
    ID4X4 = 0,
    ID8X8 = 1,
    ID16X16 = 2,
} lab_id_t; */

typedef enum{
    NORTE = 0,
    LESTE = 1,
    SUL =   2,
    OESTE = 3
} orientacao_t;

typedef struct{
    float theta;
    float x;
    float y;
    orientacao_t orientacao;
} pose_t;

void odometria_pos_init(pose_t *pose, float x0, float y0, orientacao_t orientacao_inicial);

void odometria_update_xy(pose_t *pose, float deslocamento);

void odometria_mudar_sentido(pose_t *pose, bool dir);

// void odometria_set_maze_id();

orientacao_t odometria_get_orientacao();

float odometria_get_x();

float odometria_get_y();

const char* odometria_orientacao_string(orientacao_t o);

#endif
