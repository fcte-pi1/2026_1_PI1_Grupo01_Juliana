//codigo-fonte funcoes relacionadas a odometria do micromouse

#include <stdio.h>

#include "encoder.h"
#include "m_driver.h"
#include "odometria.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "odometria";

/* motor_t motorR = {
    .pwm_gpio = PWM_R,
    .dir_gpio = GPIO_R

    .gen = gen_motorR
    .comp = comp_motorR
};

motor_t motorL = {
    .pwm_gpio = PWM_L,
    .dir_gpio = GPIO_L

    .gen = gen_motorL
    .comp = comp_motorL
}; */

void odometria_update_xy(pose_t *pose, float deslocamento){
    switch (pose->orientacao)
    {
    case NORTE:
        pose->y += deslocamento;
        break;
    case LESTE:
        pose->x += deslocamento;
        break;
    case SUL:
        pose->y -= deslocamento;
        break;
    case OESTE:
        pose->x -= deslocamento;
        break;
    
    default:
        break;
    }
}

void odometria_mudar_sentido(pose_t *pose, bool dir){ //1 - sentido horario; 0 - sentido anti-horario
    if(dir){
        switch (pose->orientacao)
        {
        case NORTE:
            pose->orientacao++;
            break;
        case LESTE:
            pose->orientacao++;
            break;
        case SUL:
            pose->orientacao++;
            break;
        case OESTE:
            pose->orientacao = NORTE;
	    break;
        default:
            break;
        }
    } else {
        switch (pose->orientacao)
        {
        case NORTE:
            pose->orientacao = OESTE;
            break;
        case LESTE:
            pose->orientacao--;
            break;
        case SUL:
            pose->orientacao--;
            break;
        case OESTE:
            pose->orientacao--;
	    break;
        default:
            break;
        }
    }
}

//funcoes acessiveis

const char* odometria_orientacao_string(orientacao_t o){
    switch (o)
    {
    case NORTE:
        return "NORTE";
    case LESTE:
        return "LESTE";
    case SUL:
        return "SUL";
    case OESTE:
        return "OESTE";
    default:
        return "DESCONHECIDA";
    }
}

void odometria_pos_init(pose_t *pose, float x0, float y0, orientacao_t orientacao_inicial){
    pose->x = x0;
    pose->y = y0;
    pose->orientacao = orientacao_inicial;
    pose->theta = 0;

    ESP_LOGI(TAG, "INICIO: (%d, %d) ORIENCACAO %s", pose->x, pose->y, odometria_orientacao_string(pose->orientacao));
}

void odometria_move_cell(){

}
