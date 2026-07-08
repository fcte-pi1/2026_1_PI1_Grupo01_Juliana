#include "robot_state.h"

#include <math.h>
#include <stddef.h>

#include "telemetria.h"

static pose_t *s_pose = NULL;
static int s_linha = 0;
static int s_coluna = 0;

#define GRADE_MAX 15

// Converte pose continua (metros) em indices de celula para o frontend.
static void robot_state_sincronizar_de_pose(void)
{
    if (s_pose == NULL) {
        return;
    }

    s_coluna = (int)lroundf(s_pose->x / L_CELULA_CM);
    s_linha = (int)lroundf(s_pose->y / L_CELULA_CM);

    if (s_coluna < 0) {
        s_coluna = 0;
    }
    if (s_linha < 0) {
        s_linha = 0;
    }
    if (s_coluna > GRADE_MAX) {
        s_coluna = GRADE_MAX;
    }
    if (s_linha > GRADE_MAX) {
        s_linha = GRADE_MAX;
    }
}

void robot_state_init(pose_t *pose)
{
    s_pose = pose;
    robot_state_sincronizar_de_pose();
}

void robot_state_avancar_celula(void)
{
    robot_state_sincronizar_de_pose();
}

int robot_state_linha(void)
{
    return s_linha;
}

int robot_state_coluna(void)
{
    return s_coluna;
}

void robot_state_publicar(float nivel_bateria, float velocidade, const char *mensagem)
{
    robot_state_sincronizar_de_pose();

    const char *orientacao = (s_pose != NULL)
        ? odometria_orientacao_string(s_pose->orientacao)
        : "DESCONHECIDO";

    telemetria_envia(s_linha, s_coluna, nivel_bateria, velocidade, orientacao, mensagem);
}
