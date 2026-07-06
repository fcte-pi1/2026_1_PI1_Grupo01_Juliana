//codigo-fonte leitura dos encoders acoplados aos motores
//abaixo estao declaradas as bibliotecas necessarias

#include <stdio.h>
#include <math.h>

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_err.h"
#include "esp_log.h"

#include "encoder.h"

static const char *TAG = "encoder";

//rotina de configuracoes iniciais do periferico pcnt
esp_err_t encoder_init(encoder_t *encoder, gpio_num_t gpio)
{
    if (encoder == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    encoder->gpio = gpio;

    pcnt_unit_config_t unit_config = {
        .high_limit = 32767,
        .low_limit = -32768,
    };

    ESP_ERROR_CHECK(
        pcnt_new_unit(
            &unit_config,
            &(encoder->unit)
        )
    );

    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = gpio,
        .level_gpio_num = -1,
    };

    ESP_ERROR_CHECK(
        pcnt_new_channel(
            encoder->unit,
            &chan_config,
            &(encoder->channel)
        )
    );

    ESP_ERROR_CHECK(
        pcnt_channel_set_edge_action(
            encoder->channel,
            PCNT_CHANNEL_EDGE_ACTION_INCREASE,
            PCNT_CHANNEL_EDGE_ACTION_HOLD
        )
    );

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };

    ESP_ERROR_CHECK(
        pcnt_unit_set_glitch_filter(
            encoder->unit,
            &filter_config
        )
    );

    ESP_ERROR_CHECK(
        pcnt_unit_enable(
            encoder->unit
        )
    );

    ESP_ERROR_CHECK(
        pcnt_unit_clear_count(
            encoder->unit
        )
    );

    ESP_ERROR_CHECK(
        pcnt_unit_start(
            encoder->unit
        )
    );

    ESP_LOGI(TAG, "encoder: iniciado"); 
    return ESP_OK;
}

void encoder_clean(encoder_t *encoder){

    pcnt_unit_clear_count(encoder->unit);
    
    ESP_LOGI(TAG, "encoder resetado");
}

static int encoder_consume_count(encoder_t *encoder)
{
    int count = 0;

    pcnt_unit_get_count(encoder->unit, &count);
    pcnt_unit_clear_count(encoder->unit);

    ESP_LOGD(TAG, "pulsos lidos: %d", count);
    return count;
}

//movimento estimado do eixo do motor [rad]
//baseado na condicao de que a leitura e feita
//com o carrinho parado...
float encoder_get_teta(encoder_t *encoder){
    int pulsos = encoder_consume_count(encoder);
    return (pulsos * M_PI) / 10.0f;
}

//velocidade angular media [rad/s]
float encoder_get_w(encoder_t *encoder, float dt){
    
    float dteta = encoder_get_teta(encoder);

    float w = dteta/dt;

    return w;
}

//velocidade linear media [m/s]
float encoder_get_v(encoder_t *encoder, float dt, float raio){
    float w = encoder_get_w(encoder, dt);
    float v = w * raio;

    return v;
}

//deslocamento estimado [m]
float encoder_get_deslocamento(encoder_t *encoder, float raio, int *pulsos_lidos){

    int pulsos = encoder_consume_count(encoder);
    if (pulsos_lidos != NULL) {
        *pulsos_lidos = pulsos;
    }

    float teta = (pulsos * M_PI) / 10.0f;
    return teta * raio;
}
