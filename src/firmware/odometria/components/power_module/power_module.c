#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"

#include "power_module.h"

static const char *TAG = "potencia e i2c";

void scan_devices(i2c_master_bus_handle_t bus)
{
    uint8_t dummy = 0;

    for (uint8_t addr = 1; addr < 127; addr++)
    {
        i2c_device_config_t cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = 100000,
        };

        i2c_master_dev_handle_t dev;

        if (i2c_master_bus_add_device(bus, &cfg, &dev) == ESP_OK)
        {
            esp_err_t err =
                i2c_master_transmit(
                    dev,
                    &dummy,
                    1,
                    20);

            if (err == ESP_OK)
            {
                ESP_LOGI(TAG,
                         "Encontrado: 0x%02X",
                         addr);
            }

            i2c_master_bus_rm_device(dev);
        }
    }
}

static esp_err_t write_reg(
    ina226_t *ina,
    uint8_t reg,
    uint16_t value)
{
    uint8_t tx[3];

    tx[0] = reg;
    tx[1] = value >> 8;
    tx[2] = value & 0xFF;

    return i2c_master_transmit(
        ina->dev,
        tx,
        sizeof(tx),
        100);
}

static esp_err_t read_reg(
    ina226_t *ina,
    uint8_t reg,
    uint16_t *value)
{
    uint8_t rx[2];

    ESP_ERROR_CHECK(
        i2c_master_transmit_receive(
            ina->dev,
            &reg,
            1,
            rx,
            2,
            100));

    *value = (rx[0] << 8) | rx[1];

    return ESP_OK;
}

esp_err_t ina226_init(
    ina226_t *ina,
    i2c_master_bus_handle_t bus,
    uint8_t address,
    float shunt_resistance,
    float max_current)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };

    esp_err_t err =
        i2c_master_bus_add_device(
            bus,
            &dev_cfg,
            &ina->dev);

    if (err != ESP_OK)
    {
        return err;
    }

    ina->shunt_resistance = shunt_resistance;

    ina->current_lsb =
        max_current / 32768.0f;

    uint16_t calibration =
        (uint16_t)(
            0.00512f /
            (ina->current_lsb * shunt_resistance));

    err = write_reg(
        ina,
        INA226_REG_CALIBRATION,
        calibration);

    if (err != ESP_OK)
    {
        return err;
    }

    uint16_t config = 0x4127;

    err = write_reg(
        ina,
        INA226_REG_CONFIG,
        config);

    if (err != ESP_OK)
    {
        return err;
    }

    return ESP_OK;
}

esp_err_t ina226_get_bus_voltage(
    ina226_t *ina,
    float *voltage)
{
    uint16_t raw;

    esp_err_t err =
        read_reg(
            ina,
            INA226_REG_BUS_VOLT,
            &raw);

    if (err != ESP_OK)
    {
        return err;
    }

    *voltage = raw * 1.25e-3f;

    return ESP_OK;
}

esp_err_t ina226_get_current(
    ina226_t *ina,
    float *current)
{
    uint16_t raw;

    esp_err_t err =
        read_reg(
            ina,
            INA226_REG_CURRENT,
            &raw);

    if (err != ESP_OK)
    {
        return err;
    }

    *current =
        ((int16_t)raw) *
        ina->current_lsb;

    return ESP_OK;
}

esp_err_t ina226_get_power(
    ina226_t *ina,
    float *power)
{
    uint16_t raw;

    esp_err_t err =
        read_reg(
            ina,
            INA226_REG_POWER,
            &raw);

    if (err != ESP_OK)
    {
        return err;
    }

    *power =
        raw *
        (25.0f * ina->current_lsb);

    return ESP_OK;
}


