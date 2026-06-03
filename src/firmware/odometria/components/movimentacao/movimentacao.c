#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "encoder.h"
#include "m_driver.h"
#include "odometria.h"
#include "movimentacao.h"

static const char *TAG = "movimentacao";

//funcoes especificas

void mouse_movefwd(motor_t *motorR, motor_t *motorL){
    motor_set_speed(motorR, MOTOR_FWD_SPD);
    motor_set_speed(motorL, MOTOR_FWD_SPD);
    ESP_LOGI(TAG, "comando andar para frente");
}

void mouse_movebwd(motor_t *motorR, motor_t *motorL){
    motor_set_speed(motorR, MOTOR_BWD_SPD);
    motor_set_speed(motorL, MOTOR_BWD_SPD);
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
    float target = L_CELULA_CM;

    float desloc = 0,
          desloc_R = 0,
          desloc_L = 0;

    mouse_movefwd(mtrR, mtrL);

    while(desloc < target){
        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);

        desloc = (desloc_R + desloc_L)/2.0f;

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    odometria_update_xy(pos, desloc);
    ESP_LOGI(TAG, "andou 1 celula");
}

void movimentacao_turn_clws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
    float target_theta = M_PI/2.0f;

        float theta = 0,
              desloc_R = 0,
              desloc_L = 0;

    mouse_spin(mtrR, mtrL, 1);

    while (theta < target_theta){
        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);
        
        theta = (desloc_R + desloc_L)/W_EIXOS;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    odometria_mudar_sentido(pos, 1);
    ESP_LOGI(TAG, "virou 90 graus para a direita. orientacao atual: %s", odometria_orientacao_string(pos->orientacao));
}

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
        float target_theta = M_PI/2.0f;

        float theta = 0,
              desloc_R = 0,
              desloc_L = 0;

    mouse_spin(mtrR, mtrL, 0);

    while (theta < target_theta){
        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);
        
        theta = (desloc_R + desloc_L)/W_EIXOS;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    odometria_mudar_sentido(pos, 0);
    ESP_LOGI(TAG, "virou 90 graus para a esquerda. orientacao atual: %s", odometria_orientacao_string(pos->orientacao));
}
