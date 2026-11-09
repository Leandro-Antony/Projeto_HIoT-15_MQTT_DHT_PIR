
#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht.h"

#define PIN_PIR GPIO_NUM_26
#define PIN_DHT GPIO_NUM_27

void leitura_pir(void *parametros) {
    while (1) {
        int presenca = gpio_get_level(PIN_PIR);
        printf("Presenca: %d\n", presenca);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void leitura_dht(void *parametros) {
    float temperatura, umidade;
    
    while (1) {
        esp_err_t resultado = dht_read_float_data(
            DHT_TYPE_DHT11,
            PIN_DHT,
            &umidade,
            &temperatura
        );

        if (resultado == ESP_OK) {

            printf("Temperatura: %.2f C\n", temperatura);
            printf("Umidade: %.2f %%\n", umidade);

        } else {

            printf("Erro na leitura do DHT11!\n");
        }

        printf("------------------------\n");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {

    gpio_reset_pin(PIN_PIR);
    gpio_set_direction(PIN_PIR, GPIO_MODE_INPUT);

    gpio_set_pull_mode(PIN_DHT, GPIO_PULLUP_ONLY);
    
    xTaskCreate(
        leitura_pir,
        "leitura_pir",
        2048,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        leitura_dht,
        "leitura_dht",
        4096,
        NULL,
        5,
        NULL
    );
}
