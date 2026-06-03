idf_component_register(
    SRCS "encoder.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_pcnt
             esp_driver_gpio
)
