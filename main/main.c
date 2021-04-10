
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <buzzer_controller.h>

void app_main(void)
{
    buzzer_init();
    vTaskDelay(2000/portTICK_RATE_MS);
    set_frequency(1000);

    while(1) {
        vTaskDelay(10);
    }
}