#include <Manchester.h>

/*

  Manchester Transmitter example
  
  In this example transmitter will send one byte of data and 4bit sender ID per transmittion
  message also contains a checksum and receiver can check if the data has been transmited correctly

  try different speeds using this constants, your maximum possible speed will 
  depend on various factors like transmitter type, distance, microcontroller speed, ...

  MAN_300 0
  MAN_600 1
  MAN_1200 2
  MAN_2400 3
  MAN_4800 4
  MAN_9600 5
  MAN_19200 6
  MAN_38400 7


*/

#define TX_PIN 5  //pin where your transmitter is connected
#define LED_PIN 13 //pin for blinking LED
#define SENDER_ID 5

uint8_t moo = 1; //last led status
uint8_t transmit_data = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, moo);
  man.workAround1MhzTinyCore(); //add this in order for transmitter to work with 1Mhz Attiny85/84
  man.setupTransmit(TX_PIN, MAN_1200);
}

void loop() {
  //SENDER_ID can be ID of the sender if you have multiple transmitters so receiver knows who sent it
  //OR it can be receiver ID if you have multiple receivers so receiver knows it's for him
  //OR it can by just any arbitrary 4 bit number
  man.transmit(man.encodeMessage(SENDER_ID, transmit_data));
  transmit_data++;
  moo = ++moo % 2;
  digitalWrite(LED_PIN, moo);
}
