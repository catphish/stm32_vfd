# STM32 variable frequency drive controller

This project aims to be an open source torque controller for AC induction motors.

## Hardware

This software runs on an STM32. In addition, the following hardware is required:

* 6 x IGBT or MOSFET
* 6 x Isolated FET drivers
* 3 x Hall effect current sensors
* Rotary encoder
* A large film capacitor

An example schematic can be found here: https://nutty.tk/inverter3.pdf

## Overview

The software generates a 3-phase output using PWM. The frequency and voltage
are adjusted to create a torque proportional to the "throttle" input.

## Algorithm

The algorighm is very simple and is based on setting the slip of the motor.
The output frequency is the shaft frequency plus a slip, where the slip is
proportional to the throttle input. The current is controlled to keep it
proportional to the slip (up to the input voltage).

## Testing

So far this has only been tested up to 1A @ 48VDC on a 250W motor. It is my
hope that this should scale to larger builds for electric vehicle use. More
experimentation will follow.

My low power test build: https://imgur.com/a/IR173Ng

## Field oriented control

Previous iterations of this code have implemented FOC, however overall the
simple slip based algorithm has produced better results so far.
