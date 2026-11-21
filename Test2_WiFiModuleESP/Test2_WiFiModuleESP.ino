// Ten szkic ma na celu przetestowanie dzialania klasy WiFiModuleESP,
// a dokladnie to jej polaczenie z portem szeregowym.
// Nalezy go zainstalowac na mikrokontrolerze AVR,
// symulujac takie polaczenie jak w prawdziwym sterowniku solarnym



#include <UpdateManager.h>
#include <SerialListener.h>
#include <SerialUsers.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>

class SolarControllerDummy : public UpdateManagerUser, public MessageReceiver
{
public:
  SolarControllerDummy()
  {
    serialListener.Attach(SERIAL_SOLARCONTROLLER, this);
    updateManager.Attach(this);
  };
  
  void Update(){};
  
  bool TakeMessage(int message, void* sender, void** additionalData)
  {
    switch (message)
    {
      case MESSAGE_SERIAL_HAVE_DATA:
      {
        byte* receivedData = (byte*)additionalData;
        int subMessage = *((int*)receivedData);

        switch (subMessage)
        {
          case MESSAGE_GET:
          {
            byte dataPack[28];
            
            ((int*)dataPack)[0] = MESSAGE_GET_RESPONSE;
            dataPack[2] = 1;
            dataPack[3] = 2;
            ((float*)dataPack)[1] = 3.3f;
            ((float*)dataPack)[2] = 4.4f;
            ((float*)dataPack)[3] = 5.5f;
            ((int*)dataPack)[8] = 6;
            ((int*)dataPack)[9] = 7;
            ((int*)dataPack)[10] = 8;
            ((int*)dataPack)[11] = 9;
            ((int*)dataPack)[12] = 10;
            ((int*)dataPack)[13] = 11;
            
            serialListener.SendData(SERIAL_WIFIMODULEESP, 28, dataPack);
            break;
          }

          case MESSAGE_SET:
          {
            byte dataPack[3];
            
            ((int*)dataPack)[0] = MESSAGE_SET_RESPONSE;
            dataPack[2] = true;
            
            serialListener.SendData(SERIAL_WIFIMODULEESP, 3, dataPack);
            break;
          }

          case MESSAGE_MOTOR:
          {
            byte dataPack[3];
            
            ((int*)dataPack)[0] = MESSAGE_MOTOR_RESPONSE;
            dataPack[2] = true;
            
            serialListener.SendData(SERIAL_WIFIMODULEESP, 3, dataPack);
            break;
          }
        }
        
        return true;
      }
    }

    return false;
  };
};



SolarControllerDummy* pSolarControllerDummy;

void setup()
{
  serialListener.begin();
  pSolarControllerDummy = new SolarControllerDummy();
}

void loop()
{
  updateManager.UpdateAll();
}
