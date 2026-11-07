#include <stdio.h> //standart c
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

#define PIN_PIR GPIO_NUM_26 // PIR conectado na GPIO26

void app_main(void) {
  gpio_reset_pin(PIN_PIR);
  gpio_set_direction(PIN_PIR, GPIO_MODE_INPUT);
  
  while(1) {
    int state = gpio_get_level(PIN_PIR);
    printf("state: %d\n", state);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}