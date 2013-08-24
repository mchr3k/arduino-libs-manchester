<<<<<<< HEAD
Full details available on the [Arduino Manchester Encoding site](http://mchr3k.github.com/arduino-libs-manchester/)
=======
This is a [Manchester encoding](http://en.wikipedia.org/wiki/Manchester_code) RF library which works on Arduino and ATTiny.

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
* 300baud
* 600baud
* 1200baud
* 2400baud
* 4800baud
* 9600baud
* 19200baud

With this library you can sucessfully transmit data between various microcontrollers 
runnning at various speeds even if their clock speed varies up to 100%. 
Works great with innacurate internal oscilator.
Your maximum transmission speed will depend on many factors like transmitter type, 
antenna, distance, enviroment noise, microcontroller type and speed

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
    
As of now (June 2013) most of tiny cores does not support micro delay at 1Mhz and for that reason
if you want to TRANSMIT (receive is OK) from your 1Mhz ATtiny85/84 you need to call this method
before you call transmit setup:

    man.workAround1MhzTinyCore(); //add this in order for transmitter to work with 1Mhz Attiny85/84
    man.setupTransmit(TX_PIN, MAN_1200);

Notes about transmitters
------

The library has been tested with common 315Mhz and 433Mhz transmitters using ASK OOK.
Tips to improve your transmit distance:
Attaching an antenna will greatly improve transmit distance (from few cm to few m)
for 315Mhz use 23.81 cm straight wire, for 433Mhz use 17.28cm straight wire as antenna.
If possible keep the wire straight, away from ground wire, and away from the rest of the circuit
and battery pack.
Transmitter can use anything from 3.3V to 12V. Increasing voltage will increase transmit
distance. If you are using voltage regulator, attach transmitter directly to the battery.
Receiver needs 5V, it doesn't work on 3.3V

Speed: I was able to achieve 9600 bauds between two 16Mhz Arduinos, 
or 2400 bauds between two 1Mhz ATTiny85, you can try different speeds to see which works 
the best for you.

Full duplex: for bidirectional communication use both 315Mhz and 433Mhz transmitters.
This way they can transmit at the same time without interfering with each other. If you have 
just one type, wait for receiver to finish receiving before transmitting. 

Credits
------

* Library originally from [carl47](http://arduino.cc/forum/index.php?action=profile;u=14566) on 
[this thread](http://arduino.cc/forum/index.php/topic,63755.0.html)
* Contributions from [mchr3k](http://mchr3k-arduino.blogspot.com/), 
[Mike](https://github.com/MichaelBell/Arduino-sketches) and
[Cano](https://github.com/cano64)
>>>>>>> 837339bb145f925d296b4031b037a9449168553c
