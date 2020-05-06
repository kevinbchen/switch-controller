/*
   Sketch for the transmitter unit that the Xbox One controller plugs into.
   Hardware: Arduino Pro Mini (3.3V), USB Host Shield Mini, HT-05 module.
*/
#include <XBOXONE.h>

// Set to 1 to print packets
#define DEBUG 0

#define SEND_MS 10
#define TOGGLE_MS 20

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

#define BTSerial Serial

USB Usb;
XBOXONE Xbox(&Usb);
State lastState;
bool turboMode = false;
bool turboModeClick = false;
bool turboToggle = false;
unsigned long lastToggleTime = 0;
unsigned long lastSendTime = 0;

byte convertAnalogValueX(int16_t value) {
  byte byteVal = (byte)min(((value >> 8) + 128), 254);
  if (abs(byteVal - 128) < 16) {
    byteVal = 128;
  }
  return byteVal;
}

byte convertAnalogValueY(int16_t value) {
  // Flip y-axis
  if (value == -32768) {
    value = -32767;
  }
  value = -value;
  byte byteVal = (byte)min(((value >> 8) + 128), 254);
  if (abs(byteVal - 128) < 16) {
    byteVal = 128;
  }
  return byteVal;
}

// Return true if states are different enough to warrant immediate sending
bool diffStates(State& state1, State& state2) {
  return (state1.data.buttons1 != state2.data.buttons1 ||
          state1.data.buttons2 != state2.data.buttons2 ||
          state1.data.dir != state2.data.dir ||
          abs(state1.data.lx - state2.data.lx) >= 16 ||
          abs(state1.data.ly - state2.data.ly) >= 16 ||
          abs(state1.data.rx - state2.data.rx) >= 16 ||
          abs(state1.data.ry - state2.data.ry) >= 16);
}

byte setButtonValue(byte& buttons, int i, bool value) {
  int mask = (1 << i);
  buttons = (buttons & ~mask) | ((value << i) & mask);
  return buttons;
}

void setup() {
  //Serial.begin(9600);
  BTSerial.begin(38400);

  if (Usb.Init() == -1) {
    //Serial.print(F("\r\nOSC did not start"));
    while (1);
  }
  //Serial.print(F("\r\nXBOX USB Library Started"));

  lastState.data.buttons1 = 0x00;
  lastState.data.buttons2 = 0x00;
  lastState.data.lx = 128;
  lastState.data.ly = 128;
  lastState.data.rx = 128;
  lastState.data.ry = 128;
  lastState.data.dir = 8;

  lastToggleTime = millis();
  lastSendTime = millis();
}

void loop() {
  unsigned long currTime = millis();
  Usb.Task();
  if (!Xbox.XboxOneConnected) {
    delay(1);
    return;
  }

  // Read in xbox controller state
  State state;
  state.data.buttons1 = 0x00;
  state.data.buttons2 = 0x00;
  state.data.lx = 128;
  state.data.ly = 128;
  state.data.rx = 128;
  state.data.ry = 128;
  state.data.dir = 8;

  // byte buttons1;  // 0:Y, 1:B, 2:A, 3:X, 4:L, 5:R, 6:ZL, 7:ZR
  // byte buttons2;  // 0:-, 1:+, 2:Lstick, 3:Rstick, 4:Home, 5:Capture
  const ButtonEnum buttons1Mapping[6] = {X, A, B, Y, L1, R1};
  for (int i = 0; i < 6; i++) {
    state.data.buttons1 |= Xbox.getButtonPress(buttons1Mapping[i]) ? (1 << i) : 0;
  }
  if (Xbox.getButtonPress(L2) > 512) {
    state.data.buttons1 |= (1 << 6);
  }
  if (Xbox.getButtonPress(R2) > 512) {
    state.data.buttons1 |= (1 << 7);
  }
  const ButtonEnum buttons2Mapping[6] = {BACK, START, L3, R3, XBOX, SYNC};
  for (int i = 0; i < 6; i++) {
    state.data.buttons2 |= Xbox.getButtonPress(buttons2Mapping[i]) ? (1 << i) : 0;
  }

  state.data.lx = convertAnalogValueX(Xbox.getAnalogHat(LeftHatX));
  state.data.ly = convertAnalogValueY(Xbox.getAnalogHat(LeftHatY));
  state.data.rx = convertAnalogValueX(Xbox.getAnalogHat(RightHatX));
  state.data.ry = convertAnalogValueY(Xbox.getAnalogHat(RightHatY));

  int dirX = 0;
  int dirY = 0;
  if (Xbox.getButtonPress(UP)) {
    dirY += 1;
  }
  if (Xbox.getButtonPress(DOWN)) {
    dirY -= 1;
  }
  if (Xbox.getButtonPress(LEFT)) {
    dirX -= 1;
  }
  if (Xbox.getButtonPress(RIGHT)) {
    dirX += 1;
  }
  int dirIdx = (dirY + 1) * 3 + (dirX + 1);
  const byte dirMapping[9] = {5, 4, 3, 6, 8, 2, 7, 0, 1};
  state.data.dir = dirMapping[dirIdx];

  // If both + and - are clicked, toggle turbo mode
  if ((state.data.buttons2 & B00000011) == B00000011) {
    turboModeClick = true;
  } else if ((state.data.buttons2 & B00000011) == B00000000) {
    Xbox.setRumbleOff();
    if (turboModeClick) {
      //Serial.println("turbo enabled");
      turboMode = !turboMode;
      turboToggle = 1;
      turboModeClick = false;
    }
  }
  if (turboMode) {
    // Only toggle A (2) and Y (0) for now
    if (state.data.buttons1 & (1 << 2)) {
      setButtonValue(state.data.buttons1, 2, turboToggle);
    }
    if (state.data.buttons1 & (1 << 0)) {
      setButtonValue(state.data.buttons1, 0, turboToggle);
    }
    if (currTime - lastToggleTime >= TOGGLE_MS) {
      lastToggleTime = currTime;
      turboToggle = !turboToggle;
    }
  }

  // Send an update every SEND_MS, or sooner if the state changed enough
  if (currTime - lastSendTime >= SEND_MS || diffStates(lastState, state)) {
    lastState = state;
    lastSendTime = currTime;
    BTSerial.write(0xFF);
    BTSerial.write(state.raw, NUM_STATE_BYTES);
    
    //for (int i = 0; i < NUM_STATE_BYTES; i++) {
    //  Serial.print(" " + String(state.raw[i]));
    //}
    //Serial.println("");
  }

  // Read response
  if (BTSerial.available()) {
    byte data = BTSerial.read();
    //Serial.println("got response: " + String(data));
  }
}
