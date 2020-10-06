## Lilygo T-Watch 2020 Kitchen Sink

```
$ git clone git@github.com:tuupola/esp_twatch2020.git --recursive
$ cd esp_twatch2020
$ cp sdkconfig.twatch2020 sdkconfig
$ make -j8 flash
```

## AXP202

|        | Purpose |
| ------ | ------- |
| DC-DC1 | PWM charger.|
| DC-DC2 | Not used 0.7V to 2.275V, 1.6A.|
| DC-DC3 | ESP32, 0.7V to 3.5V, 1.2A. **Always enable!** |
| LDO1   | Always on 30mA. |
| LDO2   | Display backlight 1.8V to 3.3V, 200mA. |
| LDO3   | Audio power 0.7V to 3.5V, 200mA. |
| LDO4   | Not used 1.8V to 3.3V, 200mA. |
| LDO5   | Not used 1.8V to 3.3V, 50mA. |


Watch stops booting when LDO3 is enabled?