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

motor_t motorR = { .pwm_gpio1 = PWM_R1, .pwm_gpio2 = PWM_R2};
motor_t motorL = { .pwm_gpio1 = PWM_L1, .pwm_gpio2 = PWM_L2};

void app_main(void)
{
        encoder_t encoderR;
	encoder_t encoderL;

	encoder_init(&encoderR, GPIO_ENC_R);
	encoder_init(&encoderL, GPIO_ENC_L);
	
        driver_init();

        motor_init(&motorR);
        motor_init(&motorL);

        motorR = (motor_t){
                PWM_R1,
                PWM_R2,
                motorR.comp1,
                motorR.comp2,
                motorR.gen1,
                motorR.gen2
        };

        motorL = (motor_t){
                PWM_L1,
                PWM_L2,
                motorL.comp1,
                motorL.comp2,
                motorL.gen1,
                motorL.gen2
        };

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
