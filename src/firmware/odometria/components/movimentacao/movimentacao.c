#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "encoder.h"
#include "m_driver.h"
#include "odometria.h"
#include "movimentacao.h"

static const char *TAG = "movimentacao";

// Tempo maximo de seguranca para cada primitiva de movimento [s].
// Se o deslocamento alvo nao for atingido nesse tempo (encoder travado,
// roda presa, fio solto, ruido impedindo a contagem de pulsos), a funcao
// freia e aborta em vez de manter os motores ligados indefinidamente.
// Atravessar 1 celula (15,6 cm) ou girar 90 graus leva ~1-2 s; estes
// limites dao folga sem deixar o robo "fugir".
#define MOVE_CELL_TIMEOUT_S 3.0f
#define TURN_TIMEOUT_S      2.5f

//funcoes especificas

void mouse_movefwd(motor_t *motorR, motor_t *motorL){
    gpio_set_level(SEL, 0); // garantir que o freio esta desativado

    motor_set_speed(motorR, MOTOR_FWD_SPD);
    motor_set_speed(motorL, MOTOR_FWD_SPD);
    ESP_LOGI(TAG, "comando andar para frente");
}

void mouse_movebwd(motor_t *motorR, motor_t *motorL){
    gpio_set_level(SEL, 0); // garantir que o freio esta desativado
    
    motor_set_speed(motorR, -MOTOR_BWD_SPD);
    motor_set_speed(motorL, -MOTOR_BWD_SPD);
    ESP_LOGI(TAG, "comando andar para tras");
}

void mouse_spin(motor_t *motorR, motor_t *motorL, bool sentido){
    gpio_set_level(SEL, 0); // garantir que o freio esta desativado

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

void mouse_coast(motor_t *motorR, motor_t *motorL){
    motor_stop(motorR);
    motor_stop(motorL);
    ESP_LOGI(TAG, "comando coast");
}

//freio ativo
void mouse_break(motor_t *motorR, motor_t *motorL){
    mouse_coast(motorR, motorL); // primeiramente cessar o pwm enviado ao driver

    gpio_set_level(SEL, 1); // pino SEL do multiplexador => 1;
                            // entradas do driver em nivel alto (freio ativo)
    ESP_LOGI(TAG, "comando frear");
}

float mouse_get_linear_speed(encoder_t *encR, encoder_t *encL, float dt){
    float   spdR = encoder_get_v(encR, dt, RAIO_R),
            spdL = encoder_get_v(encL, dt, RAIO_R);
        
    float lin_spd = (spdR + spdL)/2;

    return lin_spd;
}

//funcoes de movimentacao

void movimentacao_move_cell(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
    float time, previoustime, meanspeed;

    previoustime = odometria_get_segundos();

    float target = L_CELULA_CM;

    float desloc = 0,
          desloc_R = 0,
          desloc_L = 0;

    // int motor_speed = BASE_SPD;

    mouse_movefwd(mtrR, mtrL);

    bool timeout = false;

    while(desloc < target){

        if (odometria_get_segundos() - previoustime > MOVE_CELL_TIMEOUT_S) {
            timeout = true;
            break;
        }

        /* //possivel mudança: controle bem rudimentar (com risco de explodir a velocidade)
        if (desloc_R-desloc_L >= 5) {
            motor_speed = motor_speed+5;
            motor_set_speed(mtrL, motor_speed);
        } else if (desloc_R-desloc_L <= 5) {
            motor_speed = motor_speed+5;
            motor_set_speed(mtrR, motor_speed) 
        }
        */

        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);

        desloc = (desloc_R + desloc_L)/2.0f;

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    mouse_break(mtrR, mtrL);

    if (timeout) {
        ESP_LOGW(TAG, "TIMEOUT ao andar 1 celula apos %.1fs (desloc=%.3f de %.3f). "
                      "Encoder travado/roda presa? Abortando movimento.",
                 MOVE_CELL_TIMEOUT_S, desloc, target);
    }

    time = odometria_get_segundos() - previoustime;

    meanspeed = mouse_get_linear_speed(encR, encL, time);

    pos->cell_count++;

    odometria_update_vm(pos, meanspeed);

    odometria_update_xy(pos, desloc);
    
    ESP_LOGI(TAG, "andou 1 celula, posicao: (%f, %f), desloc: %f, velocidade media (m/s):%f",pos->x, pos->y, desloc, meanspeed);
}

void movimentacao_turn_clws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
    float target_theta = M_PI/2.0f;

        float theta = 0,
              desloc_R = 0,
              desloc_L = 0;

    float t_inicio = odometria_get_segundos();
    bool timeout = false;

    mouse_spin(mtrR, mtrL, 1);

    while (theta < target_theta){
        if (odometria_get_segundos() - t_inicio > TURN_TIMEOUT_S) {
            timeout = true;
            break;
        }

        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);

        theta = (desloc_R + desloc_L)/W_EIXOS;

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    mouse_break(mtrR, mtrL);

    if (timeout) {
        ESP_LOGW(TAG, "TIMEOUT ao virar a direita apos %.1fs (theta=%.3f de %.3f). "
                      "Encoder travado/roda presa? Abortando giro.",
                 TURN_TIMEOUT_S, theta, target_theta);
    }

    odometria_mudar_sentido(pos, 1);
    ESP_LOGI(TAG, "virou 90 graus para a direita. orientacao atual: %s, angulo(rad): %f", odometria_orientacao_string(pos->orientacao), theta);
}

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
        float target_theta = M_PI/2.0f;

        float theta = 0,
              desloc_R = 0,
              desloc_L = 0;

    float t_inicio = odometria_get_segundos();
    bool timeout = false;

    mouse_spin(mtrR, mtrL, 0);

    while (theta < target_theta){
        if (odometria_get_segundos() - t_inicio > TURN_TIMEOUT_S) {
            timeout = true;
            break;
        }

        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);

        theta = (desloc_R + desloc_L)/W_EIXOS;

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    mouse_break(mtrR, mtrL);

    if (timeout) {
        ESP_LOGW(TAG, "TIMEOUT ao virar a esquerda apos %.1fs (theta=%.3f de %.3f). "
                      "Encoder travado/roda presa? Abortando giro.",
                 TURN_TIMEOUT_S, theta, target_theta);
    }

    odometria_mudar_sentido(pos, 0);
    ESP_LOGI(TAG, "virou 90 graus para a esquerda. orientacao atual: %s, angulo(rad): %f", odometria_orientacao_string(pos->orientacao), theta);
}