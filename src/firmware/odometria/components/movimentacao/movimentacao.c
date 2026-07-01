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

// Controle de retidao (malha fechada): usa os encoders para igualar a
// distancia percorrida pelas duas rodas, de modo que o robo ande reto.
// A cada ciclo corrige o PWM proporcionalmente a diferenca acumulada entre
// as rodas; se a direita adiantou, alivia a direita e reforca a esquerda.
// Alem do clamp da correcao, a velocidade final de cada roda e limitada a
// [MOTOR_MIN_SPD, 100]: abaixo de 25 e acima de 100 o driver DESLIGA o motor,
// entao esse piso/teto evita que a correcao (ou uma base baixa) mate a roda.
#define STRAIGHT_KP        800.0f  // ganho [PWM por metro de erro] - principal ajuste
#define STRAIGHT_CORR_MAX  20      // correcao maxima de PWM aplicada a cada roda
#define MOTOR_MIN_SPD      28      // piso de PWM por roda (abaixo de ~25 o motor desliga)

// Giro em dois tempos: um "chute" inicial de PWM alto para vencer o atrito
// estatico (girar parado arrasta as rodas de lado e pesa muito) e depois uma
// velocidade de cruzeiro mais baixa para o giro ser controlado e preciso. Sem
// o chute o robo estola; sem baixar depois ele gira rapido demais, escorrega
// (e o encoder mede o angulo errado) e se perde na orientacao.
#define MOTOR_TURN_SPD     75      // chute inicial (25..100); suba se ainda travar ao iniciar o giro
#define MOTOR_TURN_CRUISE  45      // velocidade apos destravar (>=25); baixe p/ giro mais preciso
#define TURN_KICK_UNTIL    0.25f   // rad girados no chute antes de cair p/ cruzeiro (~14 graus)

// Precisao do giro: com poll de 100ms o robo girava ~16 graus por ciclo e
// passava do alvo de forma inconsistente (97 graus numa rodada, 94 noutra).
// Amostrar mais rapido reduz esse overshoot; o trim compensa a inercia
// residual apos o freio (mirar um pouco abaixo de 90 graus).
#define TURN_POLL_MS       20      // periodo de amostragem do giro [ms]
#define TURN_TARGET_TRIM   0.30f   // rad subtraidos de 90 graus p/ compensar inercia+overshoot (~17 graus, valor calibrado no robo real)

// Timeout de seguranca: se o encoder nao registrar progresso suficiente no
// tempo esperado (roda presa, motor sem forca, encoder travado), aborta o
// movimento em vez de empurrar/girar indefinidamente.
#define MOVE_CELL_TIMEOUT_S  3.0f   // tempo maximo para andar 1 celula [s]
#define TURN_TIMEOUT_S       2.5f   // tempo maximo para completar um giro [s]

// Freio anti-colisao: durante o move, se o sensor frontal ver parede perto,
// freia antes de bater em vez de andar a celula inteira as cegas. Corrige o
// "bate de frente" nas horas em que o encoder e enganado pela patinacao contra
// a parede. DEPENDE do limiar do sensor calibrado para disparar por volta de
// 10-12cm; com limiar longo demais ele pode parar cedo. So age depois de andar
// FRONT_STOP_MIN_FRAC da celula, para nao frear logo no comeco.
#define FRONT_STOP_ENABLE     1           // 0 desliga o freio anti-colisao
#define IR_FRONT_PIN          GPIO_NUM_34 // pino do sensor frontal (igual ao teste_floodfill)
#define IR_FRONT_NIVEL_PAREDE 0           // nivel digital quando HA parede na frente
#define FRONT_STOP_MIN_FRAC   0.6f        // so considera a parede depois desta fracao da celula
#define FRONT_STOP_EXTRA      0.03f       // avanco extra apos AVISTAR a parede, p/ se posicionar antes de frear/girar [m]

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
        motor_set_speed(motorR, -MOTOR_TURN_SPD);
        motor_set_speed(motorL, MOTOR_TURN_SPD);
        ESP_LOGI(TAG, "comando virar sentido horario");
    } else {
        motor_set_speed(motorR, MOTOR_TURN_SPD);
        motor_set_speed(motorL, -MOTOR_TURN_SPD);
        ESP_LOGI(TAG, "comando virar sentido anti-horario");
    }
}

