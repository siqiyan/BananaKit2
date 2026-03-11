# Weather Module

The Weather Module consists of AM2302/DHT22, BME/BMP280, and CCS811 sensors.

This module can provide temperature, humidity, pressure, altitude, eCO2 and tVOC
sensor readings.

![Weather Module](/doc/weather_module.png)

One thing that gives me trouble was
that the CCS811 sensor needs to ground the WAKE pin before it can read data,
and I wasted a lot of time to figure out.

To use the Weather Module, plug in the module into the Right-hand slot
of BananaKit2, check the **Orientation** of the module before powering
on the system to prevent damage to the circuit.

Turn on the `#define ENABLE_WEATHER_MODULE` macro in `src/bananakit.h` and
re-compile the firmware in `examples/BananaKit2/BananaKit2.ino` and upload
the firmware.