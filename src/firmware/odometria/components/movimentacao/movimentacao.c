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
// PWM reduzido nas curvas de recuperacao (evita brownout apos travamento).
#define PWM_CURVA_REC 35
#define MIN_PWM_CURVA_REC 28

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

#define CELULA_MIN_FRAC 0.70f
#define DESLOC_AJUSTE_MIN 0.02f
#define RE_DIST_M 0.07f
#define RE_TEMPO_MIN_MS 450
#define RE_TEMPO_MAX_MS 900

bool movimentacao_re_curta(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL)
{
    float desloc = 0.0f;
    float desloc_R = 0.0f;
    float desloc_L = 0.0f;
    uint32_t tempo_ms = 0;

    gpio_set_level(SEL, 0);
    motor_set_speed(mtrR, -MOTOR_RE_SPD);
    motor_set_speed(mtrL, -MOTOR_RE_SPD);
    ESP_LOGI(TAG, "re curta iniciada (pwm=%d)", MOTOR_RE_SPD);

    while (tempo_ms < RE_TEMPO_MAX_MS) {
        desloc_R += fabsf(encoder_get_deslocamento(encR, RAIO_R, NULL));
        desloc_L += fabsf(encoder_get_deslocamento(encL, RAIO_R, NULL));
        desloc = (desloc_R + desloc_L) / 2.0f;

        if (desloc >= RE_DIST_M) {
            break;
        }
        if (tempo_ms >= RE_TEMPO_MIN_MS && desloc >= (RE_DIST_M * 0.25f)) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
        tempo_ms += 10;
    }

    mouse_break(mtrR, mtrL);
    encoder_clean(encR);
    encoder_clean(encL);

    const bool ok = desloc >= (RE_DIST_M * 0.2f) || tempo_ms >= RE_TEMPO_MIN_MS;
    ESP_LOGI(TAG, "re curta | desloc=%.3fm tempo=%lums ok=%s",
             desloc, (unsigned long)tempo_ms, ok ? "SIM" : "NAO");
    return ok;
}

