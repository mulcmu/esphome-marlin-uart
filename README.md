# esphome-marlin-uart
Work in progress for integrating a FLSUN QQ-S Pro printer with Home Assistant using ESPHome.  The printer main board includes a ESP8266 module connected to one of the serial outputs.  Stock printer firmware has been replaced with Marlin 2.x.

- Temperature reports and printer status currently implemented.
- Services are implemented to set extruder or bed temperature.
- Elapsed print time and estimated remaining time are provided.  *Include M77 then M75 in start gcode to reset the print timer after bed and nozzle are heated.  Otherwise preheat time is included when printing from SD card and causes estimate to be inaccurate.*

For initial installation follow the https://github.com/Foxies-CSTL/Marlin_2.0.x/wiki/5.Firmware-Wifi guide to flash the MksWifi.bin.  You will need to build the firmware in ESPHome and rename the bin file to MksWifi.bin.  After initial installation OTA updates should work fine.

TODO:

- Thermal runaway / halt status added.
- Evaluate upload file to SD card capability

