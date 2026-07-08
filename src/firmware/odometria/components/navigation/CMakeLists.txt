idf_component_register(SRCS "navigation.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                        infrared
                        m_driver
                        encoder
                        odometria
                        movimentacao
                    )
