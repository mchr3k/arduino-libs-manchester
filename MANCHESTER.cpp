/*
This code is based on the Atmel Corporation Manchester
Coding Basics Application Note.

http://www.atmel.com/dyn/resources/prod_documents/doc9164.pdf

Quotes from the application note:

"Manchester coding states that there will always be a transition of the message signal
at the mid-point of the data bit frame.
What occurs at the bit edges depends on the state of the previous bit frame and
does not always produce a transition. A logical '1' is defined as a mid-point transition
from low to high and a '0' is a mid-point transition from high to low.

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

#define HALF_BIT_INTERVAL 769 // microseconds

MANCHESTERClass::MANCHESTERClass() //constructor
{
  TxPin = TxDefault;
  speedFactor = MAN_1200;
  //following needs to be moved to the begin method, if somebody is using default pin for something else you may short circuit something
  pinMode(TxPin, OUTPUT); // sets the digital pin 4 default as output
}//end of constructor

void MANCHESTERClass::SetTxPin(char pin)
{
  TxPin = pin; // user sets the digital pin as output
  pinMode(TxPin, OUTPUT); // sets the digital pin 4 default as output
}//end of set transmit pin

void MANCHESTERClass::begin(unsigned char SF)
{
  speedFactor = SF;
}

void MANCHESTERClass::Transmit(unsigned int data)
{
  unsigned char byteData[2] = {data >> 8, data & 0xFF};
  TransmitBytes(2, byteData);
}

/*
The 433.92 Mhz receivers have AGC, if no signal is present the gain will be set
to its highest level.

In this condition it will switch high to low at random intervals due to input noise.
A CRO connected to the data line looks like 433.92 is full of transmissions.

Any ASK transmission method must first sent a capture signal of 101010........
When the receiver has adjusted its AGC to the required level for the transmisssion
the actual data transmission can occur.

We send 14 0's 1010... It takes 1 to 3 10's for the receiver to adjust to
the transmit level.

The receiver waits until we have at least 10 10's and then a start pulse 01.
The receiver is then operating correctly and we have locked onto the transmission.
*/
void MANCHESTERClass::TransmitBytes(unsigned char numBytes, unsigned char *data)
{
  // Setup last send time so we start transmitting in 10us
  lastSend = micros() - (HALF_BIT_INTERVAL >> speedFactor) + 10;

  // Send 14 0's
  for( int i = 0; i < 14; i++) //send capture pulses
    sendzero(); //end of capture pulses
 
  // Send a single 1
  sendone(); //start data pulse
 
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
  long time = micros();
  if (time < lastSend) lastSend = time; //in case we overflowed
  long waitUntil = lastSend + (HALF_BIT_INTERVAL >> speedFactor);
  if (waitUntil > time) delayMicroseconds(waitUntil - time);
  digitalWrite(TxPin, HIGH);

  delayMicroseconds((HALF_BIT_INTERVAL >> speedFactor));
  digitalWrite(TxPin, LOW);
 
  lastSend = micros();
}//end of send a zero

void MANCHESTERClass::sendone(void)
{
  long time = micros();
  if (time < lastSend) lastSend = time; //in case we overflowed
  long waitUntil = lastSend + (HALF_BIT_INTERVAL >> speedFactor);
  if (waitUntil > time) delayMicroseconds(waitUntil - time);
  digitalWrite(TxPin, LOW);

  delayMicroseconds((HALF_BIT_INTERVAL >> speedFactor));
  digitalWrite(TxPin, HIGH);
 
  lastSend = micros();
}//end of send one

static char RxPin = 4;

static int rx_sample = 0;
static int rx_last_sample = 0;
static uint8_t rx_count = 0;
static uint8_t rx_sync_count = 0;
static uint8_t rx_mode = RX_MODE_IDLE;

static unsigned int rx_manBits = 0; //the received manchester 32 bits
static unsigned char rx_numMB = 0; //the number of received manchester bits
static unsigned char rx_curByte = 0;

static unsigned char rx_maxBytes = 2;
static unsigned char rx_default_data[2];
static unsigned char* rx_data = rx_default_data;

