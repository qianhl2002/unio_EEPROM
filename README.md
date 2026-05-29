# unio_EEPROM
Arduino esp32 library for Microchip UNI/O OneWire interface
=============

Arduino esp32 library for Microchip UNI/O OneWire interface based on PonomarovP(https://github.com/PonomarovP/Microchip_EEPROM) code

Implements all timings and commands of UNI/O interface for Microchip EEPROM devices

Tested on Microchip 11LC040 EEPROM,use ESP32 s3.

For 11LC141 and 11AA161 versions change "UNIO_device_address" to 0xA1

## NOTE: "TimerOne" Arduino library is not required! Use a custom delay function.

For usage see example
