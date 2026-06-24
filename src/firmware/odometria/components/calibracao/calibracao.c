// codigo-fonte: verificacao/validacao dos encoders e estimativa de trim

#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "calibracao.h"
#include "m_driver.h"
#include "encoder.h"

static const char *TAG = "calibracao";

void calibracao_padrao(calib_t *c)
{
    if (c == NULL) {
        return;
    }
    c->trim_dir = 1.0f;
    c->trim_esq = 1.0f;
    c->kp       = CALIB_KP_PADRAO;
    c->ki       = CALIB_KI_PADRAO;
    c->valido   = false;
}

const char *calibracao_status_str(calib_status_t s)
{
    switch (s) {
    case CALIB_OK:           return "OK";
    case CALIB_ERRO_ENC_DIR: return "FALHA encoder DIREITO";
    case CALIB_ERRO_ENC_ESQ: return "FALHA encoder ESQUERDO";
    case CALIB_ERRO_AMBOS:   return "FALHA ambos os encoders";
    case CALIB_ERRO_RUIDO:   return "RUIDO/glitch com carrinho parado";
    case CALIB_ASSIMETRIA:   return "ASSIMETRIA excessiva entre as rodas";
    default:                 return "DESCONHECIDO";
    }
}

// aciona os dois motores para frente, espera, freia e devolve os pulsos
// (em valor absoluto) lidos em cada roda durante o intervalo.
static void acionar_e_medir(motor_t *mR, motor_t *mL,
                            encoder_t *eR, encoder_t *eL,
                            int pwm, int tempo_ms,
                            int *pulsos_dir, int *pulsos_esq)
{
    encoder_clean(eR);
    encoder_clean(eL);

    motor_set_speed(mR, pwm);
    motor_set_speed(mL, pwm);

    vTaskDelay(pdMS_TO_TICKS(tempo_ms));

    motor_stop(mR);
    motor_stop(mL);

    *pulsos_dir = abs(encoder_peek_count(eR));
    *pulsos_esq = abs(encoder_peek_count(eL));

    // pequena folga para o carrinho parar antes de novas leituras
    vTaskDelay(pdMS_TO_TICKS(150));
}

calib_status_t calibracao_validar_encoders(motor_t *mR, motor_t *mL,
                                           encoder_t *eR, encoder_t *eL)
{
    ESP_LOGI(TAG, "iniciando verificacao dos encoders...");

    // 1) teste de repouso: parado nao pode contar (detecta ruido/glitch/fiacao)
    motor_stop(mR);
    motor_stop(mL);
    encoder_clean(eR);
    encoder_clean(eL);
    vTaskDelay(pdMS_TO_TICKS(200));

    int ruido_dir = abs(encoder_peek_count(eR));
    int ruido_esq = abs(encoder_peek_count(eL));
    if (ruido_dir > CALIB_RUIDO_MAX || ruido_esq > CALIB_RUIDO_MAX) {
        ESP_LOGE(TAG, "ruido em repouso (dir=%d, esq=%d). Verifique fiacao/filtro.",
                 ruido_dir, ruido_esq);
        return CALIB_ERRO_RUIDO;
    }

    // 2) teste de acionamento: cada encoder deve gerar pulsos
    int pdir = 0, pesq = 0;
    acionar_e_medir(mR, mL, eR, eL, CALIB_PWM_TESTE, CALIB_TEMPO_MS, &pdir, &pesq);
    ESP_LOGI(TAG, "pulsos no acionamento: dir=%d, esq=%d", pdir, pesq);

    bool dir_morto = (pdir < CALIB_MIN_PULSOS);
    bool esq_morto = (pesq < CALIB_MIN_PULSOS);

    if (dir_morto && esq_morto) {
        ESP_LOGE(TAG, "nenhum encoder respondeu");
        return CALIB_ERRO_AMBOS;
    }
    if (dir_morto) {
        ESP_LOGE(TAG, "encoder direito sem pulsos");
        return CALIB_ERRO_ENC_DIR;
    }
    if (esq_morto) {
        ESP_LOGE(TAG, "encoder esquerdo sem pulsos");
        return CALIB_ERRO_ENC_ESQ;
    }

    // 3) checagem de assimetria grosseira
    int maior = (pdir > pesq) ? pdir : pesq;
    int menor = (pdir < pesq) ? pdir : pesq;
    float razao = (menor > 0) ? ((float) maior / (float) menor) : 999.0f;
    ESP_LOGI(TAG, "razao de assimetria: %.2f (limite %.2f)", razao, CALIB_ASSIMETRIA_MAX);

    if (razao > CALIB_ASSIMETRIA_MAX) {
        ESP_LOGW(TAG, "assimetria alta: encoders ok, mas rodas/motores muito diferentes");
        return CALIB_ASSIMETRIA;
    }

    ESP_LOGI(TAG, "encoders validados com sucesso");
    return CALIB_OK;
}

calib_status_t calibracao_estimar_trim(motor_t *mR, motor_t *mL,
                                       encoder_t *eR, encoder_t *eL,
                                       calib_t *cal)
{
    if (cal == NULL) {
        return CALIB_ERRO_AMBOS;
    }

    calibracao_padrao(cal);

    int pdir = 0, pesq = 0;
    acionar_e_medir(mR, mL, eR, eL, CALIB_PWM_TESTE, CALIB_TEMPO_MS, &pdir, &pesq);
    ESP_LOGI(TAG, "estimativa de trim: dir=%d, esq=%d", pdir, pesq);

    if (pdir < CALIB_MIN_PULSOS || pesq < CALIB_MIN_PULSOS) {
        ESP_LOGE(TAG, "pulsos insuficientes para estimar trim");
        return (pdir < CALIB_MIN_PULSOS && pesq < CALIB_MIN_PULSOS) ? CALIB_ERRO_AMBOS
             : (pdir < CALIB_MIN_PULSOS) ? CALIB_ERRO_ENC_DIR : CALIB_ERRO_ENC_ESQ;
    }

    // a roda mais lenta vira referencia: a mais rapida recebe trim < 1.0
    if (pdir >= pesq) {
        cal->trim_dir = (float) pesq / (float) pdir; // freia a direita
        cal->trim_esq = 1.0f;
    } else {
        cal->trim_dir = 1.0f;
        cal->trim_esq = (float) pdir / (float) pesq; // freia a esquerda
    }

    cal->valido = true;
    ESP_LOGI(TAG, "trim estimado -> dir=%.3f, esq=%.3f", cal->trim_dir, cal->trim_esq);

    int maior = (pdir > pesq) ? pdir : pesq;
    int menor = (pdir < pesq) ? pdir : pesq;
    float razao = (float) maior / (float) menor;
    if (razao > CALIB_ASSIMETRIA_MAX) {
        ESP_LOGW(TAG, "trim aplicado, mas assimetria mecanica e alta (razao %.2f)", razao);
        return CALIB_ASSIMETRIA;
    }

    return CALIB_OK;
}