void MANRX_SetupReceive(unsigned char speedFactor)
{
  pinMode(RxPin, INPUT);

/*
This code gives a basic data rate as 1200 bauds. In manchester encoding we send 1 0 for a data bit 0.
We send 0 1 for a data bit 1. This ensures an average over time of a fixed DC level in the TX/RX.
This is required by the ASK RF link system to ensure its correct operation.
The data rate is then 600 bits/s.

*/

#if defined( __AVR_ATtinyX5__ )

  /*
  Timer 1 is used with a ATtiny85. 
  http://www.atmel.com/Images/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf page 88
  How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 15625 / 4
  */

  #if F_CPU == 1000000UL
    TCCR1 = _BV(CTC1) | _BV(CS12); // 1/8 prescaler
    OCR1C = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 8000000UL
    TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
    OCR1C = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 16000000UL
    TCCR1 = _BV(CTC1) | _BV(CS13); // 1/128 prescaler
    OCR1C = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 16500000UL     
    TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS11); // 1/32 prescaler
    OCR1C = (132 >> speedFactor) - 1; // interrupt every 132 counts for base speed
  #else
  #error "Manchester library only supports 1mhz, 8mhz, 16mhz, 16.5Mhz clock speeds on ATtiny85 chip"
  #endif
  
  OCR1A = 0; // Trigger interrupt when TCNT1 is reset to 0
  TIMSK |= _BV(OCIE1A); // Turn on interrupt
  TCNT1 = 0; // Set counter to 0

#elif defined( __AVR_ATtinyX4__ )

  /*
  Timer 1 is used with a ATtiny84. 
  http://www.atmel.com/Images/doc8006.pdf page 111
  How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 15625 / 4
  */

  #if F_CPU == 1000000UL
    TCCR1B = _BV(WGM12) | _BV(CS11); // 1/8 prescaler
    OCR1A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 8000000UL
    TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
    OCR1A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 16000000UL
    TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
    OCR1A = (64 >> speedFactor) - 1; // interrupt every 64 counts for base speed
  #else
  #error "Manchester library only supports 1mhz, 8mhz, 16mhz on ATtiny84"
  #endif
  
  TIMSK1 |= _BV(OCIE1A); // Turn on interrupt
  TCNT1 = 0; // Set counter to 0

#elif defined(__AVR_ATmega32U4__)

  /*
  Timer 3 is used with a ATMega32U4. 
  http://www.atmel.com/Images/doc7766.pdf page 133
  How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 15625 / 4
  */
  
  #if F_CPU == 1000000UL
    TCCR3B = _BV(WGM32) | _BV(CS31); // 1/8 prescaler
    OCR3A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 8000000UL
    TCCR3B = _BV(WGM32) | _BV(CS31) | _BV(CS30); // 1/64 prescaler
    OCR3A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 16000000UL
    TCCR3B = _BV(WGM32) | _BV(CS31) | _BV(CS30); // 1/64 prescaler
    OCR3A = (64 >> speedFactor) - 1; // interrupt every 64 counts for base speed
  #else
  #error "Manchester library only supports 1mhz, 8mhz, 16mhz on ATMega32U4"
  #endif
  
  TCCR3A = 0; // reset counter on match
  TIFR3 = _BV(OCF3A); // clear interrupt flag
  TIMSK3 = _BV(OCIE3A); // Turn on interrupt
  TCNT3 = 0; // Set counter to 0

#elif defined(__AVR_ATmega8__)

  /* 
  Timer/counter 1 is used with ATmega8. 
  http://www.atmel.com/Images/Atmel-2486-8-bit-AVR-microcontroller-ATmega8_L_datasheet.pdf page 99
  How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 15625 / 4
  */

  TCCR1A = _BV(WGM12); // reset counter on match
  #if F_CPU == 1000000UL
    TCCR1B =  _BV(CS11); // 1/8 prescaler
    OCR1A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 8000000UL
    TCCR1B =  _BV(CS11) | _BV(CS10); // 1/64 prescaler
    OCR1A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 16000000UL
    TCCR1B =  _BV(CS11) | _BV(CS10); // 1/64 prescaler
    OCR1A = (64 >> speedFactor) - 1; // interrupt every 64 counts for base speed
  #else
	#error "Manchester library only supports 1Mhz, 8mhz, 16mhz on ATMega8"
  #endif
  TIFR = _BV(OCF1A);  // clear interrupt flag
  TIMSK = _BV(OCIE1A); // Turn on interrupt
  TCNT1 = 0; // Set counter to 0

#else // ATmega328 is a default microcontroller


  /*
  Timer 2 is used with a ATMega328.
  http://www.atmel.com/dyn/resources/prod_documents/doc8161.pdf page 162
  How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 15625 / 4
  */

  TCCR2A = _BV(WGM21); // reset counter on match
  #if F_CPU == 1000000UL
    TCCR2B = _BV(CS21); // 1/8 prescaler
    OCR2A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 8000000UL
    TCCR2B = _BV(CS22); // 1/64 prescaler
    OCR2A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #elif F_CPU == 16000000UL
    TCCR2B = _BV(CS22) | _BV(CS20); // 1/128 prescaler
    OCR2A = (32 >> speedFactor) - 1; // interrupt every 32 counts for base speed
  #else
  #error "Manchester library only supports 8mhz, 16mhz on ATMega328"
  #endif
  TIMSK2 = _BV(OCIE2A); // Turn on interrupt
  TCNT2 = 0; // Set counter to 0
