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

#define SERVER_PORT 3500
#define URI_STRING "/"

#define ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "http controller";
int frequency = 1000; // base hertz

static httpd_handle_t httpServerInstance = NULL;

int get_http_frequency(){
    if (frequency>1000)
    {
        return frequency;
    }else{
        return 1000;
    }
}

void set_http_frequency(int value){
    frequency = value;
}

static esp_err_t getHandler(httpd_req_t *httpRequest)
{
    ESP_LOGI(TAG, "Get handler called");

    char * uri = httpRequest->uri;
    // Header contains info on new frequency level
    if (strlen(uri)>strlen("/?frequency="))
    {
        int pre_digit_length = strlen("/?frequency=");
        int digit_length = strlen(uri)-pre_digit_length;
        char value[digit_length+1];
        for (int character = 0; character < digit_length; character++)
        {
            value[character] = uri[pre_digit_length+character];
        }
        value[digit_length] = '\0';
        frequency = atoi(value);
    }
    

    char html[] = "<html><body> <form action=\"\" method=\"get\"> <input type=\"number\" id=\"frequency\" name=\"frequency\" value=\"%d\" /> <input type=\"submit\" value=\"Send\" /> </form></body></html>\r\n\r\n";
    char buffer[strlen(html)+1];
    sprintf(buffer,html,frequency);  

    httpd_resp_send(httpRequest, buffer, strlen(buffer));

    return ESP_OK;
}


static httpd_uri_t getUri = {
    .uri = URI_STRING,
    .method = HTTP_GET,
    .handler = getHandler,
    .user_ctx = NULL,
};

static void startHttpServer(void)
{
    httpd_config_t httpServerConfiguration = HTTPD_DEFAULT_CONFIG();
    httpServerConfiguration.server_port = SERVER_PORT;
    if (httpd_start(&httpServerInstance, &httpServerConfiguration) == ESP_OK)
    {
        ESP_ERROR_CHECK(httpd_register_uri_handler(httpServerInstance, &getUri));
    }
}

static void stopHttpServer(void)
{
    if (httpServerInstance != NULL)
    {
        ESP_ERROR_CHECK(httpd_stop(httpServerInstance));
    }
}

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
