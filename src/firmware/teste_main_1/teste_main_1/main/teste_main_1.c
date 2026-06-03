#include <stdio.h>

#define BOTAO_SELETOR GPIO_NUM_16

static void IRAM_ATTR button_isr_handler()
{
    switch (expression)
    {
    case constant expression:
        /* code */
        break;
    
    default:
        break;
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOTAO_SELETOR),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    gpio_config(&io_conf);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(
        BOTAO_SELETOR,
        button_isr_handler,
        NULL
    );
}
