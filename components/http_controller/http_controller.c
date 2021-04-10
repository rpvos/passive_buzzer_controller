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

/*
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
*/

#define SERVER_PORT 3500
#define HTTP_METHOD HTTP_GET
#define URI_STRING "/"

#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "wifi softAP";

static httpd_handle_t httpServerInstance = NULL;
 
static esp_err_t methodHandler(httpd_req_t* httpRequest){
    ESP_LOGI("HANDLER","This is the handler for the <%s> URI", httpRequest->uri);
    return ESP_OK;
}
 
static httpd_uri_t testUri = {
    .uri       = URI_STRING,
    .method    = HTTP_METHOD,
    .handler   = methodHandler,
    .user_ctx  = NULL,
};
 
static void startHttpServer(void){
    httpd_config_t httpServerConfiguration = HTTPD_DEFAULT_CONFIG();
    httpServerConfiguration.server_port = SERVER_PORT;
    if(httpd_start(&httpServerInstance, &httpServerConfiguration) == ESP_OK){
        ESP_ERROR_CHECK(httpd_register_uri_handler(httpServerInstance, &testUri));
    }
}
 
static void stopHttpServer(void){
    if(httpServerInstance != NULL){
        ESP_ERROR_CHECK(httpd_stop(httpServerInstance));
    }
}
 
static esp_err_t wifiEventHandler(void* userParameter, system_event_t *event) {
    switch(event->event_id){
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
 
void http_controller_init(){
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

/*
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    
}

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));    
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);
}

void http_controller_init(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_softap();
}
*/
