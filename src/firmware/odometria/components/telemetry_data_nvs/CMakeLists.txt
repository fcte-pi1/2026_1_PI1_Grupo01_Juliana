idf_component_register(SRCS "telemetry_data_nvs.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        nvs_flash
                        esp_timer)
