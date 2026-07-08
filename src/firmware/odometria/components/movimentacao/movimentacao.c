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

// --- Ganhos do controlador PD do trecho reto ---
// IMPORTANTE: o erro de alinhamento (desloc_R - desloc_L) e convertido para
// CENTIMETROS antes de multiplicar pelos ganhos. O encoder devolve o
// deslocamento em METROS (~0,0107 m/pulso); sem essa conversao o erro ficava
// ~100x menor e a correcao (int) era truncada para 0 — o PD nao atuava.
// Ajuste experimental: aumente KP se o robo abrir/derivar, reduza se oscilar.
#define KP_RETA 6.0f   // PWM por cm de diferenca entre as rodas
#define KD_RETA 2.0f   // PWM por (cm/amostra) de variacao do erro

// --- Curva de 90 graus (controle P com perfil trapezoidal) ---
#define KP_CURVA           100.0f // PWM por rad de erro angular restante
#define PWM_CURVA_MAX      50      // teto de PWM (inicio da curva, vence o atrito)
#define PWM_CURVA_MIN      30      // piso de PWM (acima do deadband 25 do driver)
#define THETA_CURVA_MARGIN 0.10f   // rad (~6 graus) reservados p/ a inercia pos-freio
#define TIMEOUT_CURVA_MS   2000    // aborta a curva se travar

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

float mouse_get_linear_speed(encoder_t *encR, encoder_t *encL, float dt){
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

    // Zera os contadores para o deslocamento comecar do zero. Sem isso, pulsos
    // residuais de manobras anteriores entram na primeira leitura e corrompem
    // tanto a distancia quanto o erro do PD.
    encoder_clean(encR);
    encoder_clean(encL);

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

        // O erro é a diferença de deslocamento entre as rodas, em CENTIMETROS.
        // erro > 0: a roda R andou mais (robô desviando para a esquerda)
        // erro < 0: a roda L andou mais (robô desviando para a direita)
        erro = (desloc_R - desloc_L) * 100.0f;

        // Calcula a taxa de variação do erro (derivada discreta)
        derivativo = erro - erro_anterior;

        // Calcula o esforço de controle (PD)
        correcao = (KP_RETA * erro) + (KD_RETA * derivativo);

        // Aplica a correção: freia a roda adiantada e acelera a atrasada.
        // Arredonda (lroundf) em vez de truncar para não perder autoridade
        // quando a correção é pequena.
        const int corr = (int)lroundf(correcao);
        int pwmL = motor_speed + corr;
        int pwmR = motor_speed - corr;

        // Limita ao intervalo válido de PWM (0..100). Obs.: o driver tem
        // deadband de 25 — valores 1..24 apenas fazem a roda "coast" (desacelera
        // livre), o que ainda contribui para corrigir a trajetória.
        if (pwmL > 100) pwmL = 100;
        if (pwmL < 0)   pwmL = 0;
        if (pwmR > 100) pwmR = 100;
        if (pwmR < 0)   pwmR = 0;

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

// Rotina única de curva de 90 graus.
//   dir = +1  -> horária      (R para trás, L para frente)
//   dir = -1  -> anti-horária (R para frente, L para trás)
// Controle P: o PWM cai proporcionalmente ao ângulo restante (desacelera antes
// de parar), saturado entre PWM_CURVA_MIN e PWM_CURVA_MAX. O laço termina um
// pouco antes de 90° (THETA_CURVA_MARGIN) para compensar a inércia após o freio.
static void movimentacao_turn(motor_t *mtrR, motor_t *mtrL,
                              encoder_t *encR, encoder_t *encL,
                              pose_t *pos, int dir)
{
    const float target_theta = (float)M_PI / 2.0f;
    const float alvo_loop = target_theta - THETA_CURVA_MARGIN;

    float theta = 0.0f, desloc_R = 0.0f, desloc_L = 0.0f;
    uint32_t tempo_decorrido_ms = 0;

    gpio_set_level(SEL, 0); // garantir que o freio esta desativado

    // Zera os contadores: a curva começa do repouso, sem pulsos residuais da
    // manobra anterior (que dariam um salto no primeiro cálculo de theta).
    encoder_clean(encR);
    encoder_clean(encL);

    while (theta < alvo_loop) {
        // fabsf: o encoder não distingue sentido e as rodas giram opostas.
        desloc_R += fabsf(encoder_get_deslocamento(encR, RAIO_R, NULL));
        desloc_L += fabsf(encoder_get_deslocamento(encL, RAIO_R, NULL));

        theta = (desloc_R + desloc_L) / W_EIXOS;

        // Erro angular restante -> quanto menor, menor o PWM (desacelera no fim).
        const float erro = alvo_loop - theta;
        int pwm = (int)(KP_CURVA * erro);
        if (pwm > PWM_CURVA_MAX) pwm = PWM_CURVA_MAX;
        if (pwm < PWM_CURVA_MIN) pwm = PWM_CURVA_MIN;

        motor_set_speed(mtrR, (dir > 0) ? -pwm :  pwm);
        motor_set_speed(mtrL, (dir > 0) ?  pwm : -pwm);

        vTaskDelay(pdMS_TO_TICKS(10)); // amostragem rápida p/ não perder o alvo
        tempo_decorrido_ms += 10;

        if (tempo_decorrido_ms >= TIMEOUT_CURVA_MS) {
            ESP_LOGW(TAG, "STALL DETECTED: timeout na curva %s!",
                     (dir > 0) ? "horaria" : "anti-horaria");
            break;
        }
    }

    mouse_break(mtrR, mtrL);
    encoder_clean(encR);
    encoder_clean(encL);

    odometria_mudar_sentido(pos, (dir > 0) ? 1 : 0);
    ESP_LOGI(TAG, "virou 90 graus (%s). orientacao atual: %s, angulo(rad): %f",
             (dir > 0) ? "horario" : "anti-horario",
             odometria_orientacao_string(pos->orientacao), theta);
}

void movimentacao_turn_clws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
    movimentacao_turn(mtrR, mtrL, encR, encL, pos, +1);
}

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
    movimentacao_turn(mtrR, mtrL, encR, encL, pos, -1);
}