// ajusta a velocidade do giro mantendo o sentido, usado para cair do chute
// inicial para a velocidade de cruzeiro no meio do giro
static void mouse_spin_speed(motor_t *motorR, motor_t *motorL, bool sentido, int8_t spd){
    if(sentido){
        motor_set_speed(motorR, -spd);
        motor_set_speed(motorL,  spd);
    } else {
        motor_set_speed(motorR,  spd);
        motor_set_speed(motorL, -spd);
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

    // descarta contagem residual dos encoders (freada anterior, giro ou
    // empurrao manual) para medir apenas o deslocamento deste movimento
    encoder_get_deslocamento(encR, RAIO_R);
    encoder_get_deslocamento(encL, RAIO_R);

    mouse_movefwd(mtrR, mtrL);

#if FRONT_STOP_ENABLE
    bool  parede_vista  = false;  // o sensor frontal ja avistou a parede?
    float desloc_ao_ver = 0;      // deslocamento no instante em que avistou
#endif

    while(1){

        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);

        desloc = (desloc_R + desloc_L)/2.0f;

        if (odometria_get_segundos() - previoustime > MOVE_CELL_TIMEOUT_S) {
            ESP_LOGW(TAG, "TIMEOUT ao andar 1 celula apos %.1fs (desloc=%.3f de %.3f). Encoder travado/roda presa? Abortando movimento.", MOVE_CELL_TIMEOUT_S, desloc, target);
            break;
        }

#if FRONT_STOP_ENABLE
        // o sensor frontal detecta a parede de longe. Ao AVISTAR a parede,
        // registra a posicao e ainda avanca FRONT_STOP_EXTRA para se posicionar
        // (chegar mais perto) antes de frear/girar. So considera depois de
        // FRONT_STOP_MIN_FRAC da celula, para nao disparar no comeco.
        if (!parede_vista && desloc > target * FRONT_STOP_MIN_FRAC &&
            gpio_get_level(IR_FRONT_PIN) == IR_FRONT_NIVEL_PAREDE) {
            parede_vista  = true;
            desloc_ao_ver = desloc;
            ESP_LOGI(TAG, "parede a frente avistada em %.3f, avancando +%.3f antes de parar", desloc, FRONT_STOP_EXTRA);
        }
        if (parede_vista) {
            if (desloc >= desloc_ao_ver + FRONT_STOP_EXTRA) {
                ESP_LOGI(TAG, "posicionado na parede da frente (desloc=%.3f)", desloc);
                break;
            }
        } else if (desloc >= target) {
            break;  // fim normal da celula (sem parede a frente avistada)
        }
#else
        if (desloc >= target) {
            break;  // fim normal da celula
        }
#endif

        // correcao de retidao: erro > 0 => roda direita adiantou =>
        // alivia a direita e reforca a esquerda para voltar a reta
        float corr = STRAIGHT_KP * (desloc_R - desloc_L);
        if (corr >  STRAIGHT_CORR_MAX) corr =  STRAIGHT_CORR_MAX;
        if (corr < -STRAIGHT_CORR_MAX) corr = -STRAIGHT_CORR_MAX;

        // limita cada roda a [MOTOR_MIN_SPD, 100] para nao cair na zona morta
        // (motor desliga abaixo de 25 e acima de 100), com qualquer base
        int8_t spdR = (int8_t)(MOTOR_FWD_SPD - corr);
        int8_t spdL = (int8_t)(MOTOR_FWD_SPD + corr);
        if (spdR < MOTOR_MIN_SPD) spdR = MOTOR_MIN_SPD;
        if (spdL < MOTOR_MIN_SPD) spdL = MOTOR_MIN_SPD;
        if (spdR > 100) spdR = 100;
        if (spdL > 100) spdL = 100;
        motor_set_speed(mtrR, spdR);
        motor_set_speed(mtrL, spdL);

        vTaskDelay(pdMS_TO_TICKS(100));
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
    float target_theta = M_PI/2.0f - TURN_TARGET_TRIM;

        float theta = 0,
              desloc_R = 0,
              desloc_L = 0;

    float t0_giro = odometria_get_segundos();
    bool reduziu = false;

    // descarta contagem residual dos encoders antes de medir o giro
    encoder_get_deslocamento(encR, RAIO_R);
    encoder_get_deslocamento(encL, RAIO_R);

    mouse_spin(mtrR, mtrL, 1);

    while (theta < target_theta){
        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);

        theta = (desloc_R + desloc_L)/W_EIXOS;

        // apos vencer o atrito estatico, cai para a velocidade de cruzeiro
        // (giro controlado e preciso, menos escorregamento das rodas)
        if (!reduziu && theta > TURN_KICK_UNTIL) {
            mouse_spin_speed(mtrR, mtrL, 1, MOTOR_TURN_CRUISE);
            reduziu = true;
        }

        if (odometria_get_segundos() - t0_giro > TURN_TIMEOUT_S) {
            ESP_LOGW(TAG, "TIMEOUT ao girar apos %.1fs (theta=%.3f de %.3f). Roda presa? Abortando giro.", TURN_TIMEOUT_S, theta, target_theta);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(TURN_POLL_MS));
    }
    
    mouse_break(mtrR, mtrL);

    odometria_mudar_sentido(pos, 1);
    ESP_LOGI(TAG, "virou 90 graus para a direita. orientacao atual: %s, angulo(rad): %f", odometria_orientacao_string(pos->orientacao), theta);
}

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos){
        float target_theta = M_PI/2.0f - TURN_TARGET_TRIM;

        float theta = 0,
              desloc_R = 0,
              desloc_L = 0;

    float t0_giro = odometria_get_segundos();
    bool reduziu = false;

    // descarta contagem residual dos encoders antes de medir o giro
    encoder_get_deslocamento(encR, RAIO_R);
    encoder_get_deslocamento(encL, RAIO_R);

    mouse_spin(mtrR, mtrL, 0);

    while (theta < target_theta){
        desloc_R += encoder_get_deslocamento(encR, RAIO_R);
        desloc_L += encoder_get_deslocamento(encL, RAIO_R);

        theta = (desloc_R + desloc_L)/W_EIXOS;

        // apos vencer o atrito estatico, cai para a velocidade de cruzeiro
        // (giro controlado e preciso, menos escorregamento das rodas)
        if (!reduziu && theta > TURN_KICK_UNTIL) {
            mouse_spin_speed(mtrR, mtrL, 0, MOTOR_TURN_CRUISE);
            reduziu = true;
        }

        if (odometria_get_segundos() - t0_giro > TURN_TIMEOUT_S) {
            ESP_LOGW(TAG, "TIMEOUT ao girar apos %.1fs (theta=%.3f de %.3f). Roda presa? Abortando giro.", TURN_TIMEOUT_S, theta, target_theta);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(TURN_POLL_MS));
    }

    mouse_break(mtrR, mtrL);

    odometria_mudar_sentido(pos, 0);
    ESP_LOGI(TAG, "virou 90 graus para a esquerda. orientacao atual: %s, angulo(rad): %f", odometria_orientacao_string(pos->orientacao), theta);
}
