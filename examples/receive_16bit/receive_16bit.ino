#include <MANCHESTER.h>

#define RxPin 2

void setup() {
  // Set digital TX pin
  MANRX_SetRxPin(2);
  // Prepare interrupts
  MANRX_SetupReceive();
  // Begin receiving data
  MANRX_BeginReceive();
  
  pinMode(0, OUTPUT);
}

unsigned int last_data;
void loop() {
  if (MANRX_ReceiveComplete()) {
    unsigned int data = MANRX_GetMessage();
    MANRX_BeginReceive();
    // Handle data...
    if (data == last_data + 1) {
      PINB = 1; // toggle pin 0
    }
    last_data = data;
  }
}


