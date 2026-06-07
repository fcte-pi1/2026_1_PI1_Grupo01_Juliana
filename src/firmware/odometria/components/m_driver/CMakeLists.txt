idf_component_register(
    SRCS "m_driver.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_mcpwm
             esp_driver_gpio
)