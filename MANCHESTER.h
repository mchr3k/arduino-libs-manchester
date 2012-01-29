/*
This code is based on the Atmel Corporation Manchester
Coding Basics Application Note.

http://www.atmel.com/dyn/resources/prod_documents/doc9164.pdf

Quotes from the application note:

"Manchester coding states that there will always be a transition of the message signal 
at the mid-point of the data bit frame. 
What occurs at the bit edges depends on the state of the previous bit frame and
does not always produce a transition. A logical “1” is defined as a mid-point transition
from low to high and a “0” is a mid-point transition from high to low.

We use Timing Based Manchester Decode.
In this approach we will capture the time between each transition coming from the demodulation
circuit."

Timer 2 is used with a ATMega328. Timer 1 is used for a ATtiny85.

This code gives a basic data rate as 1000 bits/s. In manchester encoding we send 1 0 for a data bit 0.
We send 0 1 for a data bit 1. This ensures an average over time of a fixed DC level in the TX/RX.
This is required by the ASK RF link system to ensure its correct operation.
The actual data rate is then 500 bits/s.
*/

#ifndef MANCHESTER_h
#define MANCHESTER_h

#define TxDefault 4  //the digital pin to use to transmit data
#define pulse 1000 //the individual transmit pulse width in msec

#define RxDefault 4  //default rx is 4

/*
Timer 2 in the ATMega328 and Timer 1 in a ATtiny85 is used to find the time between 
each transition coming from the demodulation circuit.
Their setup is for normal count. No connections. No interupts.
The divide ratio is /256 with 16 Mhz the 328 and /128 for the 85 with a 8 Mhz clock.
This gives 16 usec per count. For a normal pulse this is 62.5 counts.
With an error allowance of 22.5 usec we get the following:
*/
#define MinCount 40  //pulse lower count limit on capture
#define MaxCount 85  //pulse higher count limit on capture
#define MinLongCount 103  //pulse lower count on double pulse
#define MaxLongCount 147  //pulse higher count on double pulse

#define TimeOutDefault -1  //the timeout in msec default blocks

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif

class MANCHESTERClass
{
  public:
    MANCHESTERClass();  //the constructor
    void SetTxPin(char pin); //set the arduino digital pin for transmit. default 4.
    void Transmit(unsigned int data);  //transmit 16 bits of data
    void TransmitBytes(unsigned char numBytes, unsigned char *data); // transmit a byte array
    void SetRxPin(char pin);  //set the arduino digital pin for receive. default 4.
    unsigned int Receive(void);  //receive 16 bits of data. 0 if times out.
    unsigned char ReceiveBytes(unsigned char maxBytes, unsigned char *data); // receive a byte array
    unsigned char ReceivedTimeout(void); //whether the receive timed out
    void SetTimeOut(unsigned int timeout); //set timeout in ms. default blocks.
    void AddManBit(unsigned int *manBits, unsigned char *numMB,
                   unsigned char *curByte, unsigned char *data,
                   unsigned char bit);
    
  private:
    void sendzero(void);
    void sendone(void);
    unsigned char  RxPin;
    unsigned char  TxPin;
    unsigned int  TimeOut;
    unsigned long lastSend;
    unsigned char wasTimeout;
   
};//end of class MANCHESTER

extern MANCHESTERClass MANCHESTER;

#endif
