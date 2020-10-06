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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/spi_master.h>
#include <mipi_dcs.h>
#include <mipi_display.h>
#include <hagl.h>
#include <hagl_hal.h>
#include <fps.h>
#include <rgb565.h>
#include <i2c_helper.h>
#include <axp202.h>
#include <pcf8563.h>

#include "font8x13.h"
#include "helpers/wifi.h"
#include "helpers/nvs.h"
#include "sdkconfig.h"

static const char *TAG = "main";
static float fb_fps;

pcf8563_t pcf;
axp202_t axp;
struct tm rtc = {0};
spi_device_handle_t spi;

/*

Cap refresh rate to 45fps.
T = 1000 / 45 / (1000 / CONFIG_FREERTOS_HZ)

*/
void backbuffer_task(void *params)
{
    TickType_t last;
    const TickType_t period = 1000 / 45 / portTICK_RATE_MS;

    last = xTaskGetTickCount();

    while (1) {
        hagl_flush();
        fb_fps = fps();
        // vTaskDelayUntil(&last, period);
    }

    vTaskDelete(NULL);
}

void rtc_task(void *params)
{
    uint16_t color = rgb565(0, 255, 0);
    char16_t message[128];

    /* Calculate tm_yday for the first run. */
    mktime(&rtc);

    while (1) {
        pcf8563_read(&pcf, &rtc);
        swprintf(
            message,
            sizeof(message),
            L"%04d-%02d-%02d",
            rtc.tm_year + 1900, rtc.tm_mon + 1, rtc.tm_mday
        );
        hagl_put_text(message, 80, 100, color, font8x13);
        // ESP_LOGI(TAG, "%04d-%02d-%02d", rtc.tm_year + 1900, rtc.tm_mon + 1, rtc.tm_mday);

        swprintf(
            message,
            sizeof(message),
            L"%02d:%02d:%02d",
            rtc.tm_hour, rtc.tm_min, rtc.tm_sec
        );
        hagl_put_text(message, 88, 115, color, font8x13);
        // ESP_LOGI(TAG, "%02d:%02d:%02d", rtc.tm_hour, rtc.tm_min, rtc.tm_sec);

        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void log_task(void *params)
{
//     float vacin, iacin, vvbus, ivbus, vts, temp, pbat, vbat, icharge, idischarge, vaps, cbat;
//     uint8_t power, charge;
//     char buffer[128];

    while (1) {
//         axp192_read(&axp, AXP192_ACIN_VOLTAGE, &vacin);
//         axp192_read(&axp, AXP192_ACIN_CURRENT, &iacin);
//         axp192_read(&axp, AXP192_VBUS_VOLTAGE, &vvbus);
//         axp192_read(&axp, AXP192_VBUS_CURRENT, &ivbus);
//         axp192_read(&axp, AXP192_TEMP, &temp);
//         axp192_read(&axp, AXP192_TS_INPUT, &vts);
//         axp192_read(&axp, AXP192_BATTERY_POWER, &pbat);
//         axp192_read(&axp, AXP192_BATTERY_VOLTAGE, &vbat);
//         axp192_read(&axp, AXP192_CHARGE_CURRENT, &icharge);
//         axp192_read(&axp, AXP192_DISCHARGE_CURRENT, &idischarge);
//         axp192_read(&axp, AXP192_APS_VOLTAGE, &vaps);
//         axp192_read(&axp, AXP192_COULOMB_COUNTER, &cbat);

//         ESP_LOGI(TAG,
//             "vacin: %.2fV iacin: %.2fA vvbus: %.2fV ivbus: %.2fA vts: %.2fV temp: %.0fC "
//             "pbat: %.2fmW vbat: %.2fV icharge: %.2fA idischarge: %.2fA, vaps: %.2fV "
//             "cbat: %.2fmAh",
//             vacin, iacin, vvbus, ivbus, vts, temp, pbat, vbat, icharge, idischarge, vaps, cbat
//         );

//         axp192_ioctl(&axp, AXP192_READ_POWER_STATUS, &power);
//         axp192_ioctl(&axp, AXP192_READ_CHARGE_STATUS, &charge);
//         ESP_LOGI(TAG,
//             "power: 0x%02x charge: 0x%02x",
//             power, charge
//         );

//         strftime(buffer, 128 ,"%c (day %j)" , &rtc);
//         ESP_LOGI(TAG, "RTC: %s", buffer);

        vTaskDelay(5000 / portTICK_RATE_MS);

        ESP_LOGI(TAG, "fps: %.1f", fb_fps);
    }
    vTaskDelete(NULL);
}

static void sntp_set_rtc(struct timeval *tv)
{
    struct tm *time;

    ESP_LOGI(TAG, "Got SNTP response, setting RTC.");

    time = localtime(&tv->tv_sec);
    pcf8563_write(&pcf, time);
}

void app_main()
{
    ESP_LOGI(TAG, "SDK version: %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Heap when starting: %d", esp_get_free_heap_size());

    static i2c_port_t i2c_port = I2C_NUM_0;

    ESP_LOGI(TAG, "Initializing I2C");
    i2c_init(i2c_port);

    ESP_LOGI(TAG, "Initializing AXP202");
    axp.read = &i2c_read;
    axp.write = &i2c_write;
    axp.handle = &i2c_port;

    axp202_init(&axp);
    // axp192_ioctl(&axp, AXP192_COULOMB_COUNTER_ENABLE, NULL);
    // axp192_ioctl(&axp, AXP192_COULOMB_COUNTER_CLEAR, NULL);

    ESP_LOGI(TAG, "Initializing PCF8563");
    pcf.read = &i2c_read;
    pcf.write = &i2c_write;
    pcf.handle = &i2c_port;

    ESP_LOGI(TAG, "Initializing display");
    hagl_init();
    hagl_clear_screen();

    ESP_LOGI(TAG, "Initializing non volatile storage");
    nvs_init();

    ESP_LOGI(TAG, "Initializing wifi");
    wifi_init();

    ESP_LOGI(TAG, "Start SNTP sync");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_time_sync_notification_cb(sntp_set_rtc);
    sntp_init();

    ESP_LOGI(TAG, "Heap after init: %d", esp_get_free_heap_size());

    xTaskCreatePinnedToCore(rtc_task, "RTC", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(log_task, "Log", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(backbuffer_task, "Backbuffer", 8192, NULL, 1, NULL, 0);
}
