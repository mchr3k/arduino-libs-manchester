This is a Manchester encoding RF library which works on Arduino and ATTiny.

Note: I am currently using this library with IDE 1.0.

Getting started on Arduino
------

1) Download MANCHESTER.cpp and MANCHESTER.h and save these in the following location

    workspace_root/libraries/Manchester
    
2) Import the library from your Arduino sketch

    #include <MANCHESTER.h>
    
3) Modify your setup() method.

For receiving 16 bits of data at a time:

    void setup()
    {
      // Set digital TX pin
      MANRX_SetRxPin(4);
      // Prepare interrupts
      MANRX_SetupReceive();
      // Begin receiving data
      MANRX_BeginReceive();
    }

For receiving arrays of 8 bit values:
    
    unsigned char bufferSize = 10;
    unsigned char bufferData[bufferSize];
    void setup()
    {
      // Set digital TX pin
      MANRX_SetRxPin(4);
      // Prepare interrupts
      MANRX_SetupReceive();
      // Begin receiving data
      MANRX_BeginReceiveBytes(bufferSize, bufferData);
    }
    
For sending data:

    void setup() 
    {  
      MANCHESTER.SetRxPin(4);
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

    if (MANRX_ReceiveComplete())
    {
      unsigned int data = MANRX_GetMessage();
      MANRX_BeginReceive();
      // Handle data...
    }
    
Receive an array of 8 bit values:

    if (MANRX_ReceiveComplete())
    {
      unsigned char receivedSize = 0;
      MANRX_GetMessageBytes(&receivedSize, &bufferData);
      // Handle/copy data...
      MANRX_BeginReceiveBytes(bufferSize, bufferData);
    }
    
Getting started on ATtiny85
------

This is broadly the same as on Arduino with one extra step. You need to download the 
[ATtiny hardware files](https://github.com/downloads/mchr3k/arduino-libs-manchester/tiny.zip) (a list of
alternative ATtiny implementations is listed on [this site](http://code.google.com/p/arduino-tiny/) which
is where I got the files in tiny.zip).
These should be unzipped into the following location.

    workspace_root/hardware/attiny45_85

Credits
------

* Library originally from [carl47](http://arduino.cc/forum/index.php?action=profile;u=14566) on 
[this thread](http://arduino.cc/forum/index.php/topic,63755.0.html)
* Contributions from [mchr3k](http://mchr3k-arduino.blogspot.com/) and 
[Mike](https://github.com/MichaelBell/Arduino-sketches)
