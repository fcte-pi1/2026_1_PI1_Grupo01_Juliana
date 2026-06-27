#ifndef INFRARED_H
#define INFRARED_H

//pensando em adicionar mais um sensor para frentee
#define IR_FRONT GPIO_NUM_34
#define IR_FR GPIO_NUM_16
#define IR_FL GPIO_NUM_26
#define IR_R GPIO_NUM_13
#define IR_L GPIO_NUM_14

typedef enum
{
    IR_EVENT_FRONT,
    IR_EVENT_FRONT_LEFT,
    IR_EVENT_FRONT_RIGHT
} ir_event_t;

//interrupcoes de IR_L e IR_R so poderao ser testadas no labirinto
// LINHA 65 DE infrared.c 

//INCLUIR ESSA FUNCAO EM app_main() APOS INICIALIZAR MOTORES, ENCODERS E POSE
void IR_init();

// TAMBEM LEMBRAR DE INCLUIR EM app_main()
// 'gpio_install_isr_service(0);'

#endif