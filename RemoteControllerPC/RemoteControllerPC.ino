#include<RadioListener.h>
#include<RadioUsers.h>
#include<RemoteCodes.h>



RadioBroadcaster* radioBroadcaster;
uint8_t dataPack[4];



void setup()
{
  Serial.begin(9600);
  radioBroadcaster = new RadioBroadcaster();
  radioBroadcaster->begin(10);
}

void loop()
{
  if (Serial.available() > 0)
  {
    switch(Serial.read())
    {
      case 'a':
        ((uint32_t*)dataPack)[0] = BUTTON_ON;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_ON");
        break;

      case 'b':
        ((uint32_t*)dataPack)[0] = BUTTON_STOP;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_STOP");
        break;
      
      case 'c':
        ((uint32_t*)dataPack)[0] = BUTTON_PLAY;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_PLAY");
        break;

      case 'd':
        ((uint32_t*)dataPack)[0] = BUTTON_UP;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_UP");
        break;

      case 'e':
        ((uint32_t*)dataPack)[0] = BUTTON_DOWN;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_DOWN");
        break;

      case 'f':
        ((uint32_t*)dataPack)[0] = BUTTON_LEFT;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_LEFT");
        break;

      case 'g':
        ((uint32_t*)dataPack)[0] = BUTTON_RIGHT;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_RIGHT");
        break;

      case '1':
        ((uint32_t*)dataPack)[0] = BUTTON_1;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_1");
        break;

      case '2':
        ((uint32_t*)dataPack)[0] = BUTTON_2;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_2");
        break;

      case '3':
        ((uint32_t*)dataPack)[0] = BUTTON_3;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_3");
        break;

      case '4':
        ((uint32_t*)dataPack)[0] = BUTTON_4;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_4");
        break;

      case '5':
        ((uint32_t*)dataPack)[0] = BUTTON_5;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_5");
        break;

      case '6':
        ((uint32_t*)dataPack)[0] = BUTTON_6;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_6");
        break;

      case 'h':
        ((uint32_t*)dataPack)[0] = BUTTON_VOLUME_UP;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_VOLUME_UP");
        break;

      case 'i':
        ((uint32_t*)dataPack)[0] = BUTTON_VOLUME_DOWN;
        radioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);
        Serial.println("BUTTON_VOLUME_DOWN");
        break;
    }
  }
  
  updateManager.UpdateAll();
}
