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

volatile bool motion_abort = false;

// Ganhos do Controlador PD (Sera ajustado experimentalmente)
#define KP_RETA 7.0f
#define KD_RETA 2.0f

// Ganhos para curva de 90 graus
#define KP_CURVA 80.0f
// PWM minimo para que o robo consiga vencer o atrito estatico e girar efetivamente
#define MIN_PWM_CURVA 35

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

float mouse_get_linear_speed(encoder_t *encR, encoder_t *encL, int64_t dt){

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

    // Variáveis para o controle PD de alinhamento
    float erro = 0;
    float erro_anterior = 0;
    float derivativo = 0;
    float correcao = 0;

    // Velocidade base (em PWM) para andar uma celula
    int motor_speed = MOTOR_FWD_SPD; 

    // Variáveis de proteção contra travamento (Stall Detection)
    uint32_t tempo_decorrido_ms = 0;
    const uint32_t TIMEOUT_MAX_MS = 3500; // 3.5 segundos para percorrer 18 cm
    
    motion_abort = false; // Reseta a flag ao iniciar

    mouse_movefwd(mtrR, mtrL);

    while(desloc < target){

        // Se a tarefa de IR detectou algo, aborta o movimento imediatamente
        if (motion_abort) {
            ESP_LOGW(TAG, "Movimento abortado pela tarefa de IR!");
            mouse_coast(mtrR, mtrL);
            return; // Sai da função, parando o loop
        }

        // Atualiza a distância de cada roda
        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);
        
        // Calcula a média das rodas para saber o quanto o robô andou no total
        desloc = (desloc_R + desloc_L)/2.0f;

        // --- CONTROLE PD PARA ALINHAMENTO EM LINHA RETA ---
        
        // O erro é a diferença de deslocamento entre as rodas. 
        // Se erro > 0, a roda R andou mais (robô curvando pra esquerda)
        // Se erro < 0, a roda L andou mais (robô curvando pra direita)
        erro = desloc_R - desloc_L;
        
        // Calcula a taxa de variação do erro (derivada)
        derivativo = erro - erro_anterior;
        
        // Calcula o esforço de controle
        correcao = (KP_RETA * erro) + (KD_RETA * derivativo);
        
        // Aplica a correção nos motores.
        // Se R está na frente (erro positivo, correção positiva), freia R e acelera L.
        int pwmL = motor_speed + (int)correcao;
        int pwmR = motor_speed - (int)correcao;
        
        // Limita o PWM para não ultrapassar 100% ou inverter a polaridade
        if (pwmL > 100) pwmL = 100;
        if (pwmL < 0) pwmL = 0;
        if (pwmR > 100) pwmR = 100;
        if (pwmR < 0) pwmR = 0;

        motor_set_speed(mtrL, pwmL);
        motor_set_speed(mtrR, pwmR);
        
        // Atualiza o erro anterior
        erro_anterior = erro;

        // Intervalo de amostragem reduzido para melhorar a resolução do Derivativo.
        vTaskDelay(pdMS_TO_TICKS(10));
        tempo_decorrido_ms += 10;

        // PROTEÇÃO: Se exceder o tempo limite, desliga os motores e sai do loop
        if (tempo_decorrido_ms >= TIMEOUT_MAX_MS) {
            ESP_LOGW(TAG, "STALL DETECTED: Timeout de movimento atingido (motores desligados ou travados)!");
            break;
        }
    }

    mouse_break(mtrR, mtrL);

    time = odometria_get_segundos() - previoustime;

    meanspeed = mouse_get_linear_speed(encR, encL, time);

    pos->cell_count++;

    odometria_update_vm(pos, meanspeed);

    odometria_update_xy(pos, desloc);

    ESP_LOGI(TAG, "andou 1 celula, posicao: (%f, %f), desloc: %f, velocidade media (m/s):%f",pos->x, pos->y, desloc, meanspeed);

}

