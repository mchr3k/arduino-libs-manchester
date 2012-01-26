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

#include "MANCHESTER.h"

#define HALF_BIT_INTERVAL 1000 // microseconds

MANCHESTERClass::MANCHESTERClass()  //constructor
{
  TxPin = TxDefault;
  pinMode(TxPin, OUTPUT);      // sets the digital pin 4 default as output 
  RxPin = RxDefault;  //sets the digital pin 4 default as input 
  pinMode(RxPin, INPUT);      // default digital pin as output
  TimeOut = TimeOutDefault; //default is to block

  #if defined( __AVR_ATtinyX5__ )
    #define TimerCount TCNT1   //ATtiny85 timer 1  
  #else
    //ATMega328 timer 2
    #define TimerCount TCNT2   //ATMega328 timer 2
    TCCR2A = 0x00;
    TCCR2B = 0x06; //counts every 16 usec with 16 Mhz clock
    TIMSK2 = 0x00;
  #endif   
}//end of constructor

unsigned char MANCHESTERClass::ReceivedTimeout(void)
{
  return wasTimeout;
}

/*
Record a new received Manchester bit.  If we've now received 16 Manchester bits,
spit a new decoded byte out into the received data.
*/
void MANCHESTERClass::AddManBit(unsigned int *manBits, unsigned char *numMB, 
                                unsigned char *curByte, unsigned char *data, 
                                unsigned char bit)
{
  *manBits <<= 1;
  *manBits |= bit;
  (*numMB)++;
  if (*numMB == 16)
  {
    unsigned char newData = 0;

    for (char i = 0; i < 8; i++)
    {
      // ManBits holds 16 bits of manchester data
      // 1 = LO,HI
      // 0 = HI,LO
      // We can decode each bit by looking at the bottom bit of each pair.
      newData <<= 1;
      newData |= (*manBits & 1); // store the one
      *manBits = *manBits >> 2; //get next data bit    
    }
    data[*curByte] = newData;
    (*curByte)++;
    *numMB = 0;
  }
}

/*
The 433.92 Mhz receivers have AGC, if no signal is present the gain will be set 
to its highest level.

In this condition it will switch high to low at random intervals due to input noise.
A CRO connected to the data line looks like 433.92 is full of transmissions.

Any ASK transmission method must first sent a capture signal of 101010........
When the receiver has adjusted its AGC to the required level for the transmisssion
the actual data transmission can occur.

We send 14 0's 1010...    It takes 1 to 3 10's for the receiver to adjust to 
the transmit level.

The receiver waits until we have at least 10 10's and then a start pulse 01.
The receiver is then operating correctly and we have locked onto the transmission.
*/
unsigned int MANCHESTERClass::Receive(void)
{
  unsigned char data[2];
  unsigned char rcvBytes;

  rcvBytes = ReceiveBytes(2, data);
  if (rcvBytes != 2)
      return 0;
  return (((int)data[0]) << 8) | (int)data[1];
}

