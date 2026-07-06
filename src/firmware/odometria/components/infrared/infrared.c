#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // Necessário para o Mutex
#include "driver/gpio.h"
#include "esp_log.h"

#include "movimentacao.h"
#include "m_driver.h"
#include "odometria.h"
#include "encoder.h"
#include "infrared.h"
#include "robot_state.h"

static const char *TAG = "IR_CONTROL";

// Substituímos a Queue por um Handle direto para a Task
static TaskHandle_t ir_task_handle = NULL;

// Mutex global para proteger o acesso aos motores.
// usar "extern SemaphoreHandle_t motor_mutex;" no seu arquivo teste_navegacao.c
SemaphoreHandle_t motor_mutex = NULL;

typedef struct {
motor_t *mR;
motor_t *mL;
encoder_t *eR;
encoder_t *eL;
pose_t *p;
} contexto_t;

static contexto_t contexto;

// TASK SINALIZA INTERRUPCAO DOS SENSORES
static void IRAM_ATTR ir_isr(void *arg){
    uint32_t sensor = (uint32_t)arg; // uint32_t para passar na notificação

    BaseType_t high_task_wakeup = pdFALSE;

    // Evita novas interrupções deste sensor enquanto ele é processado
    gpio_intr_disable((gpio_num_t)sensor);

    // Envia o pino do sensor diretamente para a Task, substituindo o valor anterior
    xTaskNotifyFromISR(ir_task_handle, sensor, eSetValueWithOverwrite, &high_task_wakeup);

    if(high_task_wakeup){
        portYIELD_FROM_ISR();
    }
}

static void sensor_task(void *arg){

    uint32_t sensor_val;
    contexto_t *ctx = (contexto_t *)arg;

    while (1)
    {
        // Espera bloqueada (sem gastar CPU) até a ISR enviar a notificação
        if (xTaskNotifyWait(0x00, ULONG_MAX, &sensor_val, portMAX_DELAY) == pdTRUE)
        {
            gpio_num_t sensor = (gpio_num_t)sensor_val;

            motion_abort = true; // Avisa a main_task para sair do loop

            // TENTA PEGAR O CONTROLE DOS MOTORES (Timeout de 100ms)
            // Impede que a main ou outra task use o motor enquanto o robô evade
            if (xSemaphoreTake(motor_mutex, portMAX_DELAY) == pdTRUE) 
            {
                
                vTaskDelay(pdMS_TO_TICKS(50));

                switch(sensor)
                {
                    case IR_FR:

                        mouse_coast(ctx->mR, ctx->mL);
                        vTaskDelay(pdMS_TO_TICKS(300));

                        //frente a uma parede e detectou passagem para a direita
                        if(gpio_get_level(IR_FL)==0 && gpio_get_level(IR_R)==1){

                            movimentacao_turn_clws(ctx->mR, ctx->mL, ctx->eR, ctx->eL, ctx->p);
                            robot_state_publicar(100.0f, ctx->p->vm, "Curva 90 horaria (sensor IR)");
                        
                        //frente a uma parede e detectou passagem para a esquerda
                        } else if (gpio_get_level(IR_FL)==0 && gpio_get_level(IR_L)==1){

                            movimentacao_turn_ctclws(ctx->mR, ctx->mL, ctx->eR, ctx->eL, ctx->p);
                            robot_state_publicar(100.0f, ctx->p->vm, "Curva 90 anti-horaria (sensor IR)");
                        
                        //apenas desviando de obstaculo diagonal
                        } else {
                            if(gpio_get_level(IR_FR) == 0){
                                mouse_spin(ctx->mR, ctx->mL, 0);
                                vTaskDelay(pdMS_TO_TICKS(200));
                            }
                            mouse_break(ctx->mR, ctx->mL);

                            //como esse movimento foi feito utilizando as funcoes 'brutas'
                            //e necessario limpar os encoders para evitar atrapalhar o controle.
                            encoder_clean(ctx->eR);
                            encoder_clean(ctx->eL);
                            
                        }
                        break;

                    case IR_FL:

                        mouse_coast(ctx->mR, ctx->mL);
                        vTaskDelay(pdMS_TO_TICKS(300));

                        //frente a uma parede e detectou passagem para a direita
                        if(gpio_get_level(IR_FR)==0 && gpio_get_level(IR_R)==1){
                            
                            movimentacao_turn_clws(ctx->mR, ctx->mL, ctx->eR, ctx->eL, ctx->p);
                            robot_state_publicar(100.0f, ctx->p->vm, "Curva 90 horaria (sensor IR)");
                        
                        //frente a uma parede e detectou passagem para a esquerda
                        } else if (gpio_get_level(IR_FR)==0 && gpio_get_level(IR_L)==1){
                            
                            movimentacao_turn_ctclws(ctx->mR, ctx->mL, ctx->eR, ctx->eL, ctx->p);
                            robot_state_publicar(100.0f, ctx->p->vm, "Curva 90 anti-horaria (sensor IR)");
                        
                        //apenas desviando de obstaculo diagonal
                        } else {
                            if(gpio_get_level(IR_FL) == 0){
                                mouse_spin(ctx->mR, ctx->mL, 1);
                                vTaskDelay(pdMS_TO_TICKS(200));
                            }
                            mouse_break(ctx->mR, ctx->mL);

                            //como esse movimento foi feito utilizando as funcoes 'brutas'
                            //e necessario limpar os encoders para evitar atrapalhar o controle.
                            encoder_clean(ctx->eR);
                            encoder_clean(ctx->eL);
                        
                        }
                        break;
                    
                    default:
                        break;
                }
                
                // DEVOLVE O CONTROLE DOS MOTORES PARA A MAIN
                xSemaphoreGive(motor_mutex);
            } else {
                ESP_LOGW(TAG, "Falha ao obter controle dos motores para evasão!");
            }

            // Aguarda estabilizar o sinal e o robô fisicamente
            vTaskDelay(pdMS_TO_TICKS(30));

            // Reabilita a interrupção para este sensor
            gpio_intr_enable(sensor);
        }
    }
}

