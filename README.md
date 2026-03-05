# BananaKit2

🍌 BananaKit2 is a custom designed MPU development platform based on Arduino
Nano. The goal of this project is to bridge the gap between ideas and fast
prototyping to accelerate development process and making project more organized.

In traditional Arduino/MCU project prototyping, you build cuicuit on breadboard and
develop your firmware, and when you finish the project you discard your cuicuit and leave your
code forgotten.

Here, in prototyping stage you first design cuicuit blueprint, inspect the blueprint, then use
soldering iron to build the prototyping PCB and develop software module
that are compatable with BananaKit2 and *keep them permanently* before you
switch to the next project.

Later when you need the same function in previous work, just plugin the old
PCB module designed previously on the BananaKit2 and turn on its software
module, everything still works in the same way, no need to wire breadboard
again, no need to write code again.

![BananaKit2 All Combonents](doc/bananakit2_family.jpg)

## Get Started

Make sure Arduino IDE is installed on your local machine. Clone this repository
into the library directory (usually in `~/Arduino/libraries/` for Linux and 
`Your Home Directory\Documents\Arduino\libraries\` for Windows).

To turn on/off software modules, go to `src/bananakit.h` and comment/uncomment
the macros, i.e. uncomment `#define ENABLE_JOYSTICK_MODULE` will enable the
joystick module.

Then, open `examples/BananaKit2/BananaKit2.ino` in Arduino IDE, build and
upload the firmware into BananaKit2.

Turn on BananaKit2, the LCD screen should display the Main Menu. Use `2` and `5`
buttons on the IR controller to select the software module name, and use `6`
button to start running the module. Use `4` button to go back to the Main Menu.

## Build BananaKit2 Hardware

Checkout [Hardware List](doc/hardware_list.md) for detailed purchase list.

After you get all components, build the [Mainboard](doc/mainboard_specs.md) 
and the [Power Supply Module](doc/psm_specs.md) using Soldering Iron, use a multimeter to
measure the output voltage on LM2569 and adjust to 5V before powering on the system.
Then wire the system together according to the [Diagram](doc/system_diagram.md).
You can drill holes on an acrylic sheet as a base or 3D print a case
for BananaKit2.

## Advanced

### Build Custom Module

BananaKit2 supports two types of custom module based on the common prototype PCB
dimensions: 3x7cm and 4x6cm. You can purchase these PCBs online. Each module
requires two 8Pin male connectors. Checkout the [Removable Module
Specs](doc/custom_module_specs.md) for
details.

The 3x7cm module has I2C, UART, D3-D6, A1-A5 and A7 pin access to the Arduino
Nano, suitable for analog sensors. The 4x6cm module has SPI, D3-D6, D8 and D9
pin access mainly for digital and SPI components.

Both types of modules are power supplied at 5V and can be connected
simultaneously to BananaKit2.


### Develop Module Software

To develop custom software module compatable with BananaKit2, you need to change
the following parts in the repository:

1. Register the module IO PIN numbers to `src/bananakit.h`;

Example:
```c
#define NRF24_CE_PIN        10
#define NRF24_CSN_PIN       9
#define NRF24_IRQ_PIN       3
```

2. Add a macro enabler to `src/bananakit.h`;

Example:
```c
#define ENABLE_JOYSTICK_MODULE
```

3. Add macros to include your module header file to
   `examples/BananaKit2/BananaKit2.ino`;

Example:
```c
#ifdef ENABLE_RADIO_MODULE
#include "module/radio_module.h"
#endif
```

4. Add macros to register your module interface to the Main Menu;

Example:
```c
#ifdef ENABLE_MICROSD_MODULE
register_new_node(
    "microSD",
    microsd_module_init,
    microsd_module_update,
    microsd_module_resume,
    microsd_module_exit,
    Main_menu
);
#endif
```

5. Put your source code interface into `src/module/`, and put your custom
   libraries into `src/lib/`

Note:

Due to the limitation of ATMega328p MCU, you may encounter oversized firmware
that failed to upload if you turn on too many modules at the same time.
Try to reduce your firmware size by turning off uncessary/unused modules.

### Pinout Reference

[BananaKit2 Mainboard Pinout]()

[Module Pinout 3x7CM]()

[Module Pinout 4x6CM]()

### Supported Module Blueprints

[Weather Module Blueprint]()

[GPS IMU Module Blueprint]()

[Radio Module Blueprint]()

[DC Motor Module Blueprint]()

[Joystick Module Blueprint]()

[NRF24 Module Blueprint]()

[MicroSD Module Blueprint]()


## Dependencies & Credits

Here is a list of open source repositories used by this project:

[LiquidCrystal\_I2C](https://github.com/marcoschwartz/LiquidCrystal_I2C)
[Adafruit BME280 Library](https://github.com/adafruit/Adafruit_BME280_Library)
[Adafruit BNO055](https://github.com/adafruit/Adafruit_BNO055)
[IRremote](https://github.com/z3t0/Arduino-IRremote)
[MPU6050](https://github.com/electroniccats/mpu6050)
[RF24](https://nRF24.github.io/RF24/)
[SD](http://www.arduino.cc/en/Reference/SD)
[SparkFun CCS811 Arduino
Library](https://github.com/sparkfun/SparkFun_CCS811_Arduino_Library)
[Thin](https://github.com/siqiyan/thin)
[CRC](https://github.com/RobTillaart/CRC)
