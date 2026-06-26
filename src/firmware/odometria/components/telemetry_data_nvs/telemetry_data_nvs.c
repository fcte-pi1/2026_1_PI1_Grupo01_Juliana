//codigo todo gerado por IA, só para teste***

#include "telemetry_data_nvs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <stdio.h>

static const char *TAG = "TELEMETRIA_NVS";
static const char *NAMESPACE = "flight_data"; // Espaço de nomes na NVS
static uint32_t current_sample_index = 0;     // Contador de amostras gravadas

esp_err_t telemetry_init(void) {
    // Inicializa a partição NVS padrão
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Se a NVS estiver corrompida ou cheia, apaga e tenta novamente
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    nvs_handle_t my_handle;
    err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Tenta ler quantas amostras já estão guardadas de execuções anteriores
    err = nvs_get_u32(my_handle, "count", &current_sample_index);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        current_sample_index = 0; 
    }
    
    nvs_close(my_handle);
    ESP_LOGI(TAG, "Telemetria NVS Iniciada. Amostras pré-existentes: %lu", current_sample_index);
    return ESP_OK;
}

esp_err_t telemetry_save_sample(uint16_t voltage_mv, int16_t current_ma) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Constrói a estrutura com o carimbo de tempo
    telemetry_data_t data = {
        .timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000ULL),
        .voltage_mv = voltage_mv,
        .current_ma = current_ma
    };

    // Cria uma chave única para esta amostra (ex: "d_0", "d_1")
    // A chave na NVS tem um limite estrito de 15 caracteres
    char key[16];
    snprintf(key, sizeof(key), "d_%lu", current_sample_index);

    // Guarda o blob (bloco de dados binário)
    err = nvs_set_blob(my_handle, key, &data, sizeof(telemetry_data_t));
    if (err == ESP_OK) {
        current_sample_index++;
        // Atualiza o contador geral
        err = nvs_set_u32(my_handle, "count", current_sample_index);
        
        // Efetiva as alterações na memória Flash física
        nvs_commit(my_handle);
    } else {
        ESP_LOGE(TAG, "Erro ao guardar amostra %lu: %s", current_sample_index, esp_err_to_name(err));
    }

    nvs_close(my_handle);
    return err;
}

void telemetry_print_all(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Não há dados guardados ou erro ao abrir: %s", esp_err_to_name(err));
        return;
    }

    uint32_t total_samples = 0;
    nvs_get_u32(my_handle, "count", &total_samples);

    ESP_LOGI(TAG, "=== INÍCIO DO RELATÓRIO DE TELEMETRIA (%lu amostras) ===", total_samples);
    ESP_LOGI(TAG, "Tempo(ms) | Tensão(mV) | Corrente(mA)");
    ESP_LOGI(TAG, "---------------------------------------");

    for (uint32_t i = 0; i < total_samples; i++) {
        char key[16];
        snprintf(key, sizeof(key), "d_%lu", i);

        telemetry_data_t data;
        size_t required_size = sizeof(telemetry_data_t);
        
        err = nvs_get_blob(my_handle, key, &data, &required_size);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "%8lu | %10u | %11d", 
                     data.timestamp_ms, 
                     data.voltage_mv, 
                     data.current_ma);
        } else {
            ESP_LOGE(TAG, "Falha ao ler a amostra %lu", i);
        }
    }

    ESP_LOGI(TAG, "=== FIM DO RELATÓRIO ===");
    nvs_close(my_handle);
}

esp_err_t telemetry_clear(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Apaga todo o espaço de nomes da telemetria
    err = nvs_erase_all(my_handle);
    if (err == ESP_OK) {
        nvs_commit(my_handle);
        current_sample_index = 0;
        ESP_LOGI(TAG, "Histórico de telemetria completamente apagado.");
    } else {
        ESP_LOGE(TAG, "Erro ao apagar telemetria.");
    }
    
    nvs_close(my_handle);
    return err;
}