void movimentacao_turn_clws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){

    float target_theta = M_PI/2.0f;
    float theta = 0, desloc_R = 0, desloc_L = 0;
    float erro = target_theta;
    int pwm = MOTOR_BWD_SPD;

    // Proteção contra travamento na curva (limite de 2 segundos para 90 graus)
    uint32_t tempo_decorrido_ms = 0;
    const uint32_t TIMEOUT_CURVA_MS = 2000;

    gpio_set_level(SEL, 0); // garantir que o freio esta desativado

    while (theta < target_theta){
        // Usando fabs para garantir a soma absoluta, já que o encoder não tem direção
        desloc_R += fabs(encoder_get_deslocamento(encR, RAIO_R));
        desloc_L += fabs(encoder_get_deslocamento(encL, RAIO_R));
        
        theta = (desloc_R + desloc_L)/W_EIXOS;
        
        // Calcula o erro restante para chegar a 90 graus
        erro = target_theta - theta;

        // Diminui o PWM proporcionalmente à medida que se aproxima do alvo
        pwm = (int)(KP_CURVA * erro);

        // Saturação superior e inferior (Deadband)
        if (pwm > MOTOR_BWD_SPD) pwm = MOTOR_BWD_SPD;
        if (pwm < MIN_PWM_CURVA) pwm = MIN_PWM_CURVA;

        // Aplica PWM diretamente (Curva horária: R para trás, L para frente)
        motor_set_speed(mtrR, -pwm);
        motor_set_speed(mtrL, pwm);

        vTaskDelay(pdMS_TO_TICKS(10)); // Amostragem rápida para não perder o ponto

        tempo_decorrido_ms += 10;

        if (tempo_decorrido_ms >= TIMEOUT_CURVA_MS) {
            ESP_LOGW(TAG, "STALL DETECTED: Timeout na curva direita!");
            break;
        }
    }

    mouse_break(mtrR, mtrL);

    odometria_mudar_sentido(pos, 1);
    ESP_LOGI(TAG, "virou 90 graus para a direita. orientacao atual: %s, angulo(rad): %f", odometria_orientacao_string(pos->orientacao), theta);

}

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){

    float target_theta = M_PI/2.0f;
    float theta = 0, desloc_R = 0, desloc_L = 0;
    float erro = target_theta;
    int pwm = MOTOR_BWD_SPD;

    uint32_t tempo_decorrido_ms = 0;
    const uint32_t TIMEOUT_CURVA_MS = 2000;

    gpio_set_level(SEL, 0); // garantir que o freio esta desativado

    while (theta < target_theta){
        desloc_R += fabs(encoder_get_deslocamento(encR, RAIO_R));
        desloc_L += fabs(encoder_get_deslocamento(encL, RAIO_R));
        
        theta = (desloc_R + desloc_L)/W_EIXOS;
        
        erro = target_theta - theta;

        pwm = (int)(KP_CURVA * erro);

        if (pwm > MOTOR_BWD_SPD) pwm = MOTOR_BWD_SPD;
        if (pwm < MIN_PWM_CURVA) pwm = MIN_PWM_CURVA;

        // Aplica PWM diretamente (Curva anti-horária: R para frente, L para trás)
        motor_set_speed(mtrR, pwm);
        motor_set_speed(mtrL, -pwm);

        vTaskDelay(pdMS_TO_TICKS(10));

        tempo_decorrido_ms += 10;

        if (tempo_decorrido_ms >= TIMEOUT_CURVA_MS) {
            ESP_LOGW(TAG, "STALL DETECTED: Timeout na curva esquerda!");
            break;
        }
    }

    mouse_break(mtrR, mtrL);

    odometria_mudar_sentido(pos, 0);
    ESP_LOGI(TAG, "virou 90 graus para a esquerda. orientacao atual: %s, angulo(rad): %f", odometria_orientacao_string(pos->orientacao), theta);

}