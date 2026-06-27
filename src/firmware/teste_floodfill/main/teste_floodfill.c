// Teste embarcado do algoritmo Flood Fill com exploracao incremental.
//
// O robo nao conhece o labirinto: o mapa de paredes comeca vazio e e
// preenchido a cada celula com a leitura dos sensores IR. A cada passo o
// Flood Fill e recalculado sobre o mapa conhecido e o robo anda para o
// vizinho acessivel de menor distancia, ate chegar na area objetivo
// central (2x2). Relacionado a issue #13.
//
// Usa as primitivas de movimentacao ja validadas no carrinho:
//   movimentacao_move_cell  -> anda 1 celula
//   movimentacao_turn_clws  -> gira 90 graus para a direita (horario)
//   movimentacao_turn_ctclws-> gira 90 graus para a esquerda (anti-horario)
//
// COMO USAR: ajuste os #defines de CONFIGURACAO abaixo, compile com
// `idf.py build` e grave com `idf.py flash monitor`. O monitor mostra, a
// cada celula, a leitura crua dos 5 sensores e a decisao tomada.

#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "m_driver.h"
#include "encoder.h"
#include "odometria.h"
#include "movimentacao.h"
#include "infrared.h" // apenas para reaproveitar os #defines dos pinos IR_*

static const char *TAG = "teste_floodfill";

// ===========================================================================
// CONFIGURACAO (ajuste aqui antes de gravar)
// ===========================================================================

// Lado do labirinto: 4 para o 4x4 de teste, 8 para o 8x8.
#define LADO 4

// Celula e orientacao iniciais. Padrao: canto (linha 0, coluna 0) virado
// para o NORTE (que aqui aponta para o interior do labirinto).
#define LINHA_INICIAL    0
#define COLUNA_INICIAL   0
#define ORIENT_INICIAL   NORTE

// Nivel digital do sensor IR quando HA parede na frente dele.
// A maioria dos modulos de obstaculo manda a saida para LOW (0) ao detectar.
// Se as paredes vierem invertidas no teste, troque para 1.
#define IR_NIVEL_PAREDE  0

// Quais pinos usar para cada direcao (definidos em infrared.h).
#define SENSOR_FRENTE    IR_FRONT
#define SENSOR_ESQUERDA  IR_L
#define SENSOR_DIREITA   IR_R

// Buzzer de sinalizacao (mesmo pino do resto do firmware).
#define BUZZER_GPIO      GPIO_NUM_33

#define INF 9999

// ===========================================================================
// Estado do hardware
// ===========================================================================

motor_t motorR = { .pwm_gpio1 = PWM_R1, .pwm_gpio2 = PWM_R2 };
motor_t motorL = { .pwm_gpio1 = PWM_L1, .pwm_gpio2 = PWM_L2 };

encoder_t encoderR;
encoder_t encoderL;

pose_t pose;

// ===========================================================================
// Modelo do labirinto
// ===========================================================================
// parede[linha][coluna][direcao]: 1 = ha parede naquele lado, 0 = livre.
// Indexado pela orientacao_t (NORTE=0, LESTE=1, SUL=2, OESTE=3).
static uint8_t parede[LADO][LADO][4];
static int     dist[LADO][LADO];

// Deslocamento em (linha, coluna) para cada orientacao.
static const int dlinha[4] = { +1, 0, -1, 0 }; // NORTE, LESTE, SUL, OESTE
static const int dcol[4]   = {  0, +1, 0, -1 };

static inline int oposto(int dir) { return (dir + 2) % 4; }

static inline bool dentro(int l, int c)
{
    return l >= 0 && l < LADO && c >= 0 && c < LADO;
}

static bool eh_objetivo(int l, int c)
{
    int c1 = (LADO / 2) - 1;
    int c2 = LADO / 2;
    return (l == c1 || l == c2) && (c == c1 || c == c2);
}

