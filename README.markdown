This is a [Manchester encoding](http://en.wikipedia.org/wiki/Manchester_code) RF library which works on Arduino and ATTiny.

Note: I am currently using this library with IDE 1.0.

Library supports:

various microcontrollers 
* ATmega328
* ATmega8
* ATMega32U4
* ATtiny84 (44, 24)
* ATtiny85 (45, 85)

various microcontroller's speed both RX and TX
* 1Mhz (see notes for TX)
* 8Mhz
* 16Mhz
* 16.5Mhz ATtiny85

various transmission speeds
* 600baud
* 1200baud
* 2400baud
* 4800baud
* 9600baud
* 19200baud

You can sucessfully transmit between various microcontrollers runnning at various speeds with internal (inaccurate) clock.
Your maximum transmission speed will depend on many factors, transmitter, antenna, distance, enviroment noise, 
microcontroller type and speed

Getting started on Arduino
------

1) Unzip content of this folder to 

    workspace_root/libraries/Manchester
    
2) Import the library from your Arduino sketch

    #include <Manchester.h>
    
3) Modify your setup() method.

For sending data

    void setup()
    {
      man.setupTransmit(TX_PIN, MAN_1200);
    }

For receiving data
    
    void setup()
    {
      man.setupReceive(RX_PIN, MAN_1200);
      man.beginReceive();
    }
    
4) Modify your main loop

For sending data

    void loop() {
      man.transmit(transmit_data);
    }
  
For receiving data

    void loop() {
      if (man.receiveComplete()) {
        uint16_t m = man.getMessage();
        man.beginReceive(); //start listening for next message right after you retrieve the message
        //do something with your message here
      }
    }  
  
5) For more advanced uses like transmitting with checksum and sender/receiver address or sending array of bytes see attached examples  
    
Getting started on ATtiny85 and ATtiny84
------

This is broadly the same as on Arduino with one extra step. You need to download the 
[ATtiny hardware files](https://github.com/downloads/mchr3k/arduino-libs-manchester/tiny.zip) (a list of
alternative ATtiny implementations is listed on [this site](http://code.google.com/p/arduino-tiny/) which
is where I got the files in tiny.zip).
These should be unzipped into the following location.

    workspace_root/hardware/tiny
    
As of now (May 2013) most of tiny cores does not support micro delay at 1Mhz and for that reason
if you want to TRANSMIT (receive is OK) from your 1Mhz ATtiny85/84 you need to call this method
before you call transmit setup:

    man.workAround1MhzTinyCore(); //add this in order for transmitter to work with 1Mhz Attiny85/84
    man.setupTransmit(TX_PIN, MAN_1200);

Credits
------

* Library originally from [carl47](http://arduino.cc/forum/index.php?action=profile;u=14566) on 
[this thread](http://arduino.cc/forum/index.php/topic,63755.0.html)
* Contributions from [mchr3k](http://mchr3k-arduino.blogspot.com/), 
[Mike](https://github.com/MichaelBell/Arduino-sketches) and
[Cano](https://github.com/cano64)
