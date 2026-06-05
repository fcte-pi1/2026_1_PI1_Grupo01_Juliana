//codigo-fonte driver de motores com drv8833 no modo pwm
//abaixo estao declaradas as bibliotecas necessarias
#include "m_driver.h"

#include "driver/mcpwm_prelude.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "motor";

//timer
static mcpwm_timer_handle_t timer = NULL;
static mcpwm_oper_handle_t oper1 = NULL;
static mcpwm_oper_handle_t oper2 = NULL;

//comparadores (um par para cada motor)
static mcpwm_cmpr_handle_t cmp_motorR1;
static mcpwm_cmpr_handle_t cmp_motorR2;

static mcpwm_cmpr_handle_t cmp_motorL1;
static mcpwm_cmpr_handle_t cmp_motorL2;

//geradores (um par para cada motor)
static mcpwm_gen_handle_t gen_motorR1;
static mcpwm_gen_handle_t gen_motorR2;

static mcpwm_gen_handle_t gen_motorL1;
static mcpwm_gen_handle_t gen_motorL2;


mcpwm_cmpr_handle_t driver_get_cmpr_handlerR1(){
	return cmp_motorR1; 
	}
mcpwm_cmpr_handle_t driver_get_cmpr_handlerR2(){
	return cmp_motorR2; 
	}

mcpwm_cmpr_handle_t driver_get_cmpr_handlerL1(){
	return cmp_motorL1; 
	}
mcpwm_cmpr_handle_t driver_get_cmpr_handlerL2(){
	return cmp_motorL2; 
	}

mcpwm_gen_handle_t driver_get_gen_handlerR1(){
	return gen_motorR1; 
	}
mcpwm_gen_handle_t driver_get_gen_handlerR2(){
	return gen_motorR2; 
	}

mcpwm_gen_handle_t driver_get_gen_handlerL1(){
	return gen_motorL1; 
	}
mcpwm_gen_handle_t driver_get_gen_handlerL2(){
	return gen_motorL2; 
	}
//rotina de configuracoes iniciais para timer, operador e gpio
void driver_init(){
    //configuracao da frequencia do timer pwm
    //para este projeto, estaremos usando 20kHz

    //(freq = resolution_hz/period_ticks)

    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = HZ_RES,
        .period_ticks = PERIOD_TICKS,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP
    };

    mcpwm_new_timer(
        &timer_config,
        &timer
    );

    //configurando os operadores
    mcpwm_operator_config_t operator_config = {
        .group_id = 0
    };

    mcpwm_new_operator(
        &operator_config,
        &oper1
    );

    mcpwm_new_operator(
        &operator_config,
        &oper2
    );


    mcpwm_operator_connect_timer(
        oper1,
        timer
    );
    mcpwm_operator_connect_timer(
        oper2,
        timer
    );

    //inicia timer
    mcpwm_timer_enable(timer);

    mcpwm_timer_start_stop(
        timer,
        MCPWM_TIMER_START_NO_STOP
    );
}

//declaracoes necessarias para operacao do periferico "MCPWM" (motor control pulse width modulation) da ESP32
void motor_init(motor_t *motor){

    //configurando e criando os comparadores
    mcpwm_comparator_config_t cmp_config = {
        .flags.update_cmp_on_tez = true
    };
	//comparador1
    mcpwm_new_comparator(
        oper1,
        &cmp_config,
        &motor->comp1
    );
	//comparador2
    mcpwm_new_comparator(
        oper2,
        &cmp_config,
        &motor->comp2
    );
    
    //configurando os geradores
    mcpwm_generator_config_t gen_config;
	//gerador1
    gen_config.gen_gpio_num = motor->pwm_gpio1;
    mcpwm_new_generator(
        oper1,
        &gen_config,
        &motor->gen1
    );
	//gerador2
    gen_config.gen_gpio_num = motor->pwm_gpio2;
    mcpwm_new_generator(
        oper2,
        &gen_config,
        &motor->gen2
    );

    //configurando PWM
    mcpwm_generator_set_action_on_timer_event(
        motor->gen1,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            MCPWM_GEN_ACTION_HIGH
        )
    );

    mcpwm_generator_set_action_on_compare_event(
        motor->gen1,
        MCPWM_GEN_COMPARE_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            motor->comp1,
            MCPWM_GEN_ACTION_LOW
        )
    );

    mcpwm_generator_set_action_on_timer_event(
        motor->gen2,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            MCPWM_GEN_ACTION_HIGH
        )
    );

    mcpwm_generator_set_action_on_compare_event(
        motor->gen2,
        MCPWM_GEN_COMPARE_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            motor->comp2,
            MCPWM_GEN_ACTION_LOW
        )
    );

    ESP_LOGI(TAG, "INICIANDO MOTOR DESLIGADO");
    //iniciar motor desligado
    motor_set_speed(motor, 0);
}

//funcoes para controle de velocidade
static void set_pwm(
    mcpwm_cmpr_handle_t cmp,
    uint8_t duty
)
{
    uint32_t ticks =
        (PERIOD_TICKS * duty) / 100;

    mcpwm_comparator_set_compare_value(
        cmp,
        ticks
    );
}

void motor_set_speed(motor_t *motor, int8_t speed){
    ESP_LOGI(TAG, "velocidade do motor: %d", speed);
        if(speed >= 25 && speed <= 100)
    {
        set_pwm(motor->comp1, speed);
	set_pwm(motor->comp2, 0);
    }
    else if(speed <= -25 && speed >= -100)
    {
	set_pwm(motor->comp1, 0);
        set_pwm(motor->comp2, -speed);
    }
    else
    {
        set_pwm(motor->comp1, 0);
        set_pwm(motor->comp2, 0);
    }
}

void motor_stop(motor_t *motor){
        set_pwm(motor->comp1, 0);
        set_pwm(motor->comp2, 0);
}


