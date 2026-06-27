#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "infrared.h"
#include "navigation.h"

//TASK SINALIZA INTERRUPCAO DOS SENSORES FRONTAIS
static void IRAM_ATTR ir_isr(void *arg){
    BaseType_t high_task_wakeup = pdFALSE;

    ir_event_t event;

    switch ((gpio_num_t)arg)
    {
    case IR_FRONT:
        event = IR_EVENT_FRONT;
        break;

    case IR_FL:
        event = IR_EVENT_FRONT_LEFT;
        break;

    case IR_FR:
        event = IR_EVENT_FRONT_RIGHT;
        break;

    default:
        return;
    }

    if (navigation_queue == NULL)
    {
        return;
    }    

    if (xQueueSendFromISR(
            navigation_queue,
            &event,
            &high_task_wakeup) == pdPASS)
    {
        if (high_task_wakeup)
        {
            portYIELD_FROM_ISR();
        }
    }
}

void IR_init(){
    gpio_config_t io_conf_front = {
        .pin_bit_mask =
        (1ULL << IR_FRONT) |
        (1ULL << IR_FR) |
        (1ULL << IR_FL),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config_t io_conf_side = {
        .pin_bit_mask =
        (1ULL << IR_R) |
        (1ULL << IR_L),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
        // .intr_type = GPIO_INTR_POSEDGE //ATIVAR QUANDO FOR TESTAR NO LABIRINTO
    };

    gpio_config(&io_conf_front);
    gpio_config(&io_conf_side);

    gpio_isr_handler_add(
        IR_FRONT,
        ir_isr,
        (void *)IR_FRONT
    );

    // gpio_isr_handler_add(
    //     IR_L,
    //     ir_isr,
    //     (void *)IR_L
    // );

    gpio_isr_handler_add(
        IR_FL,
        ir_isr,
        (void *)IR_FL
    );

    // gpio_isr_handler_add(
    //     IR_R,
    //     ir_isr,
    //     (void *)IR_R
    // );

    gpio_isr_handler_add(
        IR_FR,
        ir_isr,
        (void *)IR_FR
    );
}