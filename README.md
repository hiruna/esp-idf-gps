# esp-idf-gps

## Overview
ESP-IDF component/driver ([libnmea](https://github.com/jacketizer/libnmea) wrapper) for reading &amp; parsing GPS module data via UART, inspired by https://github.com/igrr/libnmea-esp32 and [ESP-IDF nmea_parser example](https://github.com/espressif/esp-idf/blob/master/examples/peripherals/uart/nmea0183_parser/main/nmea_parser.c).

## This is currently WIP.

TODO:

+ I'm planning to optimize the code for 10hz nav updates.
+ Configure Kconfig options to setup UART port/buffer sizes etc.
+ Implement UBLOX config commands to configure NEO M modules?

### Example
A working example of this component: [gps_reader](./examples/gps_reader). 

### TLDR
[Ivan's](https://github.com/igrr) [libnmea-esp32](https://github.com/igrr/libnmea-esp32) works great.
I wanted to make my own component with the additional functionalities + get my hands dirty with CMake.
Might end up using it directly as the source of libnmea later...

I tried following the [ESP-IDF import_lib example](https://github.com/espressif/esp-idf/blob/master/examples/build_system/cmake/import_lib/components/tinyxml2/CMakeLists.txt) but 
CLion did not pick up the built lib :( (but IDF did!). See my [failed attempt](./CMakeLists.txt_failedattempt).