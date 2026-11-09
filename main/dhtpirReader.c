#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "dht.h"

#define PIN_PIR GPIO_NUM_26
#define PIN_DHT GPIO_NUM_27

void app_main(void) {

    float temperatura, umidade;

    gpio_reset_pin(PIN_PIR);
    gpio_set_direction(PIN_PIR, GPIO_MODE_INPUT);

    gpio_set_pull_mode(PIN_DHT, GPIO_PULLUP_ONLY);

    while (1) {

        int presenca = gpio_get_level(PIN_PIR);

        esp_err_t resultado = dht_read_float_data(
            DHT_TYPE_DHT11,
            PIN_DHT,
            &umidade,
            &temperatura
        );

        printf("Presenca: %d\n", presenca);

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
