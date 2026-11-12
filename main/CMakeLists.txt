idf_component_register(
    SRCS "main_working.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif esp_event nvs_flash mqtt
)
