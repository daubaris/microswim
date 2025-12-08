# mcu

This example is for systems running RIOT OS.

Some modifications are needed for different kinds of devices. For example:

* For `ESP32` (`ESP32-DevKitC-VIE` in our case), include `USEMODULE += esp_wifi` as well as `CFLAGS += -DWIFI_SSID=\"<ssid>\"` and `CFLAGS += -DWIFI_PASS=\"<password>\"`.
* For `STM32F746G-DISCO`, include `USEMODULE += periph_eth`.

## Building

Build with `BOARD=stm32f746g-disco make` and then flash: `BOARD=stm32f746g-disco make flash-only`. More information about building projects with RIOT can be found [here](https://guide.riot-os.org/getting-started/building_example).

## Notes

Before flashing, make sure to add the correct IP address of the node to be contacted in `main.c`: `netutils_get_ipv4((ipv4_addr_t*)&member.addr.addr.ipv4, <your IP here.>);`.

