#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "m_driver.h"
#include "encoder.h"
#include "odometria.h"
#include "movimentacao.h"
#include "calibracao.h"
#include "seletor_modo.h"

static const char *TAG = "main";

encoder_t encoderR;
encoder_t encoderL;

pose_t pose;

motor_t motorR = { .pwm_gpio1 = PWM_R1, .pwm_gpio2 = PWM_R2 };
motor_t motorL = { .pwm_gpio1 = PWM_L1, .pwm_gpio2 = PWM_L2 };

// task que executa a navegacao; e destruida/recriada pelo supervisor
static TaskHandle_t mission_handle = NULL;
// task que reage a interrupcao do botao
static TaskHandle_t supervisor_handle = NULL;

// Zera odometria e encoders: o carrinho volta logicamente ao ponto de partida.
static void resetar_estado(void)
{
    encoder_clean(&encoderR);
    encoder_clean(&encoderL);
    odometria_pos_init(&pose, 0, 0, NORTE);
}

// missao principal realizada no teste:
// avancar 1 bloco -> virar 90o horario -> virar 90o antihorario
static void start_mission(void)
{
    ESP_LOGI(TAG, "iniciando missao...");

    //DEBUG
    ESP_LOGI(TAG, "Stack livre: %u",
         uxTaskGetStackHighWaterMark(NULL));

    //step 1
    movimentacao_move_cell(&motorR, &motorL, &encoderR, &encoderL, &pose);

    vTaskDelay(pdMS_TO_TICKS(1000));

    encoder_clean(&encoderR);
    encoder_clean(&encoderL);

    //DEBUG
    ESP_LOGI(TAG, "Stack livre: %u",
         uxTaskGetStackHighWaterMark(NULL));

    //step 2
    movimentacao_turn_clws(&motorR, &motorL, &encoderR, &encoderL, &pose);

    vTaskDelay(pdMS_TO_TICKS(1000));

    encoder_clean(&encoderR);
    encoder_clean(&encoderL);

    //DEBUG
    ESP_LOGI(TAG, "Stack livre: %u",
         uxTaskGetStackHighWaterMark(NULL));

    //step 3
    movimentacao_turn_ctclws(&motorR, &motorL, &encoderR, &encoderL, &pose);

    vTaskDelay(pdMS_TO_TICKS(1000));

    encoder_clean(&encoderR);
    encoder_clean(&encoderL);

    ESP_LOGI(TAG, "MISSAO CUMPRIDA COM EXITO! =D");
}

// Missao de navegacao parametrizada pelo modo atual (lado 4 ou 8).
// Aqui no futuro entra o flood-fill; por enquanto executa a rotina de teste
// de movimento (start_mission), que valida andar/girar do carrinho.
static void mission_task(void *arg)
{
    uint8_t lado = seletor_modo_lado();
    ESP_LOGI(TAG, "iniciando missao no modo %dx%d", lado, lado);

    start_mission();

    ESP_LOGI(TAG, "missao %dx%d finalizada", lado, lado);

    // a propria task se encerra ao terminar a missao
    mission_handle = NULL;
    vTaskDelete(NULL);
}

static void iniciar_missao(void)
{
    if (mission_handle == NULL) {
        xTaskCreate(mission_task, "mission_task", 4096, NULL, 5, &mission_handle);
    }
}

// Supervisor: dorme ate ser acordado pela ISR do botao. Ao acordar ele
// "reinicia" o carrinho de forma controlada, sem reiniciar o chip:
//   1. mata a missao em andamento (interrompe tudo que o carrinho faz)
//   2. trava os motores imediatamente
//   3. alterna o modo (4x4 <-> 8x8) e sinaliza no buzzer
//   4. zera odometria/encoders (volta ao ponto de partida)
//   5. recria a missao ja no novo modo
static void supervisor_task(void *arg)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "botao pressionado: interrompendo e trocando de modo");

        if (mission_handle != NULL) {
            vTaskDelete(mission_handle);
            mission_handle = NULL;
        }

        mouse_break(&motorR, &motorL);

        modo_lab_t novo = seletor_modo_toggle();
        seletor_modo_sinalizar(novo);

        resetar_estado();

        iniciar_missao();
    }
}

void app_main(void)
{
    // perifericos de movimentacao
    encoder_init(&encoderR, GPIO_ENC_R);
    encoder_init(&encoderL, GPIO_ENC_L);

    driver_init();

    motor_init(&motorR);
    motor_init(&motorL);

    motorR = (motor_t){
        PWM_R1, PWM_R2,
        motorR.comp1, motorR.comp2,
        motorR.gen1, motorR.gen2
    };

    motorL = (motor_t){
        PWM_L1, PWM_L2,
        motorL.comp1, motorL.comp2,
        motorL.gen1, motorL.gen2
    };

    odometria_pos_init(&pose, 0, 0, NORTE);

    // --- verificacao/validacao dos encoders e compensacao do desvio ---
    // Rode de preferencia com o carrinho suspenso (rodas livres) na 1a vez.
    calib_status_t st = calibracao_validar_encoders(&motorR, &motorL,
                                                    &encoderR, &encoderL);
    ESP_LOGI(TAG, "validacao dos encoders: %s", calibracao_status_str(st));

    calib_t cal;
    if (st == CALIB_OK || st == CALIB_ASSIMETRIA) {
        // encoders confiaveis: mede o trim para igualar as rodas
        calibracao_estimar_trim(&motorR, &motorL, &encoderR, &encoderL, &cal);
    } else {
        // encoder com problema: segue com valores neutros (sem trim),
        // mas o controle de rumo por encoder continua ativo no que der.
        ESP_LOGW(TAG, "encoder com falha (%s): usando calibracao neutra",
                 calibracao_status_str(st));
        calibracao_padrao(&cal);
    }
    movimentacao_aplicar_calibracao(&cal);
    resetar_estado();

    // o supervisor precisa existir antes de ligar a interrupcao do botao,
    // pois e ele quem recebe a notificacao da ISR
    xTaskCreate(supervisor_task, "supervisor_task", 4096, NULL, 6, &supervisor_handle);

    // botao no GPIO 25 (interrupcao) + buzzer no GPIO 33
    seletor_modo_init(supervisor_handle);

    // sinaliza o modo inicial e parte a primeira missao
    seletor_modo_sinalizar(seletor_modo_get());
    iniciar_missao();
}
