#include <stdio.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "m_driver.h"
#include "encoder.h"
#include "odometria.h"
#include "movimentacao.h"

	static const char *TAG = "teste_encoder";

//#define BOTAO_SELETOR GPIO_NUM_16

//static void IRAM_ATTR button_isr_handler()
//{
//    switch (expression)
//    {
//    case constant expression:
//        /* code */
//        break;
//    
//    default:
//        break;
//    }
//}

void app_main(void)
{
//    gpio_config_t io_conf = {
//        .pin_bit_mask = (1ULL << BOTAO_SELETOR),
//        .mode = GPIO_MODE_INPUT,
//        .pull_up_en = GPIO_PULLUP_ENABLE,
//        .pull_down_en = GPIO_PULLDOWN_DISABLE,
//        .intr_type = GPIO_INTR_NEGEDGE
//    };

//    gpio_config(&io_conf);

//    gpio_install_isr_service(0);

//    gpio_isr_handler_add(
//        BOTAO_SELETOR,
//        button_isr_handler,
//        NULL
//    );
//

	encoder_t encoderR;
	encoder_t encoderL;

	encoder_init(&encoderR, GPIO_ENC_R);
	encoder_init(&encoderL, GPIO_ENC_L);
	
        driver_init();

        motor_t motorR = {
                PWM_R1,
                PWM_R2,
                driver_get_cmpr_handlerR1(),
                driver_get_cmpr_handlerR2(),
                driver_get_gen_handlerR1(),
                driver_get_gen_handlerR2()
        };
        motor_t motorL = {
                PWM_L1,
                PWM_L2,
                driver_get_cmpr_handlerL1(),
                driver_get_cmpr_handlerL2(),
                driver_get_gen_handlerL1(),
                driver_get_gen_handlerL2()
        };

        motor_init(&motorR);
        motor_init(&motorL);

    while(1)
    {
        int in = getchar();
        if(in == EOF){
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;

        }

        switch (in){
                case 'f':
                        mouse_movefwd(&motorR, &motorL);
                        break;
                case 'b':
                        mouse_movebwd(&motorR, &motorL);
                        break;
                case 'd':
                        mouse_spin(&motorR, &motorL, 1);
                        break;
                case 'e':
                        mouse_spin(&motorR, &motorL, 0);
                        break;
                case ' ':
                        mouse_break(&motorR, &motorL);

			ESP_LOGI(TAG, "angulo direita: %f", encoder_get_teta(&encoderR));
			ESP_LOGI(TAG, "angulo esquerda: %f", encoder_get_teta(&encoderL));

                        break;
                default:
                        break;

                }
            vTaskDelay(pdMS_TO_TICKS(10));
    }
}
