#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <dht.h>
#include "driver/gpio.h"
#include "esp_err.h"

#define SENSOR_TYPE DHT_TYPE_DHT11
#define DHT_PIN GPIO_NUM_27

void dht_test(void* pvParameters) {
    float temperature,humidity;

    gpio_set_pull_mode(DHT_PIN, GPIO_PULLUP_ONLY);

    while (1) {
	if (dht_read_float_data(SENSOR_TYPE, DHT_PIN, &humidity, &temperature) == ESP_OK)
	    printf("Humidity: %.2f%% Temperature: %.2fC\n", humidity, temperature);
	else
	    printf("Não consigo ler o dado do sensor\n");

	vTaskDelay(pdMS_TO_TICKS(2000)); // Se for muito pouco, o dht superaquece
    }
}

void app_main() {
    xTaskCreate(dht_test, // função 
	    "dht_test",   // nome 
	    configMINIMAL_STACK_SIZE * 3, // stack 
	    NULL, // parametros
	    5, // prioridade
	    NULL); // handle
}
