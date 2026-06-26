idf_component_register(SRCS "teste_sens_potencia.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        odometria
                        movimentacao
                        power_module
                        m_driver
                        encoder
                        esp_driver_ledc)
