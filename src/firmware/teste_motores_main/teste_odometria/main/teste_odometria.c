#include <stdio.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "m_driver.h"
#include "encoder.h"
#include "odometria.h"
#include "movimentacao.h"

static const char *TAG = "teste_odometria";

bool busy = false;

#define BOTAO_SELETOR   GPIO_NUM_25
#define BUZZER_GPIO     GPIO_NUM_33

#define A3_FREQ         440
#define E4_FREQ         659
#define A4_FREQ         880

#define TEMPO           300

encoder_t encoderR;
encoder_t encoderL;

pose_t pose;

motor_t motorR = { .pwm_gpio1 = PWM_R1, .pwm_gpio2 = PWM_R2};
motor_t motorL = { .pwm_gpio1 = PWM_L1, .pwm_gpio2 = PWM_L2};

TaskHandle_t mission_task_handle;

void buzzer_init(){

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_channel_config_t channel = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 512,
        .hpoint = 0
    };

    ledc_timer_config(&timer);
    ledc_channel_config(&channel);

    //iniciando desligado
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void play_tone(uint32_t freq, uint32_t duracao_ms){

    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    vTaskDelay(pdMS_TO_TICKS(duracao_ms));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

//missao principal realizada no teste:
// avancar 1 bloco -> virar 90o horario -> virar 90o antihorario
void start_mission(){

    ESP_LOGI(TAG, "iniciando missao...");

    play_tone(A4_FREQ, TEMPO);

    //step 1
   movimentacao_move_cell(&motorR, &motorL, &encoderR, &encoderL, &pose);

   play_tone(A4_FREQ, TEMPO);

   vTaskDelay(pdMS_TO_TICKS(1000));
    
    encoder_clean(&encoderR);
    encoder_clean(&encoderL);

   //step 2
    movimentacao_turn_clws(&motorR, &motorL, &encoderR, &encoderL, &pose);

    vTaskDelay(pdMS_TO_TICKS(1000));

    encoder_clean(&encoderR);
    encoder_clean(&encoderL);

    //step 3
    movimentacao_turn_ctclws(&motorR, &motorL, &encoderR, &encoderL, &pose);

    vTaskDelay(pdMS_TO_TICKS(1000));

    encoder_clean(&encoderR);
    encoder_clean(&encoderL);

    ESP_LOGI(TAG, "MISSAO CUMPRIDA COM EXITO! =D");

    play_tone(E4_FREQ, TEMPO);
    
    busy = false;
}

//INTERRUPCAO SINALIZA TASK
static void IRAM_ATTR button_isr_handler(void *arg)
{

    if (busy){
        return;
    }

    busy = true;

    BaseType_t high_task_wakeup = pdFALSE;

    vTaskNotifyGiveFromISR(
        mission_task_handle,
        &high_task_wakeup
    );

    if (high_task_wakeup) {
        portYIELD_FROM_ISR();
    }
}

//TASK REALIZADA AO RECEBER A INTERRUPCAO
void mission_task(void *arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "botao pressionado");

        play_tone(A3_FREQ, TEMPO);
        play_tone(E4_FREQ, TEMPO);
        play_tone(A4_FREQ, TEMPO);

        start_mission();
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOTAO_SELETOR),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    gpio_config(&io_conf);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(
        BOTAO_SELETOR,
        button_isr_handler,
        NULL
    );

    buzzer_init();

    play_tone(A4_FREQ, TEMPO);

	encoder_init(&encoderR, GPIO_ENC_R);
	encoder_init(&encoderL, GPIO_ENC_L);

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

   odometria_pos_init(&pose, 0, 0, NORTE);

   //criacao da task sinalizada por interrupcao
   xTaskCreate(
    mission_task,
    "mission_task",
    4096,
    NULL,
    5,
    &mission_task_handle
    );

    play_tone(A3_FREQ, TEMPO);

    vTaskDelay(pdMS_TO_TICKS(10));
}

