#ifndef TELEMETRY_DATA_NVS_H
#define TELEMETRY_DATA_NVS_H

#include <stdint.h>
#include "esp_err.h"

// Estrutura de dados para guardar uma amostra de leitura
typedef struct {
    uint32_t timestamp_ms; // Tempo desde o boot em milissegundos
    uint16_t voltage_mv;   // Tensão em milivolts (ex: 5000 = 5.0V)
    int16_t  current_ma;   // Corrente em miliamperes
} telemetry_data_t;

/**
 * @brief Inicializa o sistema NVS. Deve ser chamado no início do app_main().
 * * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t telemetry_init(void);

/**
 * @brief Guarda uma nova amostra de dados do INA226 na NVS.
 * * @param voltage_mv Tensão lida no momento (milivolts).
 * @param current_ma Corrente lida no momento (miliamperes).
 * @return esp_err_t ESP_OK se gravado com sucesso.
 */
esp_err_t telemetry_save_sample(uint16_t voltage_mv, int16_t current_ma);

/**
 * @brief Lê todas as amostras guardadas na memória e imprime no Monitor Serial.
 * Ideal para ser chamado quando liga o robô ao PC ou ao premir um botão.
 */
void telemetry_print_all(void);

/**
 * @brief Apaga completamente o histórico de telemetria da NVS.
 * Deve ser chamado antes de iniciar uma nova "corrida" no labirinto.
 * * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t telemetry_clear(void);

#endif