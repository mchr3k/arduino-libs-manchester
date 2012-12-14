
#include <MANCHESTER.h>

int ls;

void setup() 
{  
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  MANCHESTER.SetTxPin(4);
  //MANCHESTER.SetTimeOut(1000);
  
  ls = 0;
}

void loop()
{
  
  if(digitalRead(1)) {
    unsigned char data[1];
    data[0] = 2;
    MANCHESTER.TransmitBytes(1, &data[0]);
    delay(1000);
  }
  if(digitalRead(2)) {
    unsigned char data[1];
    data[0] = 1;
    MANCHESTER.TransmitBytes(1, &data[0]);
    delay(1000);
  }
}
