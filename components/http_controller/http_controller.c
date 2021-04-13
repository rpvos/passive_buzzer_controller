#include "http_controller.h"

#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_netif.h"

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
static esp_err_t frequencyPageHandler(httpd_req_t *httpRequest)
{
    ESP_LOGI(TAG, "Frequency setter webpage called");

    // Var where the extra headers are stored
    const char *uri = httpRequest->uri;

    // Variables how long the webpage components are
    int uri_length = strlen(URI_STRING);
    int base_header_length = strlen("?frequency=");
    int total_length = uri_length + base_header_length;

    // Header contains info on new frequency level
    if (strlen(uri) > total_length)
    {
        // Calculate how many digits are there to read
        int digit_length = strlen(uri) - total_length;
        // Initiate the array to store them with space for a null byte
        char value[digit_length + 1];
        // Loop through all the digits
        for (int character = 0; character < digit_length; character++)
        {
            value[character] = uri[total_length + character];
        }

        // End the char array with \0 to make it a string
        value[digit_length] = '\0';

        // Log the new frequency
        ESP_LOGI(TAG, "New frequency: %s", value);

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
    while (counter != 0)
    {
        counter /= 10;
        count++;
    }

    // Initiate a buffer to put the html page in containing the frequency
    char buffer[strlen(html) + 1 + count];
    // Copy the html into the buffer with the %d swapped for the frequency
    sprintf(buffer, html, frequency);

    // Send the page to the requester
    httpd_resp_send(httpRequest, buffer, strlen(buffer));

    // Return ESP_OK
    return ESP_OK;
}

// Struct that contains the info about the getter for the webpage
static httpd_uri_t frequency_uri = {
    .uri = URI_STRING,
    .method = HTTP_GET,
    .handler = frequencyPageHandler,
    .user_ctx = NULL,
};

// Function to start the http server
static void start_http_server(void)
{
    // Default config for the http server
    httpd_config_t httpServerConfiguration = HTTPD_DEFAULT_CONFIG();
    // Use the assigned port from settings
    httpServerConfiguration.server_port = SERVER_PORT;

    // Start the server and check if it was completed
    if (httpd_start(&httpServerInstance, &httpServerConfiguration) == ESP_OK)
    {
        // Register the frequency page in the http server
        ESP_ERROR_CHECK(httpd_register_uri_handler(httpServerInstance, &frequency_uri));
    }
}

// Function to stop the http server
static void stop_http_server(void)
{
    // If the server is existend
    if (httpServerInstance != NULL)
    {
        // Stop the server
        ESP_ERROR_CHECK(httpd_stop(httpServerInstance));
    }
}

// If a client joins we start the http server and if it leaves we stop the server
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    // In the event of a client joining
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        // Get the event data and display it in the log
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Mac address: " MACSTR " joined, AID=%d", MAC2STR(event->mac), event->aid);

        // Start the http server
        start_http_server();
    }
    // In the event of a client disconnecting
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        // Get the event data and display it in the log
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Mac address: " MACSTR " left, AID=%d", MAC2STR(event->mac), event->aid);

        // Stop the http server
        stop_http_server();
    }
}

// Initializer method for the http server
void http_controller_init()
{
    // Make sure nvs is initialized
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the network stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create a loop for handling connection request
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Default wifi acces point
    esp_netif_create_default_wifi_ap();

    // Default wifi config
    wifi_init_config_t wifiConfiguration = WIFI_INIT_CONFIG_DEFAULT();

    // Set the wifi config
    ESP_ERROR_CHECK(esp_wifi_init(&wifiConfiguration));

    // Regester the event handler for joining and leaving clients
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Create the wifi config using the settings
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = CONFIG_ESP_MAX_STA_CONN,
            .beacon_interval = 150,
        },
    };

    // If the password is empty the acces point is open
    if (strlen(ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // Set the mode to acces point
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    // Set the config
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    // Start the wifi acces point
    ESP_ERROR_CHECK(esp_wifi_start());

    // Log that the acces point is online
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
}
