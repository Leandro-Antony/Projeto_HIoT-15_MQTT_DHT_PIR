idf_component_register(
    SRCS "mqtt_communication.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif esp_event nvs_flash mqtt
)
