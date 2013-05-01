#include <MANCHESTER.h>

#define TxPin 4  //the digital pin to use to transmit data

unsigned int Tdata = 0;  //the 16 bits to send

void setup() 
{                
MANCHESTER.SetTxPin(TxPin);      // sets the digital pin as output default 4
}//end of setup

void loop() 
{
 Tdata +=1;
 MANCHESTER.Transmit(Tdata);
 delay(100);
}//end of loop