bool infrared_recuperar_canto(motor_t *mR, motor_t *mL, encoder_t *eR, encoder_t *eL, pose_t *p)
{
    const bool frente_dir = gpio_get_level(IR_FR) == 0;
    const bool frente_esq = gpio_get_level(IR_FL) == 0;
    const bool abertura_dir = gpio_get_level(IR_R) == 1;
    const bool abertura_esq = gpio_get_level(IR_L) == 1;

    if (!frente_dir && !frente_esq) {
        return false;
    }

    if (abertura_dir) {
        movimentacao_turn_clws(mR, mL, eR, eL, p);
        robot_state_publicar(100.0f, p->vm, "Curva de recuperacao — direita (sensor IR)");
        return true;
    }

    if (abertura_esq) {
        movimentacao_turn_ctclws(mR, mL, eR, eL, p);
        robot_state_publicar(100.0f, p->vm, "Curva de recuperacao — esquerda (sensor IR)");
        return true;
    }

    return false;
}

bool infrared_recuperar_travamento(motor_t *mR, motor_t *mL, encoder_t *eR, encoder_t *eL, pose_t *p)
{
    movimentacao_re_curta(mR, mL, eR, eL);
    vTaskDelay(pdMS_TO_TICKS(350));

    const bool frente_dir = gpio_get_level(IR_FR) == 0;
    const bool frente_esq = gpio_get_level(IR_FL) == 0;
    const bool abertura_dir = gpio_get_level(IR_R) == 1;
    const bool abertura_esq = gpio_get_level(IR_L) == 1;

    if (abertura_dir) {
        movimentacao_turn_clws(mR, mL, eR, eL, p);
        robot_state_publicar(100.0f, p->vm, "Curva de recuperacao — direita (sensor IR)");
        vTaskDelay(pdMS_TO_TICKS(400));
        return true;
    }

    if (abertura_esq) {
        movimentacao_turn_ctclws(mR, mL, eR, eL, p);
        robot_state_publicar(100.0f, p->vm, "Curva de recuperacao — esquerda (sensor IR)");
        vTaskDelay(pdMS_TO_TICKS(400));
        return true;
    }

    if (frente_dir && !frente_esq) {
        movimentacao_turn_ctclws(mR, mL, eR, eL, p);
        robot_state_publicar(100.0f, p->vm, "Curva de recuperacao — esquerda (obstaculo a direita)");
    } else if (frente_esq && !frente_dir) {
        movimentacao_turn_clws(mR, mL, eR, eL, p);
        robot_state_publicar(100.0f, p->vm, "Curva de recuperacao — direita (obstaculo a esquerda)");
    } else {
        movimentacao_turn_clws(mR, mL, eR, eL, p);
        robot_state_publicar(100.0f, p->vm, "Curva de recuperacao — direita (padrao)");
    }

    vTaskDelay(pdMS_TO_TICKS(400));
    return true;
}

void IR_init(motor_t *mR, motor_t *mL, encoder_t *eR, encoder_t *eL, pose_t *p){
    // Cria o Mutex que protegerá os motores de acessos cruzados
    if (motor_mutex == NULL) {
    motor_mutex = xSemaphoreCreateMutex();
    }

    gpio_config_t io_conf_front = {
        .pin_bit_mask =
        (1ULL << IR_FR) |
        (1ULL << IR_FL),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    gpio_config_t io_conf_side = {
        .pin_bit_mask =
        (1ULL << IR_R) |
        (1ULL << IR_L),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    contexto.mR = mR;
    contexto.mL = mL;
    contexto.eR = eR;
    contexto.eL = eL;
    contexto.p  = p;

    gpio_config(&io_conf_front);
    gpio_config(&io_conf_side);

    xTaskCreate(
        sensor_task,
        "ir_sensor",
        4096,
        &contexto,
        8, // Prioridade alta, evasão crítica
        &ir_task_handle
    );    

    // gpio_isr_handler_add(
    //     IR_L,
    //     ir_isr,
    //     (void *)IR_L
    // );

    gpio_isr_handler_add(
        IR_FL,
        ir_isr,
        (void *)IR_FL
    );

    // gpio_isr_handler_add(
    //     IR_R,
    //     ir_isr,
    //     (void *)IR_R
    // );

    gpio_isr_handler_add(
        IR_FR,
        ir_isr,
        (void *)IR_FR
    );
}