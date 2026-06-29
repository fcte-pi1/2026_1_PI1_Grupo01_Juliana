//codigo-fonte funcoes relacionadas a odometria do micromouse

#include <stdio.h>

#include "encoder.h"
#include "m_driver.h"
#include "odometria.h"

#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "odometria";

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

void odometria_update_vm(pose_t *pose, float vm_novo){
    float sum_vm = (pose->vm * (pose->cell_count -1)) + vm_novo;

    pose->vm = sum_vm / pose->cell_count;
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

//retorna tempo de execucao ate agora em segundos
float odometria_get_segundos(){
    int64_t tempo_atual = esp_timer_get_time();

    float time = tempo_atual/1000000.0f;

    return time;
}

void odometria_pos_init(pose_t *pose, float x0, float y0, orientacao_t orientacao_inicial){
    pose->x = x0;
    pose->y = y0;
    pose->orientacao = orientacao_inicial;
    // pose->theta = 0;
    pose->vm = 0;
    pose->cell_count = 0;

    ESP_LOGI(TAG, "INICIO: (%f, %f) ORIENTACAO %s", pose->x, pose->y, odometria_orientacao_string(pose->orientacao));
}