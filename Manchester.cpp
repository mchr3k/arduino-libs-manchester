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

This code gives a basic data rate as 1200 bauds. In manchester encoding we send 1 0 for a data bit 0.
We send 0 1 for a data bit 1. This ensures an average over time of a fixed DC level in the TX/RX.
This is required by the ASK RF link system to ensure its correct operation.
The data rate is then 600 bits/s.
*/

#include "Manchester.h"

static int8_t RxPin = 255;

static int16_t rx_sample = 0;
static int16_t rx_last_sample = 0;
static uint8_t rx_count = 0;
static uint8_t rx_sync_count = 0;
static uint8_t rx_mode = RX_MODE_IDLE;

static uint16_t rx_manBits = 0; //the received manchester 32 bits
static uint8_t rx_numMB = 0; //the number of received manchester bits
static uint8_t rx_curByte = 0;

static uint8_t rx_maxBytes = 2;
static uint8_t rx_default_data[2];
static uint8_t* rx_data = rx_default_data;

Manchester::Manchester() //constructor
{
  applyWorkAround1Mhz = 0;
}


void Manchester::setTxPin(uint8_t pin)
{
  TxPin = pin; // user sets the digital pin as output
  pinMode(TxPin, OUTPUT); 
}


void Manchester::setRxPin(uint8_t pin)
{
  ::RxPin = pin; // user sets the digital pin as output
  pinMode(::RxPin, INPUT); 
}

void Manchester::workAround1MhzTinyCore(uint8_t a)
{
  applyWorkAround1Mhz = a;
}

void Manchester::setupTransmit(uint8_t pin, uint8_t SF)
{
  setTxPin(pin);
  speedFactor = SF;
  //we don't use exact calculation of passed time spent outside of transmitter
  //because of high ovehead associated with it, instead we use this 
  //emprirically determined values to compensate for the time loss
  
  #if F_CPU == 1000000UL
    uint16_t compensationFactor = 88; //must be divisible by 8 for workaround
  #elif F_CPU == 8000000UL
    uint16_t compensationFactor = 12; 
  #else //16000000Mhz
    uint16_t compensationFactor = 4; 
  #endif  

  delay1 = (HALF_BIT_INTERVAL >> speedFactor) - compensationFactor;
  delay2 = (HALF_BIT_INTERVAL >> speedFactor) - 2;
  
  #if F_CPU == 1000000UL
    delay2 -= 22; //22+2 = 24 is divisible by 8
    if (applyWorkAround1Mhz) { //definition of micro delay is broken for 1MHz speed in tiny cores as of now (May 2013)
      //this is a workaround that will allow us to transmit on 1Mhz
      //divide the wait time by 8
      delay1 >>= 3;
      delay2 >>= 3;
    }
  #endif  
}


void Manchester::setupReceive(uint8_t pin, uint8_t SF)
{
  setRxPin(pin);
  ::MANRX_SetupReceive(SF);
}


void Manchester::setup(uint8_t Tpin, uint8_t Rpin, uint8_t SF)
{
  setupTransmit(Tpin, SF);
  setupReceive(Rpin, SF);
}


