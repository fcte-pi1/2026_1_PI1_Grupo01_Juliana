idf_component_register(SRCS "teste_floodfill.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        odometria
                        movimentacao
                        m_driver
                        encoder
                        # infrared
                        esp_driver_ledc
                        )
