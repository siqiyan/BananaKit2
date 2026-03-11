# MicroSD Module

![MicroSD Module](/doc/microsd_module.png)

This module is relatively simple, it only wires the SPI interface between
the MicroSD card and Arduino Nano. I use the `D10/CS` pin on Arduino Nano
so the Nano itself cannot be used as a slave device for other SPI devices.

TODO: Add a jumper to switch the `CS` pin for flexibility.

Currently the program here only dump the SD card status to check
if the module is functioning. To use it, plug in the module and
uncomment `#define ENABLE_MICROSD_MODULE` in `src/bananakit.h`,
re-compile and upload.