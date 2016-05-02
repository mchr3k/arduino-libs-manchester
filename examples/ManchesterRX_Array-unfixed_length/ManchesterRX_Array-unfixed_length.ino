
#include "Manchester.h"

/*

  Manchester Receiver example
  
  In this example receiver will receive array of 10 bytes per transmittion

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

#define RX_PIN 4
#define LED_PIN 13

uint8_t moo = 1;
#define BUFFER_SIZE 22
uint8_t buffer[BUFFER_SIZE];

void setup() 
{
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, moo);
  Serial.begin(19200);
  man.setupReceive(RX_PIN, MAN_9600);
  man.beginReceiveArray(BUFFER_SIZE, buffer);
}

void loop() 
{
  if (man.receiveComplete()) 
  {
    uint8_t receivedSize = 0;

    //do something with the data in 'buffer' here before you start receiving to the same buffer again
    receivedSize = buffer[0];
    for(uint8_t i=1; i<receivedSize; i++)
      Serial.write(buffer[i]);
    
    Serial.println();

    man.beginReceiveArray(BUFFER_SIZE, buffer);
    moo = ++moo % 2;
    digitalWrite(LED_PIN, moo);
  }
}