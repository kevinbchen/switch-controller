/*
   Sketch for the receiver unit that plugs into the Nintendo Switch.
   Hardware: Arduino Pro Micro, HT-06 module.
*/
#include "SwitchJoystick.h"

// Set to 1 to print data to serial instead of emulating joystick
#define DEBUG 0

#define NUM_STATE_BYTES 7
union State {
  struct {
    byte buttons1;  // 0:Y, 1:B, 2:A, 3:X, 4:L, 5:R, 6:ZL, 7:ZR
    byte buttons2;  // 0:-, 1:+, 2:Lstick, 3:Rstick, 4:Home, 5:Capture
    byte lx;
    byte ly;
    byte rx;
    byte ry;
    byte dir;
  } data;
  byte raw[NUM_STATE_BYTES];
};

#define BTSerial Serial1

SwitchJoystick_ Joystick;
State state;
int currByte = -1;
unsigned long lastPollTime;

void setup() {
  BTSerial.begin(38400);

#if DEBUG
  Serial.begin(9600);
#else
  Joystick.begin(false);
  Joystick.setXAxis(128);
  Joystick.setYAxis(128);
  Joystick.setZAxis(128);
  Joystick.setRzAxis(128);
  Joystick.sendState();
#endif

  currByte = -1;
  lastPollTime = millis();
}

void loop() {
  int availableBytes = BTSerial.available();
  if (availableBytes > 0) {
    byte data = BTSerial.read();
    if (data == 0xFF) {
      // Start of a packet
      if (availableBytes > SERIAL_RX_BUFFER_SIZE / 2) {
        // Queue too full, don't process this packet
        currByte = -1;
      } else {
        currByte = 0;
      }
    } else if (currByte != -1) {
      state.raw[currByte++] = data;
    }
  }
  if (currByte < NUM_STATE_BYTES) {
    return;
  }
  // Got a full packet, send back response and update joystick state
  currByte = -1;
  BTSerial.write(availableBytes);
  BTSerial.flush();

#if DEBUG
  for (int i = 0; i < NUM_STATE_BYTES; i++) {
    Serial.print(" " + String(state.raw[i]));
  }
  Serial.println("");
  // Delay to simulate joystick workload
  delay(10);
#else

  for (int i = 0; i < 8 ; i++) {
    Joystick.setButton(i, state.data.buttons1 & (1 << i));
  }
  for (int i = 0; i < 8 ; i++) {
    Joystick.setButton(i + 8, state.data.buttons2 & (1 << i));
  }
  Joystick.setXAxis(state.data.lx);
  Joystick.setYAxis(state.data.ly);
  Joystick.setZAxis(state.data.rx);
  Joystick.setRzAxis(state.data.ry);

  if (state.data.dir == 8) {
    Joystick.setHatSwitch(-1);
  } else {
    Joystick.setHatSwitch(state.data.dir * 45);
  }
  Joystick.sendState();
#endif
}
