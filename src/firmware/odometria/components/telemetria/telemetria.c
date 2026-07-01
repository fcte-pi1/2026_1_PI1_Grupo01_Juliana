// Modulo de transmissao de telemetria via Wi-Fi + WebSocket (issue #15).
// Ver telemetria.h para a visao geral. Best-effort: falha de rede nunca trava
// a navegacao.

#include "telemetria.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "esp_websocket_client.h"
#include "cJSON.h"

// ===========================================================================
// CONFIGURACAO (preencher antes de testar com a ESP)
// ===========================================================================

// Rede Wi-Fi que a ESP usa. Dica: hotspot de celular costuma ser o mais
// garantido para a demo (a rede da facul as vezes bloqueia dispositivo a
// dispositivo). TODO: preencher com a rede do dia.
#define WIFI_SSID   "COLOQUE_O_SSID"
#define WIFI_PASS   "COLOQUE_A_SENHA"

// Endereco do backend (uvicorn) na rede. Trocar o IP pelo da maquina que
// estiver rodando o backend. A porta padrao do projeto e 8000.
// TODO: ajustar o IP.
#define BACKEND_WS_URI  "ws://192.168.1.8:8000/ws/telemetria"

// Quantos ms esperar pelo Wi-Fi e pelo WebSocket antes de desistir (best-effort).
#define WIFI_TIMEOUT_MS   10000
#define WS_TIMEOUT_MS      5000

// Servidor NTP para datar os pacotes (opcional). Se nao houver internet na
// rede, o modulo usa um relogio-base local (timestamp valido, porem nao e a
// hora real do mundo).
#define NTP_SERVER        "pool.ntp.org"
#define NTP_TIMEOUT_MS     5000
// Base usada quando o NTP nao sincroniza: 2026-06-30T00:00:00 UTC (epoch).
#define RELOGIO_BASE_EPOCH  1782777600

static const char *TAG = "telemetria";

// ===========================================================================
// Estado interno
// ===========================================================================

#define WIFI_CONECTADO_BIT  BIT0
#define WS_CONECTADO_BIT    BIT1

static EventGroupHandle_t s_eventos;
static esp_websocket_client_handle_t s_ws;
static volatile bool s_ws_conectado = false;
static int  s_labirinto_id = 1;
static volatile int s_corrida_id = -1; // -1 = ainda nao sei (uso labirinto_id)

// ===========================================================================
// Relogio / timestamp ISO8601
// ===========================================================================
static void relogio_init(void)
{
    // Tenta NTP (best-effort). Se nao sincronizar, fixa um relogio-base para
    // que os timestamps ainda saiam em ISO8601 valido e monotonico.
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER);
    esp_sntp_init();

    int esperou = 0;
    struct tm info = {0};
    while (esperou < NTP_TIMEOUT_MS) {
        time_t agora = time(NULL);
        gmtime_r(&agora, &info);
        if (info.tm_year >= (2020 - 1900)) {
            ESP_LOGI(TAG, "hora sincronizada via NTP");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        esperou += 500;
    }

    struct timeval tv = { .tv_sec = RELOGIO_BASE_EPOCH, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    ESP_LOGW(TAG, "NTP nao sincronizou; usando relogio-base local");
}

static void iso8601_agora(char *buf, size_t n)
{
    time_t agora = time(NULL);
    struct tm info;
    gmtime_r(&agora, &info);
    strftime(buf, n, "%Y-%m-%dT%H:%M:%SZ", &info);
}

// ===========================================================================
// Wi-Fi (station mode)
// ===========================================================================
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi caiu, tentando reconectar...");
        xEventGroupClearBits(s_eventos, WIFI_CONECTADO_BIT);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evento = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Wi-Fi conectado, IP: " IPSTR, IP2STR(&evento->ip_info.ip));
        xEventGroupSetBits(s_eventos, WIFI_CONECTADO_BIT);
    }
}

static bool wifi_init(void)
{
    // nvs e event loop podem ja ter sido inicializados em outro lugar.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_err_t loop = esp_event_loop_create_default();
    if (loop != ESP_OK && loop != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "falha ao criar event loop: %s", esp_err_to_name(loop));
        return false;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    EventBits_t bits = xEventGroupWaitBits(s_eventos, WIFI_CONECTADO_BIT,
                                           pdFALSE, pdTRUE,
                                           pdMS_TO_TICKS(WIFI_TIMEOUT_MS));
    return (bits & WIFI_CONECTADO_BIT) != 0;
}

