idf_component_register(
    SRCS "seletor_modo.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio
             esp_driver_ledc
             esp_timer
)
