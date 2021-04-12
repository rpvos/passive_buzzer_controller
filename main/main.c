
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <buzzer_controller.h>
#include <http_controller.h>
#include "esp_log.h"

void app_main(void)
{
    http_controller_init();
    buzzer_init();
    set_http_frequency(get_frequency());

    while(1) {
        vTaskDelay(50/portTICK_RATE_MS);
        if (get_http_frequency()!=get_frequency())
        {
            set_frequency(get_http_frequency());
        }
        
    }
}
