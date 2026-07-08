#ifndef ENCODER_H
#define ENCODER_H

#define GPIO_ENC_R 35
#define GPIO_ENC_L 32

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"

//struct para unidade de periferico pcnt
typedef struct
{
    gpio_num_t gpio;

    pcnt_unit_handle_t unit;
    pcnt_channel_handle_t channel;

} encoder_t;

esp_err_t encoder_init(encoder_t *encoder, gpio_num_t gpio);

void encoder_clean(encoder_t *encoder);

float encoder_get_teta(encoder_t *encoder);

float encoder_get_w(encoder_t *encoder, float dt);

float encoder_get_v(encoder_t *encoder, float dt, float raio);

float encoder_get_deslocamento(encoder_t *encoder, float raio, int *pulsos_lidos);

#endif
