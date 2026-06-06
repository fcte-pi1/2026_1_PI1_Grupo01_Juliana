idf_component_register(SRCS "teste_odometria.c"
                    INCLUDE_DIRS "."
                    REQUIRES   
                        odometria
                        movimentacao
                        m_driver
                        encoder
                        esp_timer
                        esp_driver_ledc
                    )
