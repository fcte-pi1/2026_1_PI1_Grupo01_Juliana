idf_component_register(SRCS "odometria.c"
                    INCLUDE_DIRS "."
                    REQUIRES encoder
                    	     m_driver
                    	     esp_driver_gpio
                             esp_timer)
