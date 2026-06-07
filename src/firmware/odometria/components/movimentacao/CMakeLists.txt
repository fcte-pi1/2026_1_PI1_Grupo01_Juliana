idf_component_register(SRCS "movimentacao.c"
                    INCLUDE_DIRS "."
                    REQUIRES encoder
                    	     m_driver
                    	     odometria)
