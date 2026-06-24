#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "encoder.h"
#include "m_driver.h"
#include "odometria.h"
#include "calibracao.h"
#include "movimentacao.h"

static const char *TAG = "movimentacao";

// --------------------------------------------------------------------------
// parametros do controle em malha fechada
// --------------------------------------------------------------------------
// distancia linear percorrida pela roda por pulso do encoder [m]
#define DIST_POR_PULSO   ((2.0f * (float) M_PI * RAIO_R) / ENCODER_PULSOS_POR_VOLTA)

// limites de PWM para o controle (motor_set_speed ignora |v| < 25)
#define SPD_MIN          30
#define SPD_MAX          95

// saturacao da correcao de rumo para evitar solavancos
#define CORRECAO_MAX     30.0f

// periodo de amostragem do controle [ms]
#define CTRL_DT_MS       30

// calibracao atual (neutra por padrao: trim 1.0 e ganhos padrao)
static calib_t g_cal = {
    .trim_dir = 1.0f,
    .trim_esq = 1.0f,
    .kp = CALIB_KP_PADRAO,
    .ki = CALIB_KI_PADRAO,
    .valido = false,
};

void movimentacao_aplicar_calibracao(const calib_t *cal)
{
    if (cal == NULL) {
        return;
    }
    g_cal = *cal;
    ESP_LOGI(TAG, "calibracao aplicada: trim_dir=%.3f trim_esq=%.3f kp=%.2f ki=%.2f",
             g_cal.trim_dir, g_cal.trim_esq, g_cal.kp, g_cal.ki);
}

// satura um valor de PWM no intervalo util e devolve int8_t.
static int8_t satura_spd(float v)
{
    if (v > 0) {
        if (v > SPD_MAX) v = SPD_MAX;
        if (v < SPD_MIN) v = SPD_MIN;
    } else if (v < 0) {
        if (v < -SPD_MAX) v = -SPD_MAX;
        if (v > -SPD_MIN) v = -SPD_MIN;
    }
    return (int8_t) v;
}

static float satura_correcao(float c)
{
    if (c >  CORRECAO_MAX) return  CORRECAO_MAX;
    if (c < -CORRECAO_MAX) return -CORRECAO_MAX;
    return c;
}

//funcoes especificas

void mouse_movefwd(motor_t *motorR, motor_t *motorL){
    motor_set_speed(motorR, MOTOR_FWD_SPD);
    motor_set_speed(motorL, MOTOR_FWD_SPD);
    ESP_LOGI(TAG, "comando andar para frente");
}

void mouse_movebwd(motor_t *motorR, motor_t *motorL){
    motor_set_speed(motorR, -MOTOR_BWD_SPD);
    motor_set_speed(motorL, -MOTOR_BWD_SPD);
    ESP_LOGI(TAG, "comando andar para tras");
}

void mouse_spin(motor_t *motorR, motor_t *motorL, bool sentido){
    if(sentido){
        motor_set_speed(motorR, -MOTOR_BWD_SPD);
        motor_set_speed(motorL, MOTOR_BWD_SPD);
        ESP_LOGI(TAG, "comando virar sentido horario");
    } else {
        motor_set_speed(motorR, MOTOR_BWD_SPD);
        motor_set_speed(motorL, -MOTOR_BWD_SPD);
        ESP_LOGI(TAG, "comando virar sentido anti-horario");
    }
}

void mouse_break(motor_t *motorR, motor_t *motorL){
    motor_stop(motorR);
    motor_stop(motorL);
    ESP_LOGI(TAG, "comando frear");
}

//funcoes de movimentacao

