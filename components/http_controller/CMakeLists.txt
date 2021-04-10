idf_component_register(SRCS "http_controller.c"
                        INCLUDE_DIRS "include"
                        REQUIRES nvs_flash esp_http_server)
