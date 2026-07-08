#ifndef INFRARED_H
#define INFRARED_H

//pensando em adicionar mais um sensor para frentee
// #define IR_FRONT GPIO_NUM_34
#define IR_FR GPIO_NUM_16
#define IR_FL GPIO_NUM_26
#define IR_R GPIO_NUM_13
#define IR_L GPIO_NUM_14

//INCLUIR ESSA FUNCAO EM app_main() APOS INICIALIZAR MOTORES, ENCODERS E POSE
void IR_init();

// Tenta virar quando encostou na parede e ha abertura lateral (recuperacao de canto).
bool infrared_recuperar_canto(motor_t *mR, motor_t *mL, encoder_t *eR, encoder_t *eL, pose_t *p);

// Sequencia completa apos travamento: re + virada para sair da parede.
bool infrared_recuperar_travamento(motor_t *mR, motor_t *mL, encoder_t *eR, encoder_t *eL, pose_t *p);

// TAMBEM LEMBRAR DE INCLUIR EM app_main()
// 'gpio_install_isr_service(0);'

#endif