// ===========================================================================
// Buzzer (feedback sonoro em campo)
// ===========================================================================
static void buzzer_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz         = 1000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num   = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&channel);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static void bip(uint32_t freq, uint32_t ms)
{
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    vTaskDelay(pdMS_TO_TICKS(ms));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// ===========================================================================
// Sensores IR (leitura sincrona, sem ISR/task reativa)
// ===========================================================================
static void sensores_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << IR_FRONT) | (1ULL << IR_FL) | (1ULL << IR_FR) |
                        (1ULL << IR_L) | (1ULL << IR_R),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,   // os modulos IR ja drivam a linha
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
}

static inline bool ha_parede(gpio_num_t pin)
{
    return gpio_get_level(pin) == IR_NIVEL_PAREDE;
}

// ===========================================================================
// Logica do Flood Fill
// ===========================================================================
static void init_labirinto(void)
{
    for (int l = 0; l < LADO; l++) {
        for (int c = 0; c < LADO; c++) {
            for (int d = 0; d < 4; d++) {
                parede[l][c][d] = 0;
            }
            dist[l][c] = INF;
        }
    }

    // paredes do contorno externo
    for (int i = 0; i < LADO; i++) {
        parede[0][i][SUL]         = 1;
        parede[LADO - 1][i][NORTE] = 1;
        parede[i][0][OESTE]        = 1;
        parede[i][LADO - 1][LESTE] = 1;
    }
}

// Marca uma parede dos dois lados (na celula atual e na vizinha).
static void marcar_parede(int l, int c, int dir)
{
    parede[l][c][dir] = 1;

    int nl = l + dlinha[dir];
    int nc = c + dcol[dir];

    if (dentro(nl, nc)) {
        parede[nl][nc][oposto(dir)] = 1;
    }
}

// Le os sensores na celula atual e atualiza o mapa conhecido.
static void sentir_paredes(int l, int c, orientacao_t head)
{
    int frente   = head;
    int direita  = (head + 1) % 4;
    int esquerda = (head + 3) % 4;

    bool pf = ha_parede(SENSOR_FRENTE);
    bool pd = ha_parede(SENSOR_DIREITA);
    bool pe = ha_parede(SENSOR_ESQUERDA);

    ESP_LOGI(TAG,
             "sensores brutos -> FRONT:%d FL:%d FR:%d L:%d R:%d | "
             "parede frente:%d esq:%d dir:%d",
             gpio_get_level(IR_FRONT), gpio_get_level(IR_FL),
             gpio_get_level(IR_FR), gpio_get_level(IR_L),
             gpio_get_level(IR_R), pf, pe, pd);

    if (pf) marcar_parede(l, c, frente);
    if (pd) marcar_parede(l, c, direita);
    if (pe) marcar_parede(l, c, esquerda);
}

// BFS a partir das 4 celulas objetivo, propagando pelas passagens livres.
static void flood_fill(void)
{
    int ql[LADO * LADO];
    int qc[LADO * LADO];
    int frente = 0, fim = 0;

    for (int l = 0; l < LADO; l++) {
        for (int c = 0; c < LADO; c++) {
            dist[l][c] = INF;
        }
    }

    int c1 = (LADO / 2) - 1;
    int c2 = LADO / 2;
    int objetivos[4][2] = { {c1, c1}, {c1, c2}, {c2, c1}, {c2, c2} };

    for (int i = 0; i < 4; i++) {
        int gl = objetivos[i][0];
        int gc = objetivos[i][1];
        dist[gl][gc] = 0;
        ql[fim] = gl;
        qc[fim] = gc;
        fim++;
    }

    while (frente < fim) {
        int l = ql[frente];
        int c = qc[frente];
        frente++;

        for (int d = 0; d < 4; d++) {
            if (parede[l][c][d]) {
                continue;
            }

            int nl = l + dlinha[d];
            int nc = c + dcol[d];

            if (dentro(nl, nc) && dist[nl][nc] > dist[l][c] + 1) {
                dist[nl][nc] = dist[l][c] + 1;
                ql[fim] = nl;
                qc[fim] = nc;
                fim++;
            }
        }
    }
}

