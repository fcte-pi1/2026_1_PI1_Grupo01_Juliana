//codigo feito para testar integração odometria+sensores

#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#include "m_driver.h"
#include "encoder.h"
#include "odometria.h"
#include "movimentacao.h"
#include "power_module.h"
#include "infrared.h"

#include "telemetry_data_nvs.h"

static const char *TAG = "teste_navegacao";

volatile bool busy = false;

#define BOTAO_SELETOR   GPIO_NUM_25
#define BUZZER_GPIO     GPIO_NUM_33

#define A3_FREQ         440
#define E4_FREQ         659
#define A4_FREQ         880

#define TEMPO           300

// TIPO DE LABIRINTO
typedef enum{
    ID4X4 = 0,
    ID8X8 = 1
} lab_id_t;

lab_id_t id_teste = ID4X4;

pose_t pose;

encoder_t encoderR;
encoder_t encoderL;

ina226_t ina;

static i2c_master_bus_handle_t bus_handle;

motor_t motorR = { .pwm_gpio1 = PWM_R1, .pwm_gpio2 = PWM_R2};
motor_t motorL = { .pwm_gpio1 = PWM_L1, .pwm_gpio2 = PWM_L2};

TaskHandle_t mission_task_handle;

//INTERRUPCAO SINALIZA TASK DO BOTAO
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

void telemetry_task(void *arg)
{
    float voltage_v = 0.0;
    float current_a = 0.0;

    while (1)
    {
        // 1. Lemos os dados reais do sensor passando o endereço das variáveis (&)
        ina226_get_bus_voltage(&ina, &voltage_v);
        ina226_get_current(&ina, &current_a);

        // 2. Convertemos os floats (Volts/Amperes) para inteiros (Milivolts/Miliamperes)
        // O "cast" (uint16_t) garante que o ESP32 descarte as casas decimais corretamente
        uint16_t voltage_mv = (uint16_t)(voltage_v * 1000.0f);
        int16_t current_ma = (int16_t)(current_a * 1000.0f);

        // 3. Salvamos na memória NVS usando a função do Canvas
        telemetry_save_sample(voltage_mv, current_ma);

        // 4. Aguardamos 500ms antes de ler novamente (2 amostras por segundo)
        // Isso evita encher a memória muito rápido
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

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

void set_maze_id(uint8_t id){
    if(id == ID8X8){
        id = ID4X4;
        play_tone(E4_FREQ, TEMPO);

    } else {
        id = ID8X8;
        play_tone(A4_FREQ, TEMPO);
        
    }

    busy = false;
}

//TASK REALIZADA AO RECEBER A INTERRUPCAO DO BOTAO
void mission_task(void *arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "botao pressionado");

        set_maze_id(id_teste);

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

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    
    play_tone(A4_FREQ, TEMPO);

    encoder_init(&encoderR, GPIO_ENC_R);
    encoder_init(&encoderL, GPIO_ENC_L);

    play_tone(A3_FREQ, 50);

    vTaskDelay(pdMS_TO_TICKS(100));

    play_tone(A3_FREQ, 50);

    esp_err_t err = ina226_init(
        &ina, 
        bus_handle,
        INA_ADDRESS, 
        SHUNT, 
        MAX_CURRENT
    );
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "INA226 inicializado com sucesso!");
    } else {
        ESP_LOGE(TAG, "Falha ao inicializar INA226");
    }   

    play_tone(E4_FREQ, TEMPO);

    vTaskDelay(pdMS_TO_TICKS(1000));

    play_tone(E4_FREQ, 50);

    vTaskDelay(pdMS_TO_TICKS(100));

    play_tone(E4_FREQ, 50);

    ESP_ERROR_CHECK(telemetry_init());

    play_tone(A4_FREQ, TEMPO);

    driver_init();

    motor_init(&motorR);
    motor_init(&motorL);

    IR_init(&motorR, &motorL, &encoderR, &encoderL, &pose);

    odometria_pos_init(&pose, 0, 0, NORTE);

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

    //criacao da task sinalizada por interrupcao
    xTaskCreate(
    mission_task,
    "mission_task",
    4096,
    NULL,
    5,
    &mission_task_handle
    );

    // criacao da task que vai ficar lendo o INA226 e salvando em segundo plano
    xTaskCreate(
        telemetry_task,
        "telemetry_task",
        4096,
        NULL,
        2, // Prioridade baixa, para não atrapalhar os motores e sensores
        NULL
    );

    telemetry_print_all();

    ESP_ERROR_CHECK(telemetry_clear());

    while(1){
        vTaskDelay(pdMS_TO_TICKS(3000));

        play_tone(A3_FREQ, TEMPO);

        movimentacao_move_cell(&motorR, &motorL, &encoderR, &encoderL, &pose);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}