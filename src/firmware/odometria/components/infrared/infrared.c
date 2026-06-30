#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "movimentacao.h"
#include "m_driver.h"
#include "odometria.h"
#include "encoder.h"
#include "infrared.h"

static QueueHandle_t sensor_queue;

typedef struct {
    motor_t *mR;
    motor_t *mL;
    encoder_t *eR;
    encoder_t *eL;
    pose_t *p;
} contexto_t;

static contexto_t contexto;

//TASK SINALIZA INTERRUPCAO DOS SENSORES FRONTAIS
static void IRAM_ATTR ir_isr(void *arg){
    gpio_num_t sensor = (gpio_num_t)arg;

    BaseType_t high_task_wakeup = pdFALSE;

    // Evita novas interrupções deste sensor
    gpio_intr_disable(sensor);

    xQueueSendFromISR(
        sensor_queue,
        &sensor,
        &high_task_wakeup
    );

    if(high_task_wakeup){
        portYIELD_FROM_ISR();
    }
}

static void sensor_task(void *arg)
{
    gpio_num_t sensor;
    contexto_t *ctx = (contexto_t *)arg;

    while (1)
    {
        if (xQueueReceive(
                sensor_queue,
                &sensor,
                portMAX_DELAY))
        {
            mouse_break(ctx->mR, ctx->mL);

            vTaskDelay(pdMS_TO_TICKS(50));

            switch(sensor)
            {
                
                case IR_FRONT:
                    if(gpio_get_level(IR_R) == 1){
                        movimentacao_turn_clws(ctx->mR, ctx->mL, ctx->eR,
                                                ctx->eL, ctx->p);
                    } else if (gpio_get_level(IR_L)==1){
                        movimentacao_turn_ctclws(ctx->mR, ctx->mL, ctx->eR,
                                                ctx->eL, ctx->p);
                    } else {
                        mouse_movebwd(ctx->mR, ctx->mL);

                        vTaskDelay(pdMS_TO_TICKS(300));
                        
                        mouse_break(ctx->mR, ctx->mL);
                        //GIRA 180 GRAUS
                        movimentacao_turn_clws(ctx->mR, ctx->mL, ctx->eR,
                                                ctx->eL, ctx->p);

                        vTaskDelay(pdMS_TO_TICKS(50));

                        movimentacao_turn_clws(ctx->mR, ctx->mL, ctx->eR,
                                                ctx->eL, ctx->p);

                    }
                    break;

                case IR_FR:
                    // if(gpio_get_level(IR_FL)==0 && gpio_get_level(IR_R)==1){
                    //     movimentacao_turn_clws(ctx->mR, ctx->mL, ctx->eR,
                    //                             ctx->eL, ctx->p);
                    // } else if (gpio_get_level(IR_FL)==0 && gpio_get_level(IR_L)==1){
                    //     movimentacao_turn_ctclws(ctx->mR, ctx->mL, ctx->eR,
                    //                             ctx->eL, ctx->p);
                    // } else {
                    //     if(gpio_get_level(IR_FR) == 0){
                    //         mouse_movebwd(ctx->mR, ctx->mL);

                    //         vTaskDelay(pdMS_TO_TICKS(300));
                            
                    //         mouse_break(ctx->mR, ctx->mL);

                    //         mouse_spin(ctx->mR, ctx->mL, 0);

                    //         vTaskDelay(pdMS_TO_TICKS(300));
                    //     }
                    //     mouse_break(ctx->mR, ctx->mL);
                    // }

                    if(gpio_get_level(IR_FR) == 0){
                            mouse_movebwd(ctx->mR, ctx->mL);

                            vTaskDelay(pdMS_TO_TICKS(300));
                            
                            mouse_break(ctx->mR, ctx->mL);

                            mouse_spin(ctx->mR, ctx->mL, 0);

                            vTaskDelay(pdMS_TO_TICKS(300));
                        }
                        mouse_break(ctx->mR, ctx->mL);

                    break;

                case IR_FL:
                    // if(gpio_get_level(IR_FR)==0 && gpio_get_level(IR_R)==1){
                    //     movimentacao_turn_clws(ctx->mR, ctx->mL, ctx->eR,
                    //                             ctx->eL, ctx->p);
                    // } else if (gpio_get_level(IR_FR)==0 && gpio_get_level(IR_L)==1){
                    //     movimentacao_turn_ctclws(ctx->mR, ctx->mL, ctx->eR,
                    //                             ctx->eL, ctx->p);
                    // } else {
                    //     if(gpio_get_level(IR_FL) == 0){
                    //         mouse_movebwd(ctx->mR, ctx->mL);

                    //         vTaskDelay(pdMS_TO_TICKS(300));
                            
                    //         mouse_break(ctx->mR, ctx->mL);

                    //         mouse_spin(ctx->mR, ctx->mL, 1);

                    //         vTaskDelay(pdMS_TO_TICKS(300));
                    //     }
                    //     mouse_break(ctx->mR, ctx->mL);
                    // }

                    if(gpio_get_level(IR_FL) == 0){
                        mouse_movebwd(ctx->mR, ctx->mL);

                        vTaskDelay(pdMS_TO_TICKS(300));
                        
                        mouse_break(ctx->mR, ctx->mL);

                        mouse_spin(ctx->mR, ctx->mL, 1);

                        vTaskDelay(pdMS_TO_TICKS(300));
                    }
                    mouse_break(ctx->mR, ctx->mL);

                    break;
                
                default:
                    break;
            }
            // Aguarda estabilizar o sinal
            vTaskDelay(pdMS_TO_TICKS(30));

            // Reabilita a interrupção
            gpio_intr_enable(sensor);
        }
    }
}

void IR_init(motor_t *mR, motor_t *mL, encoder_t *eR, encoder_t *eL, pose_t *p){
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

    contexto.mR = mR;
    contexto.mL = mL;
    contexto.eR = eR;
    contexto.eL = eL;
    contexto.p  = p;

    gpio_config(&io_conf_front);
    gpio_config(&io_conf_side);

    sensor_queue = xQueueCreate(10, sizeof(gpio_num_t));

    xTaskCreate(
        sensor_task,
        "ir_sensor",
        4096,
        &contexto,
        5,
        NULL
    );    

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