void Manchester::transmit(uint16_t data)
{
  uint8_t byteData[2] = {data >> 8, data & 0xFF};
  transmitArray(2, byteData);
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
void Manchester::transmitArray(uint8_t numBytes, uint8_t *data)
{
  // Send 14 0's
  for( int16_t i = 0; i < 14; i++) //send capture pulses
    sendZero(); //end of capture pulses
 
  // Send a single 1
  sendOne(); //start data pulse
 
  // Send the user data
  for (uint8_t i = 0; i < numBytes; i++)
  {
    uint16_t mask = 0x01; //mask to send bits
    uint8_t d = data[i] ^ DECOUPLING_MASK;
    for (uint8_t j = 0; j < 8; j++)
    {
      if ((d & mask) == 0)
        sendZero();
      else
        sendOne();
      mask <<= 1; //get next bit
    }//end of byte
  }//end of data

  // Send 2 terminatings 0's to correctly terminate the previous bit and to turn the transmitter off
  sendZero(); 
  sendZero();
}//end of send the data


void Manchester::sendZero(void)
{
  delayMicroseconds(delay1);
  digitalWrite(TxPin, HIGH);

  delayMicroseconds(delay2);
  digitalWrite(TxPin, LOW);
}//end of send a zero


void Manchester::sendOne(void)
{
  delayMicroseconds(delay1);
  digitalWrite(TxPin, LOW);

  delayMicroseconds(delay2);
  digitalWrite(TxPin, HIGH);
}//end of send one

//TODO use repairing codes perhabs?
//http://en.wikipedia.org/wiki/Hamming_code

/*
    format of the message including checksum and ID
    
    [0][1][2][3][4][5][6][7][8][9][a][b][c][d][e][f]
    [    ID    ][ checksum ][         data         ]      
                  checksum = ID xor data[7:4] xor data[3:0] xor 0b0011
                  
*/

//decode 8 bit payload and 4 bit ID from the message, return true if checksum is correct, otherwise false
uint8_t Manchester::decodeMessage(uint16_t m, uint8_t &id, uint8_t &data)
{
  //extract components
  data = (m & 0xFF);
  id = (m >> 12);
  uint8_t ch = (m >> 8) & 0b1111; //checksum received
  //calculate checksum
  uint8_t ech = (id ^ data ^ (data >> 4) ^ 0b0011) & 0b1111; //checksum expected
  return ch == ech;
}

//encode 8 bit payload, 4 bit ID and 4 bit checksum into 16 bit
uint16_t Manchester::encodeMessage(uint8_t id, uint8_t data)
{
  uint8_t chsum = (id ^ data ^ (data >> 4) ^ 0b0011) & 0b1111;
  uint16_t m = ((id) << 12) | (chsum << 8) | (data);
  return m;
}

void Manchester::beginReceiveArray(uint8_t maxBytes, uint8_t *data)
{
  ::MANRX_BeginReceiveBytes(maxBytes, data);
}

void Manchester::beginReceive(void)
{
  ::MANRX_BeginReceive();
}


uint8_t Manchester::receiveComplete(void)
{
  return ::MANRX_ReceiveComplete();
}


uint16_t Manchester::getMessage(void)
{
  return ::MANRX_GetMessage();
}


void Manchester::stopReceive(void)
{
  ::MANRX_StopReceive();
}

//global functions

void MANRX_SetupReceive(uint8_t speedFactor)
{
  pinMode(RxPin, INPUT);
  //setup timers depending on the microcontroller used

  #if defined( __AVR_ATtinyX5__ )

    /*
    Timer 1 is used with a ATtiny85. 
    http://www.atmel.com/Images/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf page 88
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR1C is 8 bit register
    */

    #if F_CPU == 1000000UL
      TCCR1 = _BV(CTC1) | _BV(CS12); // 1/8 prescaler
      OCR1C = (64 >> speedFactor) - 1; 
    #elif F_CPU == 8000000UL
      TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1C = (64 >> speedFactor) - 1; 
    #elif F_CPU == 16000000UL
      TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1C = (128 >> speedFactor) - 1; 
    #elif F_CPU == 16500000UL     
      TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1C = (132 >> speedFactor) - 1; 
    #else
    #error "Manchester library only supports 1mhz, 8mhz, 16mhz, 16.5Mhz clock speeds on ATtiny85 chip"
    #endif
    
    OCR1A = 0; // Trigger interrupt when TCNT1 is reset to 0
    TIMSK |= _BV(OCIE1A); // Turn on interrupt
    TCNT1 = 0; // Set counter to 0

  #elif defined( __AVR_ATtinyX313__ )

    /*
    Timer 1 is used with a ATtiny2313. 
    http://www.atmel.com/Images/doc2543.pdf page 107
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR1A/B are 8 bit registers
    */

    #if F_CPU == 1000000UL
      TCCR1A = 0;
      TCCR1B = _BV(WGM12) | _BV(CS11); // reset counter on match, 1/8 prescaler
      OCR1A = (64 >> speedFactor) - 1; 
    #elif F_CPU == 8000000UL
      TCCR1B = _BV(WGM12) | _BV(CS12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1A = (64 >> speedFactor) - 1; 
    #else
    #error "Manchester library only supports 1mhz, 8mhz clock speeds on ATtiny2313 chip"
    #endif
    
    OCR1B = 0; // Trigger interrupt when TCNT1 is reset to 0
    TIMSK |= _BV(OCIE1B); // Turn on interrupt
    TCNT1 = 0; // Set counter to 0

  #elif defined( __AVR_ATtiny84__ )

    /*
    Timer 1 is used with a ATtiny84. 
    http://www.atmel.com/Images/doc8006.pdf page 111
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR1A is 8 bit register
    */

	TCCR1A = 0;

    #if F_CPU == 1000000UL
      TCCR1B = _BV(WGM12) | _BV(CS11); // 1/8 prescaler
      OCR1A = (64 >> speedFactor) - 1; 
    #elif F_CPU == 8000000UL
      TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1A = (64 >> speedFactor) - 1;
    #elif F_CPU == 16000000UL
      TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10); // 1/64 prescaler
      OCR1A = (128 >> speedFactor) - 1; 
    #else
    #error "Manchester library only supports 1mhz, 8mhz, 16mhz on ATtiny84"
    #endif
    
    TIMSK1 |= _BV(OCIE1A); // Turn on interrupt
    TCNT1 = 0; // Set counter to 0

  #elif defined(__AVR_ATmega32U4__)

    /*
    Timer 3 is used with a ATMega32U4. 
    http://www.atmel.com/Images/doc7766.pdf page 133
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR3A is 16 bit register
    */
    
    TCCR3B = _BV(WGM32) | _BV(CS31); // 1/8 prescaler
    #if F_CPU == 1000000UL
      OCR3A = (64 >> speedFactor) - 1; 
    #elif F_CPU == 8000000UL
      OCR3A = (512 >> speedFactor) - 1; 
    #elif F_CPU == 16000000UL
      OCR3A = (1024 >> speedFactor) - 1; 
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
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR1A is 16 bit register
    */

    TCCR1A = _BV(WGM12); // reset counter on match
    TCCR1B =  _BV(CS11); // 1/8 prescaler
    #if F_CPU == 1000000UL
      OCR1A = (64 >> speedFactor) - 1; 
    #elif F_CPU == 8000000UL
      OCR1A = (512 >> speedFactor) - 1; 
    #elif F_CPU == 16000000UL
      OCR1A = (1024 >> speedFactor) - 1; 
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
    How to find the correct value: (OCRxA +1) = F_CPU / prescaler / 1953.125
    OCR2A is only 8 bit register
    */

    TCCR2A = _BV(WGM21); // reset counter on match
    #if F_CPU == 1000000UL
      TCCR2B = _BV(CS21); // 1/8 prescaler
      OCR2A = (64 >> speedFactor) - 1;
    #elif F_CPU == 8000000UL
      TCCR2B = _BV(CS21) | _BV(CS20); // 1/32 prescaler
      OCR2A = (128 >> speedFactor) - 1; 
    #elif F_CPU == 16000000UL
      TCCR2B = _BV(CS22); // 1/64 prescaler
      OCR2A = (128 >> speedFactor) - 1; 
    #else
    #error "Manchester library only supports 8mhz, 16mhz on ATMega328"
    #endif
    TIMSK2 = _BV(OCIE2A); // Turn on interrupt
    TCNT2 = 0; // Set counter to 0
  #endif

} //end of setupReceive

void MANRX_BeginReceive(void)
{
  rx_maxBytes = 2;
  rx_data = rx_default_data;
  rx_mode = RX_MODE_PRE;
}

void MANRX_BeginReceiveBytes(uint8_t maxBytes, uint8_t *data)
{
  rx_maxBytes = maxBytes;
  rx_data = data;
  rx_mode = RX_MODE_PRE;
}

void MANRX_StopReceive(void)
{
  rx_mode = RX_MODE_IDLE;
}

uint8_t MANRX_ReceiveComplete(void)
{
  return (rx_mode == RX_MODE_MSG);
}

uint16_t MANRX_GetMessage(void)
{
  return (((int16_t)rx_data[0]) << 8) | (int16_t)rx_data[1];
}


void MANRX_SetRxPin(uint8_t pin)
{
  RxPin = pin;
  pinMode(RxPin, INPUT);
}//end of set transmit pin

void AddManBit(uint16_t *manBits, uint8_t *numMB,
               uint8_t *curByte, uint8_t *data,
               uint8_t bit)
{
  *manBits <<= 1;
  *manBits |= bit;
  (*numMB)++;
  if (*numMB == 16)
  {
    uint8_t newData = 0;
    for (int8_t i = 0; i < 8; i++)
    {
      // ManBits holds 16 bits of manchester data
      // 1 = LO,HI
      // 0 = HI,LO
      // We can decode each bit by looking at the bottom bit of each pair.
      newData <<= 1;
      newData |= (*manBits & 1); // store the one
      *manBits = *manBits >> 2; //get next data bit
    }
    data[*curByte] = newData ^ DECOUPLING_MASK;
    (*curByte)++;
    *numMB = 0;
  }
}

#if defined( __AVR_ATtinyX5__ )
ISR(TIMER1_COMPA_vect)
#elif defined( __AVR_ATtinyX313__ )
ISR(TIMER1_COMPB_vect)
#elif defined( __AVR_ATtiny84__ )
ISR(TIM1_COMPA_vect)
#elif defined(__AVR_ATmega32U4__)
ISR(TIMER3_COMPA_vect)
#else
ISR(TIMER2_COMPA_vect)
#endif
{
  if (rx_mode < RX_MODE_MSG) //receiving something
  {
    // Increment counter
    rx_count += 8;
    
    // Check for value change
    rx_sample = digitalRead(RxPin);
    uint8_t transition = (rx_sample != rx_last_sample);
  
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
             (rx_count >= MinLongCount))
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
          // wrong signal lenght, discard the message
          rx_mode = RX_MODE_PRE;
        }
        else
        {
          if(rx_count >= MinLongCount) // was the previous bit a double bit?
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

Manchester man;
