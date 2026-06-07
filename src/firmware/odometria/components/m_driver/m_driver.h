#ifndef M_DRIVER_H
#define M_DRIVER_H


#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_DIR_R) | (1ULL<<GPIO_DIR_L))

#define PWM_R1              19
#define PWM_R2              18
#define PWM_L1              23
#define PWM_L2		    17

#define HZ_RES              1000000
#define PERIOD_TICKS        500

#define MOTOR_FWD_SPD 60 
#define MOTOR_BWD_SPD 60

#include<stdint.h>
#include<stdbool.h>

#include "driver/gpio.h"
#include "driver/mcpwm_prelude.h"

//struct do motor para modularizar as inicializacoes
typedef struct{
    gpio_num_t pwm_gpio1;
    gpio_num_t pwm_gpio2;

    mcpwm_cmpr_handle_t comp1; 
    mcpwm_cmpr_handle_t comp2; 
    mcpwm_gen_handle_t  gen1;
    mcpwm_gen_handle_t  gen2;
} motor_t;

void driver_init();

mcpwm_cmpr_handle_t driver_get_cmpr_handlerR1();

mcpwm_cmpr_handle_t driver_get_cmpr_handlerR2();

mcpwm_cmpr_handle_t driver_get_cmpr_handlerL1();

mcpwm_cmpr_handle_t driver_get_cmpr_handlerL2();

mcpwm_gen_handle_t driver_get_gen_handlerR1();

mcpwm_gen_handle_t driver_get_gen_handlerR2();

mcpwm_gen_handle_t driver_get_gen_handlerL1();

mcpwm_gen_handle_t driver_get_gen_handlerL2();

void motor_init(motor_t *motor);

void motor_set_speed(motor_t *motor, int8_t speed);

void motor_stop(motor_t *motor);

#endif
