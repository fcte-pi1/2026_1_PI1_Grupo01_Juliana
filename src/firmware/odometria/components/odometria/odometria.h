#ifndef ODOMETRIA_H
#define ODOMETRIA_H

#define L_CELULA_CM 0.180 // lado da celula do labirinto [m]
#define RAIO_R 0.034 // raio das rodas [m]
#define W_EIXOS 0.110 // distancia entre os eixos [m]

typedef enum{
    NORTE = 0,
    LESTE = 1,
    SUL =   2,
    OESTE = 3
} orientacao_t;

typedef struct{
    // float theta; atributo a ser adicionado em caso de giroscopio
    float x; // posicao em x
    float y; // posicao em y
    float vm; // velocidade media total
    int cell_count; // conta numero de deslocamentos de celula para auxiliar
                    // no calculo de velocidade media total

    orientacao_t orientacao;
} pose_t;

float odometria_get_segundos();

void odometria_pos_init(pose_t *pose, float x0, float y0, orientacao_t orientacao_inicial);

void odometria_update_xy(pose_t *pose, float deslocamento);

void odometria_update_vm(pose_t *pose, float vm_novo);

void odometria_mudar_sentido(pose_t *pose, bool dir);

// void odometria_set_maze_id();

orientacao_t odometria_get_orientacao();

float odometria_get_x();

float odometria_get_y();

const char* odometria_orientacao_string(orientacao_t o);

#endif
