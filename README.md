# switch-controller

This is a simple electronics project to wirelessly control a Nintendo Switch using an Xbox One controller.
There are 2 main parts:
- The **transmitter** unit acts as a usb host for the Xbox controller; it reads in the controller input and periodically sends the input data to the receiver via bluetooth.
- The **receiver** unit connects to the Nintendo Switch (via the dock) and emulates a 3rd-party controller with the given input data.

![20200507_003424](https://user-images.githubusercontent.com/2881968/175892978-a62cf80e-5c9a-436b-a24d-054c34099c26.jpg)

Note that there are commercial 3rd-party controllers and wireless adapters that achieve the same thing much more conveniently; this was mainly a fun side project to get back into electronics and 3d-printing. Of course, having control over the software does enable things like a turbo mode for specific buttons (to hit rocks and chop trees more easily in Animal Crossing!).

Although the software is currently written for an Xbox One Controller, other usb controllers should also work, assuming the usb host shield library support them. In addition, the transmitter can also be replaced by a PC, opening up potential for some basic automation.

## Repository Contents

- **SwitchControllerReceiver/** - Arduino sketch for the receiver
- **SwitchControllerTransmitter/** - Arduino sketch for the transmitter
- **BluetoothAT/** - Helper Arduino sketch for sending AT commands to the bluetooth modules
- **enclosures/** - Fusion 360 and .stl files for the  3d-printed enclosures

## Hardware
**Transmitter** components:
- Arduino Pro Mini (3.3V)
- USB host shield (Mini)
- HC-05 bluetooth module
- MT3608 boost converter

The transmitter is powered directly from the Xbox controller's 2 AA batteries, stepped up to 5V. This gets regulated back down to 3.3V for all the components, but USB VBUS still needs 5V. Probably pretty inefficient, but needs no modification to the controller itself , besides exposing connections to the batteries. In total, the transmitter draws ~120 mA from the batteries (~100 mA from the controller itself).

**Receiver** components:
- Arduino Pro Micro (5V)
- HC-06 bluetooth module

The receiver is powered by via USB (5V). The HC-06 has its own regulator for VCC, so we just need a voltage divider for the module's RX pin.

## Software
Dependencies:
- [Arduino SwitchJoystick Library](https://github.com/HackerLoop/Arduino-JoyCon-Library-for-Nintendo-Switch) (for receiver - note installation instructions for setting the USB vid/pid) 
- [USB Host Library Rev.2.0](https://github.com/felis/USB_Host_Shield_2.0) (for transmitter)

The **transmitter** uses the USB Host Library to read in the Xbox controller state. It encodes the state into 7 bytes and sends them over the bluetooth serial link, along with an initial `0xFF` byte as a sentinel. A state "packet" is sent every 10ms, or immediately if the controller state changes (ignoring small changes in analog values). The 7 data bytes sent are:
```
    byte buttons1;  // 0:Y, 1:B, 2:A, 3:X, 4:L, 5:R, 6:ZL, 7:ZR
    byte buttons2;  // 0:-, 1:+, 2:Lstick, 3:Rstick, 4:Home, 5:Capture
    byte lx;
    byte ly;
    byte rx;
    byte ry;
    byte dir;
```
The button bytes are simple bitmasks. The `lx/ly/rx/ry` bytes are in [0, 254] and represent each axis of the left/right analog sticks, wiht 128 being center (255 avoided because of sentinel). The `dir` byte is in [0, 8] and represents the number of 45 degree clockwise increments from up for the directional pad, with 8 being no direction.

The **receiver** simply reads in these packets, and uses the SwitchJoystick Library to send these inputs to the Nintendo Switch. There's some simple logic for skipping packets if the serial RX queue is being flooded.

## Misc Notes

### Latency
Latency was intially a concern, but I was able to reduce it somewhat with a combination of:
- Sending a packet immediately on controller state changes
- Using the hardware serial for the bluetooth on the transmitter (though unfortunately means can't use serial debugging)
- Increasing bluetooth baudrate to 38400

I don't have qualitative numbers, but I think it's fairly reasonable now.
