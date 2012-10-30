
#include <MANCHESTER.h>

unsigned char bufferSize = 1;
unsigned char *bufferData;

int whichLight;
int lightCount;

void setup()
{
  pinMode(4, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(1, OUTPUT);

  lightCount = 0;
  whichLight = 0;

  bufferData = (unsigned char*) malloc(1);
  // Set digital TX pin
  MANRX_SetRxPin(3);
  // Prepare interrupts
  MANRX_SetupReceive();
  // Begin receiving data
  MANRX_BeginReceiveBytes(1, bufferData);
}

void loop()
{
  if(lightCount != 0)
  {
    lightCount = lightCount + 1;
    if(lightCount > 1000) 
    {
      lightCount = 0;
      digitalWrite(whichLight, LOW);
      // Begin receiving data
      MANRX_BeginReceiveBytes(1, bufferData);
    }
  }

  if (MANRX_ReceiveComplete())
  {
    unsigned char receivedSize = 1;
    MANRX_GetMessageBytes(&receivedSize, &bufferData);

    if(bufferData[0] == 1 ) {
      bufferData[0] = 0;
      whichLight = 1;
      digitalWrite(whichLight, HIGH);
      lightCount = 1;
    }

    if( bufferData[0] == 2 ) {
      bufferData[0] = 0;
      whichLight = 2;
      digitalWrite(whichLight, HIGH);
      lightCount = 1;
    }
  }
}

