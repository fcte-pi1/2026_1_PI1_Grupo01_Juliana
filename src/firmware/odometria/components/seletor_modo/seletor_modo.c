// codigo-fonte do seletor de modo do micromouse
// responsavel por: ler o botao do GPIO 25 (por interrupcao), guardar o modo
// de operacao atual (labirinto 4x4 ou 8x8) e sinalizar trocas pelo buzzer.

#include "seletor_modo.h"

#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "seletor_modo";

#define LEDC_BUZZER_TIMER    LEDC_TIMER_0
#define LEDC_BUZZER_CHANNEL  LEDC_CHANNEL_0
#define LEDC_BUZZER_DUTY     512   // ~50% de duty em resolucao de 10 bits

// estado compartilhado entre a ISR e as tasks
static volatile modo_lab_t modo_atual = MODO_LAB_4X4;
static TaskHandle_t supervisor_task = NULL;

// ---------------------------------------------------------------------------
// buzzer
// ---------------------------------------------------------------------------
static void buzzer_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_BUZZER_TIMER,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz         = 1000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num   = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_BUZZER_CHANNEL,
        .timer_sel  = LEDC_BUZZER_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&channel);

    // comeca em silencio
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CHANNEL);
}

void play_tone(uint32_t freq, uint32_t duracao_ms)
{
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_TIMER, freq);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CHANNEL, LEDC_BUZZER_DUTY);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CHANNEL);

    vTaskDelay(pdMS_TO_TICKS(duracao_ms));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_BUZZER_CHANNEL);
}

// ---------------------------------------------------------------------------
// botao (interrupcao)
// ---------------------------------------------------------------------------
// A ISR e mantida o mais curta possivel: faz apenas o debounce por tempo e
// acorda a task supervisora. Toda a logica pesada (parar motores, tocar o
// buzzer, reiniciar a missao) roda na task, nunca aqui dentro.
static void IRAM_ATTR botao_isr_handler(void *arg)
{
    static volatile int64_t ultimo_us = 0;

    int64_t agora = esp_timer_get_time();
    if (agora - ultimo_us < BOTAO_DEBOUNCE_US) {
        return;
    }
    ultimo_us = agora;

    if (supervisor_task == NULL) {
        return;
    }

    BaseType_t hp_task = pdFALSE;
    vTaskNotifyGiveFromISR(supervisor_task, &hp_task);
    if (hp_task) {
        portYIELD_FROM_ISR();
    }
}

void seletor_modo_init(TaskHandle_t supervisor)
{
    supervisor_task = supervisor;

    buzzer_init();

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << BOTAO_SELETOR_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io);

    gpio_isr_handler_add(BOTAO_SELETOR_GPIO, botao_isr_handler, NULL);

    ESP_LOGI(TAG, "seletor iniciado (botao GPIO %d, buzzer GPIO %d)",
             BOTAO_SELETOR_GPIO, BUZZER_GPIO);
}

// ---------------------------------------------------------------------------
// estado do modo
// ---------------------------------------------------------------------------
modo_lab_t seletor_modo_toggle(void)
{
    modo_atual = (modo_atual == MODO_LAB_4X4) ? MODO_LAB_8X8 : MODO_LAB_4X4;
    ESP_LOGI(TAG, "modo alterado para %s",
             (modo_atual == MODO_LAB_4X4) ? "4x4" : "8x8");
    return modo_atual;
}

modo_lab_t seletor_modo_get(void)
{
    return modo_atual;
}

uint8_t seletor_modo_lado(void)
{
    return (modo_atual == MODO_LAB_4X4) ? 4 : 8;
}

void seletor_modo_sinalizar(modo_lab_t modo)
{
    if (modo == MODO_LAB_4X4) {
        // 1 bipe agudo = 4x4
        play_tone(BUZZER_FREQ_4X4, BUZZER_BEEP_MS);
    } else {
        // 2 bipes graves = 8x8
        play_tone(BUZZER_FREQ_8X8, BUZZER_BEEP_MS);
        vTaskDelay(pdMS_TO_TICKS(120));
        play_tone(BUZZER_FREQ_8X8, BUZZER_BEEP_MS);
    }
}