// Escolhe a direcao do vizinho acessivel com menor distancia. -1 se preso.
static int proximo_movimento(int l, int c)
{
    int melhor = INF;
    int dir = -1;

    for (int d = 0; d < 4; d++) {
        if (parede[l][c][d]) {
            continue;
        }

        int nl = l + dlinha[d];
        int nc = c + dcol[d];

        if (dentro(nl, nc) && dist[nl][nc] < melhor) {
            melhor = dist[nl][nc];
            dir = d;
        }
    }

    return dir;
}

// Gira para a direcao desejada (usando o caminho mais curto) e anda 1 celula.
static void executar_movimento(orientacao_t *head, int dir, int *l, int *c)
{
    int diff = (dir - *head + 4) % 4;

    switch (diff) {
        case 1: // direita
            movimentacao_turn_clws(&motorR, &motorL, &encoderR, &encoderL, &pose);
            break;
        case 2: // meia-volta
            movimentacao_turn_clws(&motorR, &motorL, &encoderR, &encoderL, &pose);
            movimentacao_turn_clws(&motorR, &motorL, &encoderR, &encoderL, &pose);
            break;
        case 3: // esquerda
            movimentacao_turn_ctclws(&motorR, &motorL, &encoderR, &encoderL, &pose);
            break;
        default: // ja esta na direcao certa
            break;
    }

    *head = (orientacao_t)dir;

    movimentacao_move_cell(&motorR, &motorL, &encoderR, &encoderL, &pose);

    *l += dlinha[dir];
    *c += dcol[dir];
}

static void resolver(void)
{
    int l = LINHA_INICIAL;
    int c = COLUNA_INICIAL;
    orientacao_t head = ORIENT_INICIAL;

    int passos_max = LADO * LADO * 4;

    for (int passo = 0; passo < passos_max; passo++) {

        sentir_paredes(l, c, head);
        flood_fill();

        ESP_LOGI(TAG, "passo %d | celula (%d,%d) | orientacao %s | dist %d",
                 passo, l, c, odometria_orientacao_string(head), dist[l][c]);

        if (eh_objetivo(l, c)) {
            ESP_LOGI(TAG, "chegou ao centro em (%d,%d)!", l, c);
            bip(880, 150);
            bip(988, 150);
            bip(1175, 300);
            return;
        }

        int dir = proximo_movimento(l, c);
        if (dir < 0) {
            ESP_LOGW(TAG, "encurralado em (%d,%d), sem vizinho acessivel", l, c);
            return;
        }

        executar_movimento(&head, dir, &l, &c);
    }

    ESP_LOGW(TAG, "limite de passos atingido sem chegar ao centro");
}

// ===========================================================================
// app_main
// ===========================================================================
void app_main(void)
{
    ESP_LOGI(TAG, "iniciando teste de flood fill (labirinto %dx%d)", LADO, LADO);

    buzzer_init();
    sensores_init();

    encoder_init(&encoderR, GPIO_ENC_R);
    encoder_init(&encoderL, GPIO_ENC_L);

    driver_init();
    motor_init(&motorR);
    motor_init(&motorL);

    // reatribuicao exigida pelo m_driver apos motor_init (ver m_driver.h)
    motorR = (motor_t){ PWM_R1, PWM_R2, motorR.comp1, motorR.comp2,
                        motorR.gen1, motorR.gen2 };
    motorL = (motor_t){ PWM_L1, PWM_L2, motorL.comp1, motorL.comp2,
                        motorL.gen1, motorL.gen2 };

    odometria_pos_init(&pose, 0, 0, ORIENT_INICIAL);

    init_labirinto();

    // contagem regressiva para posicionar o robo e afastar a mao
    ESP_LOGI(TAG, "comecando em 3s...");
    bip(440, 100); vTaskDelay(pdMS_TO_TICKS(900));
    bip(440, 100); vTaskDelay(pdMS_TO_TICKS(900));
    bip(440, 100); vTaskDelay(pdMS_TO_TICKS(900));
    bip(880, 200);

    resolver();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
