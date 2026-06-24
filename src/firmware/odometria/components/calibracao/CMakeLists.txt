idf_component_register(SRCS "calibracao.c"
                    INCLUDE_DIRS "."
                    REQUIRES encoder
                             m_driver
                             esp_timer)
