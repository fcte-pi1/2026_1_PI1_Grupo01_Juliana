#ifndef SELETOR_MODO_H
#define SELETOR_MODO_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// botao de selecao de modo (entrada com pull-up interno, aciona em borda de descida)
#define BOTAO_SELETOR_GPIO  GPIO_NUM_25
// buzzer de sinalizacao (saida PWM via LEDC)
#define BUZZER_GPIO         GPIO_NUM_33

// frequencias usadas para diferenciar os modos no buzzer
#define BUZZER_FREQ_4X4     880   // 1 bipe agudo  -> labirinto 4x4
#define BUZZER_FREQ_8X8     440   // 2 bipes graves -> labirinto 8x8
#define BUZZER_BEEP_MS      150

// janela de debounce do botao, em microssegundos
#define BOTAO_DEBOUNCE_US   250000

typedef enum {
    MODO_LAB_4X4 = 0,
    MODO_LAB_8X8 = 1,
} modo_lab_t;

// Inicializa o buzzer (LEDC) e configura o botao do GPIO 25 com interrupcao.
// A cada clique valido a 'supervisor' recebe uma notificacao (task notification).
void seletor_modo_init(TaskHandle_t supervisor);

// Alterna 4x4 <-> 8x8 e devolve o novo modo. Deve ser chamada fora da ISR.
modo_lab_t seletor_modo_toggle(void);

// Modo atualmente selecionado.
modo_lab_t seletor_modo_get(void);

// Lado do labirinto (4 ou 8) correspondente ao modo atual.
uint8_t seletor_modo_lado(void);

// Emite o padrao sonoro do modo informado. Usa vTaskDelay, entao deve ser
// chamada em contexto de task (nunca dentro da ISR).
void seletor_modo_sinalizar(modo_lab_t modo);

#endif
