#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

#include "odometria.h"

// Estado da grade (celulas) + publicacao unificada de telemetria.
void robot_state_init(pose_t *pose);

void robot_state_avancar_celula(void);

int robot_state_linha(void);

int robot_state_coluna(void);

// Publica posicao na grade, orientacao atual e mensagem de log para o frontend.
void robot_state_publicar(float nivel_bateria, float velocidade, const char *mensagem);

#endif
