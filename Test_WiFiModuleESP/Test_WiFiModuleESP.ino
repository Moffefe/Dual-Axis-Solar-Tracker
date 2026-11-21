#include <WiFiModuleESP.h>


SdFat sdFat;
WiFiModuleESP* pWiFiModuleESP;


void setup()
{
  //Serial1.begin(115200);                    // Do debugowania
  
  timeKeeper.begin();
  serialListener.begin();
  pWiFiModuleESP = new WiFiModuleESP(sdFat);
}

void loop()
{
  updateManager.UpdateAll();
}
