# Joystick Module

![Joystick Module](/doc/joystick_module.png)

I put a joystick (contains two ananlog channels and a built-in push button)
and two push buttons onto this module mainly for
used as a control interface my other projects, such as
[RC Station](/src/app/rc_station/).

This folder contains a test program to see if all the components
are working properly. Also I can check the neutral position of the
x/y axis in case they are not in the absolute middle.

To run this module, simply plug the module and uncomment `#define ENABLE_JOYSTICK_MODULE`
in `src/bananakit.h`, re-compile and upload firmware into BananaKit2.