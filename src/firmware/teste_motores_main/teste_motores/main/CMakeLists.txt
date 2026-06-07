idf_component_register(SRCS "teste_motores.c"
                    INCLUDE_DIRS "."
                    REQUIRES m_driver
                    	     odometria
                    	     encoder
                    	     movimentacao)
