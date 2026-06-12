#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define SENSOR_1 GPIO_NUM_12
#define SENSOR_2 GPIO_NUM_15
#define SENSOR_3 GPIO_NUM_13

void app_main(void)
{
    gpio_config_t io_conf = {};

    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pin_bit_mask =
        (1ULL << SENSOR_1) |
        (1ULL << SENSOR_2) |
        (1ULL << SENSOR_3);

    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&io_conf);

    while (1)
    {
        int s1 = gpio_get_level(SENSOR_1);
        int s2 = gpio_get_level(SENSOR_2);
        int s3 = gpio_get_level(SENSOR_3);

        printf("S1:%d | S2:%d | S3:%d\n", s1, s2, s3);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}