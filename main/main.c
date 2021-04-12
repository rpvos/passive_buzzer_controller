/*
 * Project name:
     Passive buzzer controller
 * Author: Rik Vos
 * Description:
     This component includes a way to make music using an passive buzzer
 * Test configuration:
     Board: ESP32
     Ext: pin25 -> passive buzzer -> ground
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <buzzer_controller.h>
#include <http_controller.h>
#include "esp_log.h"

// Callback to set the frequency of the buzzer by the http server
void cb(int frequency){
    set_frequency(frequency);
}

// Starting point of the project
void app_main(void)
{
    // Must initialize the http controller first otherwise the esp will throw an error
    http_controller_init();
    // Initialize the buzzer
    buzzer_init();

    // Set the callback with which we will edit the frequency
    set_frequency_callback(&cb);

    // Endless loop
    while(1) {
        vTaskDelay(50/portTICK_RATE_MS);
    }
}