void movimentacao_move_cell(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
    const float target = L_CELULA_CM;

    // zera as contagens e amostra de forma nao-destrutiva durante o trajeto
    encoder_clean(encR);
    encoder_clean(encL);

    float base_R = (float) MOTOR_FWD_SPD * g_cal.trim_dir;
    float base_L = (float) MOTOR_FWD_SPD * g_cal.trim_esq;

    float integral = 0.0f;
    float desloc = 0.0f;

    motor_set_speed(mtrR, satura_spd(base_R));
    motor_set_speed(mtrL, satura_spd(base_L));

    while (desloc < target) {
        int cR = encoder_peek_count(encR);
        int cL = encoder_peek_count(encL);

        // erro de rumo: diferenca ACUMULADA de pulsos entre as rodas.
        // como ja e a integral da diferenca de velocidade, um ganho
        // proporcional sobre ele se comporta como um "segura-rumo".
        float erro = (float) (cR - cL);
        integral += erro;

        float correcao = satura_correcao(g_cal.kp * erro + g_cal.ki * integral);

        // se a direita anda mais (erro > 0), freia a direita e acelera a esquerda
        motor_set_speed(mtrR, satura_spd(base_R - correcao));
        motor_set_speed(mtrL, satura_spd(base_L + correcao));

        desloc = ((float) (cR + cL) / 2.0f) * DIST_POR_PULSO;

        vTaskDelay(pdMS_TO_TICKS(CTRL_DT_MS));
    }

    mouse_break(mtrR, mtrL);

    odometria_update_xy(pos, desloc);
    ESP_LOGI(TAG, "andou 1 celula, desloc: %f m (correcao de rumo ativa)", desloc);
}

// Gira 90 graus no proprio eixo balanceando as duas rodas pelo encoder.
// sentido_horario: true -> horario (direita); false -> anti-horario (esquerda).
static void girar_90(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL,
                     pose_t *pos, bool sentido_horario)
{
    const float target_theta = M_PI / 2.0f;

    encoder_clean(encR);
    encoder_clean(encL);

    // magnitude base de cada roda, ja compensada pelo trim
    float mag_R = (float) MOTOR_BWD_SPD * g_cal.trim_dir;
    float mag_L = (float) MOTOR_BWD_SPD * g_cal.trim_esq;

    // sinal de cada roda conforme o sentido do giro
    float sgn_R = sentido_horario ? -1.0f : 1.0f;
    float sgn_L = sentido_horario ?  1.0f : -1.0f;

    float theta = 0.0f;

    motor_set_speed(mtrR, satura_spd(sgn_R * mag_R));
    motor_set_speed(mtrL, satura_spd(sgn_L * mag_L));

    while (theta < target_theta) {
        // no giro as duas rodas contam em modulo; igualar os modulos
        // mantem o giro centrado no eixo do carrinho.
        int aR = abs(encoder_peek_count(encR));
        int aL = abs(encoder_peek_count(encL));

        float erro = (float) (aR - aL);
        float correcao = satura_correcao(g_cal.kp * erro);

        // freia a roda que esta girando mais que a outra
        motor_set_speed(mtrR, satura_spd(sgn_R * (mag_R - correcao)));
        motor_set_speed(mtrL, satura_spd(sgn_L * (mag_L - correcao)));

        float desloc_R = aR * DIST_POR_PULSO;
        float desloc_L = aL * DIST_POR_PULSO;
        theta = (desloc_R + desloc_L) / W_EIXOS;

        vTaskDelay(pdMS_TO_TICKS(CTRL_DT_MS));
    }

    mouse_break(mtrR, mtrL);
    odometria_mudar_sentido(pos, sentido_horario ? 1 : 0);
}

void movimentacao_turn_clws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
    girar_90(mtrR, mtrL, encR, encL, pos, true);
    ESP_LOGI(TAG, "virou 90 graus para a direita. orientacao atual: %s",
             odometria_orientacao_string(pos->orientacao));
}

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
    girar_90(mtrR, mtrL, encR, encL, pos, false);
    ESP_LOGI(TAG, "virou 90 graus para a esquerda. orientacao atual: %s",
             odometria_orientacao_string(pos->orientacao));
}
