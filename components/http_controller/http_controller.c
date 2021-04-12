#include "http_controller.h"

#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

// Lowest hertz the buzzer can handle
#define BASE_HERTZ 1000

// Base http request link
// Can be set to /test to make the acces point ip:port/test
#define URI_STRING "/"

// Settings from the config file
#define ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define SERVER_PORT CONFIG_ESP_WIFI_PORT
#define ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

// Tag for logging
static const char *TAG = "http controller";

// Global varible to know the frequency on the webpage
int frequency = BASE_HERTZ;

// Callback method
void (*set_frequency_cb)(int);

// Instance of the http server
static httpd_handle_t httpServerInstance = NULL;

// Setter for the callback
void set_frequency_callback(void (*cb)(int))
{
    set_frequency_cb = cb;
}

// Handler made to handle the following link ip:port/uri_string
static esp_err_t getHandler(httpd_req_t *httpRequest)
{
    ESP_LOGI(TAG, "Frequency setter webpage called");

    // Var where the extra headers are stored
    char *uri = httpRequest->uri;

    // Variables how long the webpage components are
    int uri_length = strlen(URI_STRING);
    int base_header_length = strlen("?frequency=");

    // Header contains info on new frequency level
    if (strlen(uri) > uri_length + base_header_length)
    {
        // Calculate how many digits are there to read
        int digit_length = strlen(uri) - base_header_length;
        // Initiate the array to store them with space for a null byte
        char value[digit_length + 1];
        // Loop through all the digits
        for (int character = 0; character < digit_length; character++)
        {
            value[character] = uri[base_header_length + character];
        }

        // End the char array with \0 to make it a string
        value[digit_length] = '\0';

        // Use atoi to convert a string to a integer
        frequency = atoi(value);

        // If the frequency is smaller then the lowest posible hertz
        if (frequency < BASE_HERTZ)
        {
            // Set the frequency to the lowest posible hertz
            frequency = BASE_HERTZ;
        }

        // Use the callback to set the frequency
        set_frequency_cb(frequency);
    }

    // Respond to the request with a html page without the frequency
    char html[] = "<html><body> <form action=\"\" method=\"get\"> <input type=\"number\" id=\"frequency\" name=\"frequency\" value=\"%d\" /> <input type=\"submit\" value=\"Send\" /> </form></body></html>\r\n\r\n";
    
    // Count the amount of digits used in the html page, 
    // it is initiated at -2 because the %d is two characters that are already in this string
    int count = -2;
    int counter = frequency;
    while (counter != 0) {
        counter /= 10;
        count++;
    }

    // Initiate a buffer to put the html page in containing the frequency
    char buffer[strlen(html) + 1+ count];
    // Copy the html into the buffer with the %d swapped for the frequency
    sprintf(buffer, html, frequency);

    // Send the page to the requester
    httpd_resp_send(httpRequest, buffer, strlen(buffer));

    // Return ESP_OK
    return ESP_OK;
}

/// Below this all the code is from an example project

// Struct that contains the info about the webpage
static httpd_uri_t getUri = {
    .uri = URI_STRING,
    .method = HTTP_GET,
    .handler = getHandler,
    .user_ctx = NULL,
};

// Function to start the server
static void startHttpServer(void)
{
    httpd_config_t httpServerConfiguration = HTTPD_DEFAULT_CONFIG();
    httpServerConfiguration.server_port = SERVER_PORT;
    if (httpd_start(&httpServerInstance, &httpServerConfiguration) == ESP_OK)
    {
        ESP_ERROR_CHECK(httpd_register_uri_handler(httpServerInstance, &getUri));
    }
}

// Function to stop the server
static void stopHttpServer(void)
{
    if (httpServerInstance != NULL)
    {
        ESP_ERROR_CHECK(httpd_stop(httpServerInstance));
    }
}

// Make a switch if a client joins we start the server and if it leaves we stop the server
static esp_err_t wifiEventHandler(void *userParameter, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_AP_STACONNECTED:
        startHttpServer();
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        stopHttpServer();
        break;
    default:
        break;
    }
    return ESP_OK;
}

// Initisilizer method for the http server
void http_controller_init()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));
    wifi_init_config_t wifiConfiguration = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiConfiguration));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_config_t apConfiguration = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .ssid_len = 0,
            .channel = ESP_WIFI_CHANNEL,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = CONFIG_ESP_MAX_STA_CONN,
            .beacon_interval = 150,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apConfiguration));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_start());
}
