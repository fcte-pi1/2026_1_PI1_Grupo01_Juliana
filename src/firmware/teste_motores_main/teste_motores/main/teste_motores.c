#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "m_driver.h"
#include "movimentacao.h"

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
	
//	char linha[32];

//      int  index = 0;


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
			break;
		default:
			break;

		}
	    vTaskDelay(pdMS_TO_TICKS(10));
    }
}
