#include <MANCHESTER.h>

void setup() {
  MANCHESTER.SetTxPin(0);
}

unsigned int transmit_data = 0;
void loop() {
  MANCHESTER.Transmit(transmit_data);
  transmit_data += 1;
  delay(250);
}
