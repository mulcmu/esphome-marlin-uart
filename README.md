# esphome-marlin-uart
Custom ESPHome component for integrating a FLSUN QQ-S Pro printer with Home Assistant.  The printer main board includes a ESP8266 module connected to one of the serial outputs.  The stock printer firmware has been replaced with [Marlin 2.x]( https://github.com/Foxies-CSTL/Marlin_2.0.x/).  Main usage is printing from the SD card.  The stock wifi firmware or [ESP3D](https://github.com/luc-github/ESP3D) didn't have a good way to interface with HA.  This custom component for ESPhome supports the following:

- Current temperature and setpoint for bed and extruder. 
- Printer status: Idle, Preheat, Cooling, Printing, Filament Runout, Paused, Aborted, Finished Printing, and Halted. 
- HA services to set extruder and bed temperature setpoints.
- Percent complete, elapsed print time, and estimated remaining time are provided.  *Include M77 then M75 in start gcode to reset the print timer after bed and nozzle are heated.  Otherwise preheat time is included when printing from SD card and causes estimate to be inaccurate.*

For initial installation follow the https://github.com/Foxies-CSTL/Marlin_2.0.x/wiki/5.Firmware-Wifi guide to flash the MksWifi.bin.  You will need to build the firmware in ESPHome and rename the bin file to MksWifi.bin.  After initial installation, OTA updates via ESPHome work well.  Alternatively, module can be removed and flashed externally if you can adapt it properly to a programmer.

TODO:

- Evaluate upload file to SD card capability
- Add support for multiple extruders
- Evaluate using slicer computed time
- Format repository to be compatible with ESPHome External Components

