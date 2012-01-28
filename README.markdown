This is a Manchester encoding RF library which works on Arduino and ATTiny.

Warning: This lib has only been tested with Arduino IDE 0022.

Getting started on Arduino
------

1) Download MANCHESTER.cpp and MANCHESTER.h and save these in the following location

    workspace_root/libraries/Manchester
    
2) Import the library from your Arduino sketch

    #include <MANCHESTER.h>
    
3) Modify your setup() method.

For receiving data:

    #define TxPin 4
    void setup()
    {
      MANCHESTER.SetTxPin(TxPin);
    }

For sending data:

    #define RxPin 4
    void setup() 
    {  
      MANCHESTER.SetRxPin(RxPin);
      MANCHESTER.SetTimeOut(1000);
    }

4) Modify your main code.

Sending 16 bits of data:

    unsigned int data = 1234;
    MANCHESTER.Transmit(data);
    
Sending an array of 8 bit values:

    char[] data = new char[3];
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    MANCHESTER.TransmitBytes(3, &data);
    
Receive 16 bits of data:

    unsigned int data = MANCHESTER.Receive();
    
Receive an array of 8 bit values:

    char[] data = new char[3];
    MANCHESTER.ReceiveBytes(3, &data);
    
Getting started on ATtiny85
------

This is broadly the same as on Arduino with one extra step. You need to download the [ATtiny hardware files](https://github.com/downloads/mchr3k/arduino-libs-manchester/attiny45_85.zip). These should be unzipped into the following location.

    workspace_root/hardware/attiny45_85

Credits
------

* Library originally from [carl47](http://arduino.cc/forum/index.php?action=profile;u=14566) on [this thread](http://arduino.cc/forum/index.php/topic,63755.0.html)
* Contributions from [mchr3k](http://mchr3k-arduino.blogspot.com/) and [Mike](https://github.com/MichaelBell/Arduino-sketches)
