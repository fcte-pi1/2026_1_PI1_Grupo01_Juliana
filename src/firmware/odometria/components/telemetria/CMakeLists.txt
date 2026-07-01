idf_component_register(SRCS "telemetria.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        esp_wifi
                        esp_event
                        esp_netif
                        nvs_flash
                        esp_websocket_client
                        json
                        lwip
                    )
