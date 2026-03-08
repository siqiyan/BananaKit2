# DC Motor Module

![DC Motor Module](/doc/dc_motor_module.png)

The DC Motor Module is for controling a motor using a MOSFET (RFP30N06LE)
as switch, and measuring the input voltage of the motor by dividing the
voltage using two resistors (47k and 10k).

The motor speed is controled by PWM using the Arduino analogWrite
function.

To use the module, plug in the DC Motor Module into the Right slot
of BananaKit2. Use two jumper cables to supply 7.4v unregulated power
from the Power Supply Module to the DC Motor Module.

Turn on `#define ENABLE_DC_MOTOR_MODULE` in `src/bananakit.h`, re-compile
the firmware in `examples/BananaKit2/BananaKit2.ino` and upload the
firmware into BananaKit2.

You can check the motor speed and voltage from the LCD and adjust
motor speed by rotating the potentiometer on the BananaKit2 mainboard.