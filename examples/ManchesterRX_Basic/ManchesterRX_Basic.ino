
#include <Manchester.h>

/*

  Manchester Receiver example
  
  In this example receiver will receive one 16 bit number per transmittion

  try different speeds using this constants, your maximum possible speed will 
  depend on various factors like transmitter type, distance, microcontroller speed, ...

    MAN_1200 0
    MAN_2400 1
    MAN_4800 2
    MAN_9600 3
    MAN_19200 4
    MAN_38400 5
    MAN_76800 6
    MAN_153600 7
    MAN_307200 8

*/

#define RX_PIN 4
#define LED_PIN 13

uint8_t moo = 1;

void setup() {
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, moo);
  man.setupReceive(RX_PIN, MAN_1200);
  man.beginReceive();
}

void loop() {
  if (man.receiveComplete()) {
    uint16_t m = man.getMessage();
    man.beginReceive(); //start listening for next message right after you retrieve the message
    moo = ++moo % 2;
    digitalWrite(LED_PIN, moo);
  }
}


