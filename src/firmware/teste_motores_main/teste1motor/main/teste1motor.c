#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "m_driver.h"

motor_t motorR = { .pwm_gpio1 = PWM_R1, .pwm_gpio2 = PWM_R2};
motor_t motorL = { .pwm_gpio1 = PWM_L1, .pwm_gpio2 = PWM_L2};

void app_main(void)
{
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

	motor_set_speed(&motorR, 31);
	motor_set_speed(&motorL, 31);
	
	vTaskDelay(pdMS_TO_TICKS(1500));

	motor_stop(&motorR);
	motor_stop(&motorL);

	vTaskDelay(pdMS_TO_TICKS(1500));

	motor_set_speed(&motorR, 40);
	
	vTaskDelay(pdMS_TO_TICKS(1500));

	motor_stop(&motorR);

	vTaskDelay(pdMS_TO_TICKS(1500));

	motor_set_speed(&motorL, 40);

	vTaskDelay(pdMS_TO_TICKS(1500));

	motor_stop(&motorR);
	motor_stop(&motorL);
}

