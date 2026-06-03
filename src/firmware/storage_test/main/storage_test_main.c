#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "STORAGE_TEST";

// ====================================================================
// Cenário 1: Armazenamento de Dados Básicos na NVS (Memória Não-Volátil)
// ====================================================================
esp_err_t test_nvs_storage(void)
{
    ESP_LOGI(TAG, "=== [INICIANDO TESTE NVS] ===");

    // 1. Inicializar o driver da NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS sem páginas livres ou nova versão encontrada. Formatando...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar NVS: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Abrir o namespace "config_dados"
    nvs_handle_t my_handle;
    err = nvs_open("config_dados", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir NVS: %s", esp_err_to_name(err));
        return err;
    }

    // 3. Escrever um dado básico (ex: corrida_id = 3)
    int32_t corrida_id_gravado = 3;
    ESP_LOGI(TAG, "Gravando corrida_id = %" PRId32 " na NVS...", corrida_id_gravado);
    err = nvs_set_i32(my_handle, "corrida_id", corrida_id_gravado);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao gravar na NVS: %s", esp_err_to_name(err));
        nvs_close(my_handle);
        return err;
    }

    // 4. Salvar as alterações
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro no commit da NVS: %s", esp_err_to_name(err));
        nvs_close(my_handle);
        return err;
    }

    // Fechar para forçar a reabertura e simular um reinício do ciclo de leitura
    nvs_close(my_handle);

    // 5. Reabrir e Ler o dado de volta
    err = nvs_open("config_dados", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao reabrir NVS: %s", esp_err_to_name(err));
        return err;
    }

    int32_t corrida_id_lido = 0;
    err = nvs_get_i32(my_handle, "corrida_id", &corrida_id_lido);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ler da NVS: %s", esp_err_to_name(err));
        nvs_close(my_handle);
        return err;
    }

    nvs_close(my_handle);

    // 6. Validar o resultado
    if (corrida_id_lido == corrida_id_gravado) {
        ESP_LOGI(TAG, "[NVS TEST] Aprovado: Gravado=%" PRId32 ", Lido=%" PRId32, corrida_id_gravado, corrida_id_lido);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "[NVS TEST] Reprovado: Valores divergem! Gravado=%" PRId32 ", Lido=%" PRId32, corrida_id_gravado, corrida_id_lido);
        return ESP_FAIL;
    }
}

// ====================================================================
// Cenário 2: Armazenamento e Validação de Dados de Sensores no SPIFFS
// ====================================================================
esp_err_t test_spiffs_storage(void)
{
    ESP_LOGI(TAG, "=== [INICIANDO TESTE SPIFFS] ===");

    // 1. Configurar e registrar o sistema de arquivos SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Falha ao montar ou formatar o SPIFFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Partição SPIFFS não encontrada no mapa de partições");
        } else {
            ESP_LOGE(TAG, "Falha ao inicializar o SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    // Obter informações do SPIFFS
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS montado: Total: %d KB, Usado: %d KB", total/1024, used/1024);
    }

    // 2. Criar e gravar dados de sensores simulados no arquivo
    const char *file_path = "/spiffs/telemetry.json";
    ESP_LOGI(TAG, "Abrindo arquivo %s para gravação...", file_path);
    FILE *f = fopen(file_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir o arquivo para gravação");
        esp_vfs_spiffs_unregister(conf.partition_label);
        return ESP_FAIL;
    }

    // Dados simulados do sensor do Micromouse
    const char *sensor_data_write = "{\"posicao_x\":5,\"posicao_y\":7,\"nivel_bateria\":84.5,\"velocidade\":0.42}";
    ESP_LOGI(TAG, "Escrevendo leituras no arquivo: %s", sensor_data_write);
    fprintf(f, "%s\n", sensor_data_write);
    fclose(f);
    ESP_LOGI(TAG, "Arquivo fechado.");

    // 3. Reabrir o arquivo e ler os dados armazenados
    ESP_LOGI(TAG, "Reabrindo arquivo %s para leitura...", file_path);
    f = fopen(file_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Erro ao abrir o arquivo para leitura");
        esp_vfs_spiffs_unregister(conf.partition_label);
        return ESP_FAIL;
    }

    char line_read[128];
    if (fgets(line_read, sizeof(line_read), f) == NULL) {
        ESP_LOGE(TAG, "Erro ao ler dados do arquivo");
        fclose(f);
        esp_vfs_spiffs_unregister(conf.partition_label);
        return ESP_FAIL;
    }
    fclose(f);

    // Remover caractere de nova linha (\n) do final, se houver
    line_read[strcspn(line_read, "\n")] = 0;
    ESP_LOGI(TAG, "Dados lidos do SPIFFS: %s", line_read);

    // 4. Validar os dados recuperados
    // Fazemos uma validação lógica simples para provar que a string recuperada corresponde ao sensor
    bool validation_pass = true;
    if (strstr(line_read, "\"posicao_x\":5") == NULL) validation_pass = false;
    if (strstr(line_read, "\"posicao_y\":7") == NULL) validation_pass = false;
    if (strstr(line_read, "\"nivel_bateria\":84.5") == NULL) validation_pass = false;

    // Desregistrar SPIFFS para liberar recursos
    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(TAG, "SPIFFS desmontado.");

    // 5. Imprimir resultado do teste
    if (validation_pass) {
        ESP_LOGI(TAG, "[SPIFFS TEST] Aprovado: Leituras dos sensores gravadas e validadas com sucesso!");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "[SPIFFS TEST] Reprovado: Dados lidos divergem dos dados de sensores gravados!");
        return ESP_FAIL;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "Iniciando Testes de Armazenamento Local - Micromouse");
    ESP_LOGI(TAG, "==================================================");

    // Executa Cenário 1 (NVS)
    esp_err_t nvs_res = test_nvs_storage();

    // Pequena pausa entre os testes
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Executa Cenário 2 (SPIFFS)
    esp_err_t spiffs_res = test_spiffs_storage();

    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "Resumo dos Testes de Armazenamento:");
    ESP_LOGI(TAG, "  Teste NVS (Dados Básicos):      %s", (nvs_res == ESP_OK) ? "PASSOU" : "FALHOU");
    ESP_LOGI(TAG, "  Teste SPIFFS (Dados Sensores): %s", (spiffs_res == ESP_OK) ? "PASSOU" : "FALHOU");
    ESP_LOGI(TAG, "==================================================");
}
