#ifndef M_DRIVER_H
#define M_DRIVER_H

#define GPIO_DIR_R              18
#define GPIO_DIR_L              17

#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_DIR_R) | (1ULL<<GPIO_DIR_L))

#define PWM_R               19
#define PWM_L               5

#define HZ_RES              1000000
#define PERIOD_TICKS        500

#define MOTOR_FWD_SPD 65
#define MOTOR_BWD_SPD 50

#include<stdint.h>
#include<stdbool.h>

#include "driver/gpio.h"
#include "driver/mcpwm_prelude.h"

//struct do motor para modularizar as inicializacoes
typedef struct{
    gpio_num_t pwm_gpio;
    gpio_num_t dir_gpio;

    mcpwm_cmpr_handle_t comp; 
    mcpwm_gen_handle_t  gen;
} motor_t;

void driver_init();

mcpwm_cmpr_handle_t driver_get_cmpr_handlerR();

mcpwm_cmpr_handle_t driver_get_cmpr_handlerL();

mcpwm_gen_handle_t driver_get_gen_handlerR();

mcpwm_gen_handle_t driver_get_gen_handlerL();

void motor_init(motor_t *motor);

void motor_set_speed(motor_t *motor, int8_t speed);

void motor_stop(motor_t *motor);

#endif
