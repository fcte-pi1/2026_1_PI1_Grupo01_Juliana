#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "m_driver.h"

void app_main(void)
{
        driver_init();

        motor_t motorR = {
                PWM_R,
                GPIO_DIR_R,
                driver_get_cmpr_handlerR(),
                driver_get_gen_handlerR()
        };
        motor_t motorL = {
                PWM_L,
                GPIO_DIR_L,
                driver_get_cmpr_handlerL(),
                driver_get_gen_handlerL()
        };

        motor_init(&motorR);
        motor_init(&motorL);

	motor_set_speed(&motorR, 40);
	motor_set_speed(&motorL, 40);
	
	vTaskDelay(pdMS_TO_TICKS(500));

	motor_stop(&motorR);
	motor_stop(&motorL);

	vTaskDelay(pdMS_TO_TICKS(500));

	motor_set_speed(&motorR, 40);
	
	vTaskDelay(pdMS_TO_TICKS(500));

	motor_stop(&motorR);

	vTaskDelay(pdMS_TO_TICKS(500));

	motor_set_speed(&motorL, 40);

	vTaskDelay(pdMS_TO_TICKS(500));

	motor_stop(&motorR);
	motor_stop(&motorL);
}