// ===========================================================================
// WebSocket
// ===========================================================================
static void ws_event_handler(void *arg, esp_event_base_t base,
                             int32_t id, void *data)
{
    esp_websocket_event_data_t *ev = (esp_websocket_event_data_t *)data;

    switch (id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket conectado");
        s_ws_conectado = true;
        xEventGroupSetBits(s_eventos, WS_CONECTADO_BIT);
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WebSocket desconectado");
        s_ws_conectado = false;
        xEventGroupClearBits(s_eventos, WS_CONECTADO_BIT);
        break;

    case WEBSOCKET_EVENT_DATA:
        // O backend retransmite o evento persistido, que traz o corrida_id.
        // Capturamos para os proximos pacotes irem na mesma corrida.
        if (ev && ev->data_ptr && ev->data_len > 0 && ev->op_code == 0x01) {
            cJSON *json = cJSON_ParseWithLength(ev->data_ptr, ev->data_len);
            if (json) {
                cJSON *cid = cJSON_GetObjectItem(json, "corrida_id");
                if (cJSON_IsNumber(cid)) {
                    s_corrida_id = cid->valueint;
                }
                cJSON_Delete(json);
            }
        }
        break;

    default:
        break;
    }
}

static bool ws_init(void)
{
    esp_websocket_client_config_t cfg = {
        .uri = BACKEND_WS_URI,
    };

    s_ws = esp_websocket_client_init(&cfg);
    if (s_ws == NULL) {
        ESP_LOGE(TAG, "falha ao criar cliente WebSocket");
        return false;
    }

    esp_websocket_register_events(s_ws, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL);
    esp_websocket_client_start(s_ws);

    EventBits_t bits = xEventGroupWaitBits(s_eventos, WS_CONECTADO_BIT,
                                           pdFALSE, pdTRUE,
                                           pdMS_TO_TICKS(WS_TIMEOUT_MS));
    return (bits & WS_CONECTADO_BIT) != 0;
}

// ===========================================================================
// API publica
// ===========================================================================
bool telemetria_init(int labirinto_id)
{
    s_labirinto_id = labirinto_id;
    s_corrida_id = -1;
    s_eventos = xEventGroupCreate();

    ESP_LOGI(TAG, "subindo Wi-Fi (SSID: %s)...", WIFI_SSID);
    if (!wifi_init()) {
        ESP_LOGW(TAG, "Wi-Fi nao conectou em %d ms; seguindo SEM telemetria",
                 WIFI_TIMEOUT_MS);
        return false;
    }

    relogio_init();

    ESP_LOGI(TAG, "conectando WebSocket em %s ...", BACKEND_WS_URI);
    if (!ws_init()) {
        ESP_LOGW(TAG, "WebSocket nao conectou; seguindo SEM telemetria");
        return false;
    }

    ESP_LOGI(TAG, "telemetria pronta (labirinto_id=%d)", s_labirinto_id);
    return true;
}

void telemetria_envia(int linha, int coluna, float nivel_bateria, float velocidade)
{
    if (!s_ws_conectado || s_ws == NULL) {
        return; // best-effort: sem conexao, nao faz nada
    }

    char ts[32];
    iso8601_agora(ts, sizeof(ts));

    cJSON *pacote = cJSON_CreateObject();
    if (pacote == NULL) {
        return;
    }

    // Primeiro pacote abre a corrida via labirinto_id; depois reaproveita o
    // corrida_id devolvido pelo backend.
    if (s_corrida_id > 0) {
        cJSON_AddNumberToObject(pacote, "corrida_id", s_corrida_id);
    } else {
        cJSON_AddNumberToObject(pacote, "labirinto_id", s_labirinto_id);
    }

    cJSON_AddStringToObject(pacote, "timestamp", ts);
    cJSON_AddNumberToObject(pacote, "posicao_x", coluna);
    cJSON_AddNumberToObject(pacote, "posicao_y", linha);
    cJSON_AddNumberToObject(pacote, "nivel_bateria", nivel_bateria);
    if (velocidade >= 0.0f) {
        cJSON_AddNumberToObject(pacote, "velocidade", velocidade);
    }

    char *texto = cJSON_PrintUnformatted(pacote);
    if (texto != NULL) {
        esp_websocket_client_send_text(s_ws, texto, strlen(texto),
                                       pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "telemetria enviada: %s", texto);
        cJSON_free(texto);
    }

    cJSON_Delete(pacote);
}

void telemetria_stop(void)
{
    if (s_ws != NULL) {
        esp_websocket_client_stop(s_ws);
        esp_websocket_client_destroy(s_ws);
        s_ws = NULL;
        s_ws_conectado = false;
    }
}