unsigned char MANCHESTERClass::ReceiveBytes(unsigned char maxBytes, unsigned char *data)
{
  #if defined( __AVR_ATtinyX5__ )
    TCCR1 = 0x08;  //counts every 16 usec with 8Mhz clock
    TIMSK = 0;
  #endif

  unsigned long timeoutstart = millis();
  unsigned char curByte = 0;
  wasTimeout = 0;

  // we keep looking at the receiver output for a transmission
  for(;;)
  {
    unsigned char countlow = 0;
    unsigned char counthigh = 0;

    // Assume no lock until we get the correct preamble
    boolean locked = false;

    // transmitter sends:
    // - 14x 0's
    // - 1x  1
    // - 16x <data>
    // - 2x  0
    //
    // we lock onto 10x0 followed by 1x1.
    //
    // having a lock sequence shorter than the data means
    // we could lock onto the data if we are unlucky. However
    // we can add reliability using an extra layer on top of
    // this code.

    // The preamble (14x0, 1x1) is received as follows.
    // (0x10),HI,LO,HI,LO,HI,LO,HI,LO,LO,HI

    // The following loop attempts to sync on the regular transitions
    // and then detect the long low.

    // wait until RX 1
    while(digitalRead(RxPin) == 0);

    // start timing
    TimerCount = 0;

    // Each loop detects HI->LO and then LO->HI
    // We time the duration of the HI and the duration of the LO
    // We check the duration to ensure the transition time
    // was as expected.
    for(int i = 0; i < 16; i++)  //check at least 32 pulses
    {
      // wait until RX 0
      while(digitalRead(RxPin) != 0)
        counthigh = TimerCount;  //at this time count is still high
      TimerCount = 0;

      if((counthigh < MinCount) || (counthigh > MaxCount))
      {
        // Transition was too slow/fast
        locked = false;  
        break; //this cant be a valid pulse
      }//end of not valid

      // wait until RX 1
      while(digitalRead(RxPin) == 0)
        countlow = TimerCount;  //at this time count is still low
      TimerCount = 0;

      // Note on the 10th or later time through the loop
      // we are looking for a long low count.
      if ((countlow < MinCount) || 
          ((i < 10) && (countlow > MaxCount)) ||
          (countlow > MaxLongCount))
      {
        // Transition was too slow/fast
        locked = false;  
        break; //this cant be a valid pulse
      }//end of not valid

      if((i > 9) && (countlow > MinLongCount))
      {
        // We have seen at least 10 regular transitions
        // Lock sequence ends with 01
        // This is TX as HI,LO,LO,HI
        // We have seen a long low - we are now locked!
        locked = true;
        break;
      }
    }//end of get 10 capture pulses and start pulse

    if(millis() - timeoutstart > TimeOut)
    {
      wasTimeout = 1;
      return 0;  //timed out so return to caller
    }

    unsigned int manBits = 0; //the received manchester 32 bits
    unsigned char numMB = 0;  //the number of received manchester bits
    boolean start = true;  //remember to ignore the start bit is a one

    if(locked)  //have we detected a capture pulse train
    {
      // This section reads the raw RX input
      // It keeps reading data until either we lose the lock (due to the transmitter
      // stopping, or interference) or we hit the maximum number of bytes the user
      // wanted.  It then returns all complete bytes that were read.  It's up to the
      // user to decide what to do if fewer bytes than expected were returned.
      //
      // The first manchester bit is the HI from the end of the lock - ignore it
      // The lock ended with receiving a HI so we know we start with a HI here
      while (curByte < maxBytes)
      {
        if (!start)
          AddManBit(&manBits, &numMB, &curByte, data, 1);
        else
          start = false;

        // wait until RX 0
        while(digitalRead(RxPin) != 0)
          counthigh = TimerCount;  //end of count is high
        TimerCount = 0;

        if((counthigh < MinCount) ||
           (counthigh > MaxLongCount))
        {
          // Interference - give up
          locked = false;
          break;
        }

        if(counthigh > MinLongCount)  //is this a double 1
        {
          AddManBit(&manBits, &numMB, &curByte, data, 1);
        }//end of we have a double 1

        AddManBit(&manBits, &numMB, &curByte, data, 0);

        // wait until RX 1
        while(digitalRead(RxPin) == 0)
          countlow = TimerCount;  //end of count is low
        TimerCount = 0;

        if((countlow < MinCount) ||
           (countlow > MaxLongCount))
        {
          // Interference - give up
          locked = false;
          break;
        }
        if(countlow > MinLongCount)  //is this a double 0
        {
          AddManBit(&manBits, &numMB, &curByte, data, 0);
        }//end of we have a double 0
      }//end of read the raw RX input

      if (curByte > 0)
      {
        // We successfully received at least one byte, 
        // stop looping and return it to the user
        break;
      }
    }//end of its locked
  }//end of look until find data or timeout

  // Return number of bytes received.
  return curByte;
}//end of receive data

void MANCHESTERClass::SetTimeOut(unsigned int timeout)
{
  TimeOut = timeout;  //user sets timeout 
}//end of user sets timeout

void MANCHESTERClass::SetRxPin(char pin)
{
  RxPin = pin;
  pinMode(pin, INPUT);      // user sets the digital pin as input  
}//end of set transmit pin	

void MANCHESTERClass::SetTxPin(char pin)
{
  TxPin = pin;      // user sets the digital pin as output
  pinMode(TxPin, OUTPUT);      // sets the digital pin 4 default as output   
}//end of set transmit pin	

void MANCHESTERClass::Transmit(unsigned int data)
{
  unsigned char byteData[2] = {data >> 8, data & 0xFF};
  TransmitBytes(2, byteData);
}

void MANCHESTERClass::TransmitBytes(unsigned char numBytes, unsigned char *data)
{
  // Setup last send time so we start transmitting in 10us
  lastSend = micros() - HALF_BIT_INTERVAL + 10;

  // Send 14 0's
  for( int i = 0; i < 14; i++) //send capture pulses
    sendzero();  //end of capture pulses
 
  // Send a single 1
  sendone();  //start data pulse
 
  // Send the user data
  for (unsigned char i = 0; i < numBytes; i++)
  {
    unsigned int mask = 0x01; //mask to send bits 
    for (char j = 0; j < 8; j++)
    {
      if ((data[i] & mask) == 0)
        sendzero();
      else
        sendone();
      mask = mask << 1; //get next bit
    }//end of byte
  }//end of data

  // Send 2 terminatings 0's
  sendzero();
  sendzero();  
}//end of send the data	

void MANCHESTERClass::sendzero(void)
{
  delayMicroseconds(lastSend + HALF_BIT_INTERVAL - micros());        
  digitalWrite(TxPin, HIGH);   

  delayMicroseconds(HALF_BIT_INTERVAL);        
  digitalWrite(TxPin, LOW);
 
  lastSend = micros();
}//end of send a zero

void MANCHESTERClass::sendone(void)
{
  delayMicroseconds(lastSend + HALF_BIT_INTERVAL - micros());        
  digitalWrite(TxPin, LOW);   

  delayMicroseconds(HALF_BIT_INTERVAL);        
  digitalWrite(TxPin, HIGH);
 
  lastSend = micros();
}//end of send one

MANCHESTERClass MANCHESTER;
