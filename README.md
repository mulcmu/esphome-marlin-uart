# esphome-marlin-uart
 Work in progress for integrating a FLSUN QQ-S Pro printer with home assistant using ESPhome.  The printer main board includes a ESP8266 module connected to one of the serial outputs.  Stock printer firmware has been replaced with Marlin 2.x.

Temperature reports and printer status currently implemented.

For initial installation follow the https://github.com/Foxies-CSTL/Marlin_2.0.x/wiki/5.Firmware-Wifi guide to flash the MksWifi.bin.  You will need to build the firmware in ESPhome and rename the bin file to MksWifi.bin.  After initial installation OTA updates should work fine.

TODO:

- Thermal runaway / halt status added.
- Elapsed print time
- Estimated remaining time
- Service to preheat nozzle/bed
- Upload file to SD card

