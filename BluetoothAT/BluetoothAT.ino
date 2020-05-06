/*
   Simple sketch for sending AT commands to a HC-05/HC-06 module.
*/
#include <SoftwareSerial.h>

#if 1
// Transmitter
SoftwareSerial PCSerial(2, 3);
#define BTSerial Serial
#define BT_BAUD 38400
#else
// Receiver
#define PCSerial Serial
#define BTSerial Serial1
#define BT_BAUD 38400
#endif

void setup() {
  PCSerial.begin(9600);
  PCSerial.println("Enter AT commands:");

  // HC-06 default serial speed is 9600
  BTSerial.begin(BT_BAUD);
}

void loop() {
  if (BTSerial.available()) {
    PCSerial.write(BTSerial.read());
  }
  if (PCSerial.available()) {
    BTSerial.write(PCSerial.read());
  }
}
