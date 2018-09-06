#include "wifi_connection.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "freertos/event_groups.h"

#define WIFI_TAG "WIFI_CONNECTION"
#define EVENT_TAG "EVENT_HANDLER"

EventGroupHandle_t wifi_event_group;
extern const int CONNECTED_BIT;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    esp_err_t status;
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            // connect to the AP, can fail if initialized incorrectly
            status = esp_wifi_connect();
            ESP_ERROR_CHECK(status);
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            // got an IP address from the DHCP server, set the connected flag to 1
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            ESP_LOGI(EVENT_TAG, "Got IP: %s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            // if we disconnect, connect once again and set the connected flag to 0
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            ESP_LOGI(EVENT_TAG, "got event %d", event->event_id);
            break;
    }
    return ESP_OK;
}

EventGroupHandle_t initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    esp_err_t status;

    // creates a task that listens for wifi events
    status = esp_event_loop_init(event_handler, NULL);
    ESP_ERROR_CHECK(status);
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    status = esp_wifi_init(&cfg);
    ESP_ERROR_CHECK(status);

    status = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ESP_ERROR_CHECK(status);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_LOGI(WIFI_TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    
    // set wifi mode to soft ap (client)
    status = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(status);

    // set the AP ssid and password
    status = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    ESP_ERROR_CHECK(status);

    status = esp_wifi_start();
    ESP_ERROR_CHECK(status);
    return wifi_event_group;
}