bool movimentacao_move_cell(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL,
                            pose_t *pos, movimentacao_status_t *status)
{
    if (status != NULL) {
        *status = MOV_TRAVADO;
    }
 
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
    const uint32_t TIMEOUT_MAX_MS = 2000;
    const uint32_t SEM_PROGRESSO_MS = 700;
    const float PROGRESSO_MIN_M = 0.008f;
    bool stalled = false;
    int pulsos_R_total = 0;
    int pulsos_L_total = 0;
    int pulsos_amostra = 0;
    float ultimo_desloc = 0.0f;
    uint32_t ms_sem_progresso = 0;
    
    motion_abort = false; // Reseta a flag ao iniciar

    mouse_movefwd(mtrR, mtrL);

    while(desloc < target){

        // Se a tarefa de IR detectou algo, aborta o movimento imediatamente
        if (motion_abort) {
            ESP_LOGW(TAG, "Movimento abortado pela tarefa de IR!");
            mouse_coast(mtrR, mtrL);
            ESP_LOGW(TAG, "celula FALHOU | desloc=%.3fm stall=NAO abort=SIM pulsos R=%d L=%d",
                     desloc, pulsos_R_total, pulsos_L_total);
            if (status != NULL) {
                *status = MOV_ABORTADO;
            }
            return false;
        }

        // Atualiza a distância de cada roda
        desloc_R += encoder_get_deslocamento(encR, RAIO_R, &pulsos_amostra);
        pulsos_R_total += pulsos_amostra;
        desloc_L += encoder_get_deslocamento(encL, RAIO_R, &pulsos_amostra);
        pulsos_L_total += pulsos_amostra;
        
        // Calcula a média das rodas para saber o quanto o robô andou no total
        desloc = (desloc_R + desloc_L)/2.0f;

        if (desloc >= (ultimo_desloc + PROGRESSO_MIN_M)) {
            ultimo_desloc = desloc;
            ms_sem_progresso = 0;
        } else {
            ms_sem_progresso += 10;
        }

        if (ms_sem_progresso >= SEM_PROGRESSO_MS && desloc < (target * 0.25f)) {
            ESP_LOGW(TAG, "STALL: sem progresso por %lums — parando motores",
                     (unsigned long)ms_sem_progresso);
            mouse_coast(mtrR, mtrL);
            stalled = true;
            break;
        }

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
            stalled = true;
            break;
        }
    }

    mouse_break(mtrR, mtrL);

    time = odometria_get_segundos() - previoustime;

    meanspeed = mouse_get_linear_speed(encR, encL, time);

    const bool sucesso = desloc >= (target * CELULA_MIN_FRAC);

    if (sucesso) {
        pos->cell_count++;
        odometria_update_vm(pos, meanspeed);
        odometria_update_xy(pos, desloc);
        if (status != NULL) {
            *status = MOV_OK;
        }
        ESP_LOGI(TAG, "celula OK | desloc=%.3fm tempo=%lums pulsos R=%d L=%d pos=(%.3f, %.3f)",
                 desloc, (unsigned long)tempo_decorrido_ms, pulsos_R_total, pulsos_L_total,
                 pos->x, pos->y);
        return true;
    }

    if (stalled) {
        if (status != NULL) {
            *status = MOV_TRAVADO;
        }
        ESP_LOGW(TAG, "celula TRAVOU | desloc=%.3fm pulsos R=%d L=%d pos=(%.3f, %.3f)",
                 desloc, pulsos_R_total, pulsos_L_total, pos->x, pos->y);
    } else if (desloc >= DESLOC_AJUSTE_MIN) {
        if (status != NULL) {
            *status = MOV_AJUSTE_PAREDE;
        }
        ESP_LOGW(TAG, "celula AJUSTE | desloc=%.3fm (correcao PD na parede) pulsos R=%d L=%d",
                 desloc, pulsos_R_total, pulsos_L_total);
    } else {
        if (status != NULL) {
            *status = MOV_TRAVADO;
        }
        ESP_LOGW(TAG, "celula FALHOU | desloc=%.3fm pulsos R=%d L=%d pos=(%.3f, %.3f)",
                 desloc, pulsos_R_total, pulsos_L_total, pos->x, pos->y);
    }

    return false;
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
        desloc_R += fabs(encoder_get_deslocamento(encR, RAIO_R, NULL));
        desloc_L += fabs(encoder_get_deslocamento(encL, RAIO_R, NULL));
        
        theta = (desloc_R + desloc_L)/W_EIXOS;
        
        // Calcula o erro restante para chegar a 90 graus
        erro = target_theta - theta;

        // Diminui o PWM proporcionalmente à medida que se aproxima do alvo
        pwm = (int)(KP_CURVA * erro);

        // Saturação superior e inferior (Deadband)
        if (pwm > PWM_CURVA_REC) pwm = PWM_CURVA_REC;
        if (pwm < MIN_PWM_CURVA_REC) pwm = MIN_PWM_CURVA_REC;

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
    encoder_clean(encR);
    encoder_clean(encL);

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
        desloc_R += fabs(encoder_get_deslocamento(encR, RAIO_R, NULL));
        desloc_L += fabs(encoder_get_deslocamento(encL, RAIO_R, NULL));
        
        theta = (desloc_R + desloc_L)/W_EIXOS;
        
        erro = target_theta - theta;

        pwm = (int)(KP_CURVA * erro);

        if (pwm > PWM_CURVA_REC) pwm = PWM_CURVA_REC;
        if (pwm < MIN_PWM_CURVA_REC) pwm = MIN_PWM_CURVA_REC;

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
    encoder_clean(encR);
    encoder_clean(encL);

    odometria_mudar_sentido(pos, 0);
    ESP_LOGI(TAG, "virou 90 graus para a esquerda. orientacao atual: %s, angulo(rad): %f", odometria_orientacao_string(pos->orientacao), theta);

}