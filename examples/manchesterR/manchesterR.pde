#include <MANCHESTER.h>

#define RxPin 4

void setup() 
{ 
 MANCHESTER.SetRxPin(RxPin); //user sets rx pin default 4
 MANCHESTER.SetTimeOut(1000); //user sets timeout default blocks
 Serial.begin(9600);	// Debugging only
}//end of setup

void loop() 
{
 unsigned int data = MANCHESTER.Receive();
 Serial.println(data);
}//end of loop


