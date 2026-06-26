idf_component_register(SRCS "teste_navegacao.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        odometria
                        movimentacao
                        power_module
                        m_driver
                        encoder
                        infrared
                        esp_driver_ledc
                        # telemetry_data_nvs
                        )
