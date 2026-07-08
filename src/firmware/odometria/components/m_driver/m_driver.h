#ifndef M_DRIVER_H
#define M_DRIVER_H

#define PWM_R1              19
#define PWM_R2              18
#define PWM_L1              23
#define PWM_L2		        17
#define SEL                 27

#define HZ_RES              1000000
#define PERIOD_TICKS        500


//talvez mudar para BASE_SPD
#define MOTOR_FWD_SPD 40
#define MOTOR_BWD_SPD 40
#define MOTOR_RE_SPD  30

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

/*COMO INICIALIZAR OS MOTORES
NO INICIO, ANTES DA FUNCAO app_main(), DECLARAR OS DOIS MOTORES:

    motor_t motorR = { .pwm_gpio1 = PWM_R1, .pwm_gpio2 = PWM_R2};
    motor_t motorL = { .pwm_gpio1 = PWM_L1, .pwm_gpio2 = PWM_L2};

POR FIM, DENTRO DE app_main() FAÇA:

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

    PRONTO
*/

//INCLUIR ESSAS FUNCOES EM app_main()
void driver_init();

void motor_init(motor_t *motor);

//funcoes p controle dos motores
void motor_set_speed(motor_t *motor, int8_t speed);

void motor_stop(motor_t *motor);

#endif
