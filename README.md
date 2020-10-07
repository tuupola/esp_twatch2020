## Lilygo T-Watch 2020 Kitchen Sink

This repository aims to document and show how to use the different peripherals. For graphical demo check also [tuupola/esp_effects](https://github.com/tuupola/esp_effects). Compile the kitchen sink with the following:

```
$ git clone git@github.com:tuupola/esp_twatch2020.git --recursive
$ cd esp_twatch2020
$ cp sdkconfig.twatch2020 sdkconfig
$ make -j8 flash
```

## AXP202

|         | Purpose |
| ------- | ------- |
| DC-DC1  | PWM charger.|
| DC-DC2  | Not used 0.7V to 2.275V, 1.6A.|
| DC-DC3  | ESP32, 0.7V to 3.5V, 1.2A. **Always enable!** |
| LDO1    | Always on 30mA. |
| LDO2 1) | Display backlight 1.8V to 3.3V, 200mA. |
| LDO3 2) | Audio power 0.7V to 3.5V, 200mA. |
| LDO4    | Not used 1.8V to 3.3V, 200mA. |
| LDO5    | Not used 1.8V to 3.3V, 50mA. |

1) Just turning LDO2 on is not enough. Must also enable PWM.
2) Watch stops booting when LDO3 is enabled?
