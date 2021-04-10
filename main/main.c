
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <buzzer_controller.h>
#include <http_controller.h>

void app_main(void)
{
    http_controller_init();
    buzzer_init();

    while(1) {
        vTaskDelay(500/portTICK_RATE_MS);
    }
}