#endif

}

void MANRX_BeginReceive(void)
{
  rx_maxBytes = 2;
  rx_data = rx_default_data;
  rx_mode = RX_MODE_PRE;
}

void MANRX_BeginReceiveBytes(unsigned char maxBytes, unsigned char *data)
{
  rx_maxBytes = maxBytes;
  rx_data = data;
  rx_mode = RX_MODE_PRE;
}

void MANRX_StopReceive(void)
{
  rx_mode = RX_MODE_IDLE;
}

boolean MANRX_ReceiveComplete(void)
{
  return (rx_mode == RX_MODE_MSG);
}

unsigned int MANRX_GetMessage(void)
{
  return (((int)rx_data[0]) << 8) | (int)rx_data[1];
}

void MANRX_GetMessageBytes(unsigned char *rcvdBytes, unsigned char **data)
{
  *rcvdBytes = rx_curByte;
  *data = rx_data;
}

void MANRX_SetRxPin(char pin)
{
  RxPin = pin;
  pinMode(RxPin, INPUT);
}//end of set transmit pin

void AddManBit(unsigned int *manBits, unsigned char *numMB,
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
#if defined( __AVR_ATtinyX5__ )
ISR(TIMER1_COMPA_vect)
#elif defined( __AVR_ATtinyX4__ )
ISR(TIM1_COMPA_vect)
#elif defined(__AVR_ATmega32U4__)
ISR(TIMER3_COMPA_vect)
#else
ISR(TIMER2_COMPA_vect)
#endif
{
  if (rx_mode < 3)
  {
    // Increment counter
    rx_count += 16;
    
    // Check for value change
    rx_sample = digitalRead(RxPin);
    boolean transition = (rx_sample != rx_last_sample);
  
    if (rx_mode == RX_MODE_PRE)
    {
      // Wait for first transition to HIGH
      if (transition && (rx_sample == 1))
      {
        rx_count = 0;
        rx_sync_count = 0;
        rx_mode = RX_MODE_SYNC;
      }
    }
    else if (rx_mode == RX_MODE_SYNC)
    {
      // Initial sync block
      if (transition)
      {
        if(((rx_sync_count < 20) || (rx_last_sample == 1)) &&
           ((rx_count < MinCount) || (rx_count > MaxCount)))
        {
          // First 20 bits and all 1 bits are expected to be regular
          // Transition was too slow/fast
          rx_mode = RX_MODE_PRE;
        }
        else if((rx_last_sample == 0) &&
                ((rx_count < MinCount) || (rx_count > MaxLongCount)))
        {
          // 0 bits after the 20th bit are allowed to be a double bit
          // Transition was too slow/fast
          rx_mode = RX_MODE_PRE;
        }
        else
        {
          rx_sync_count++;
          
          if((rx_last_sample == 0) &&
             (rx_sync_count >= 20) &&
             (rx_count > MinLongCount))
          {
            // We have seen at least 10 regular transitions
            // Lock sequence ends with unencoded bits 01
            // This is encoded and TX as HI,LO,LO,HI
            // We have seen a long low - we are now locked!
            rx_mode = RX_MODE_DATA;
            rx_manBits = 0;
            rx_numMB = 0;
            rx_curByte = 0;
          }
          else if (rx_sync_count >= 32)
          {
            rx_mode = RX_MODE_PRE;
          }
          rx_count = 0;
        }
      }
    }
    else if (rx_mode == RX_MODE_DATA)
    {
      // Receive data
      if (transition)
      {
        if((rx_count < MinCount) ||
           (rx_count > MaxLongCount))
        {
          // Interference - give up
          rx_mode = RX_MODE_PRE;
        }
        else
        {
          if(rx_count > MinLongCount) // was the previous bit a double bit?
          {
            AddManBit(&rx_manBits, &rx_numMB, &rx_curByte, rx_data, rx_last_sample);
          }
          if ((rx_sample == 1) &&
              (rx_curByte >= rx_maxBytes))
          {
            rx_mode = RX_MODE_MSG;
          }
          else
          {
            // Add the current bit
            AddManBit(&rx_manBits, &rx_numMB, &rx_curByte, rx_data, rx_sample);
            rx_count = 0;
          }
        }
      }
    }
    
    // Get ready for next loop
    rx_last_sample = rx_sample;
  }
}

MANCHESTERClass MANCHESTER;
