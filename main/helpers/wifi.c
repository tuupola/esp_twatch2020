/*

SPDX-License-Identifier: MIT-0

MIT No Attribution

Copyright (c) 2020 Mika Tuupola

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <nvs_flash.h>
#include <stdint.h>

#include "helpers/wifi.h"

static const char *TAG = "wifi_helper";
static EventGroupHandle_t wifi_event_group;
static esp_event_handler_instance_t wifi_handler, ip_handler;

//static const uint8_t STA_CONNECTED = BIT0;
static const uint8_t GOT_IP_BIT = BIT1;
static const uint8_t GOT_IP6_BIT = BIT2;

static void wifi_event_handler(
    void *arg, esp_event_base_t base, int32_t id, void *event_data
) {
    switch (id) {
    case WIFI_EVENT_WIFI_READY:
        ESP_LOGI(TAG, "Got WIFI_EVENT_WIFI_READY event");
        break;
    case WIFI_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, "Got WIFI_EVENT_SCAN_DONE event");
        break;
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "Got WIFI_EVENT_STA_START event");
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_STOP:
        ESP_LOGI(TAG, "Got WIFI_EVENT_STA_STOP event");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "Got WIFI_EVENT_STA_CONNECTED event");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Got WIFI_EVENT_STA_DISCONNECTED event");
        vTaskDelay(1000 / portTICK_RATE_MS);
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_AUTHMODE_CHANGE:
        ESP_LOGI(TAG, "Got WIFI_EVENT_STA_AUTHMODE_CHANGE event");
        break;
    default:
        ESP_LOGI(TAG, "Got unknownn event?");
        break;
    }
}

static void ip_event_handler(
    void *arg, esp_event_base_t base, int32_t id, void *event_data
) {
    switch (id) {
    case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Got IP_EVENT_STA_GOT_IP event");
        xEventGroupSetBits(wifi_event_group, GOT_IP_BIT);
        break;
    case IP_EVENT_STA_LOST_IP:
        ESP_LOGI(TAG, "Got IP_EVENT_STA_LOST_IP event");
        break;
    case IP_EVENT_AP_STAIPASSIGNED:
        ESP_LOGI(TAG, "Got IP_EVENT_AP_STAIPASSIGNED event");
        break;
    case IP_EVENT_GOT_IP6:
        ESP_LOGI(TAG, "Got IP_EVENT_GOT_IP6 event");
        xEventGroupSetBits(wifi_event_group, GOT_IP6_BIT);
        break;
    case IP_EVENT_ETH_GOT_IP:
        ESP_LOGI(TAG, "Got IP_EVENT_ETH_GOT_IP event");
        break;
    default:
        ESP_LOGI(TAG, "Got unknownn event?");
        break;
    }
}

esp_err_t wifi_init() {

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    //EventBits_t bits;

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifi_handler
    ));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, &ip_handler
    ));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Wait until we get an ip address. */
    // bits = xEventGroupWaitBits(
    xEventGroupWaitBits(
        wifi_event_group,
        GOT_IP_BIT | GOT_IP6_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    return ESP_OK;
}

esp_err_t wifi_close() {
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler);
    esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ip_handler);
    vEventGroupDelete(wifi_event_group);
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();

    return ESP_OK;
}
