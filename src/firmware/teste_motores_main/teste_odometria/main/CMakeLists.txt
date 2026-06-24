idf_component_register(SRCS "teste_odometria.c"
                    INCLUDE_DIRS "."
                    REQUIRES   
                        odometria
                        movimentacao
                        m_driver
                        encoder
                        seletor_modo
                        calibracao
                    )
