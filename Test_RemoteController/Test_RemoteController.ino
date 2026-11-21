#include <RemoteController.h>



RemoteController* remoteController;
RemoteKeyboard* remoteKeyboard;
RadioBroadcaster* radioBroadcaster;



void setup()
{
  radioBroadcaster = new RadioBroadcaster();
  
  remoteController = new RemoteController(radioBroadcaster);

  RemoteKeyboard::Configuration conf;
  conf.user = remoteController;
  conf.groupPin_1 = A5;
  conf.groupPin_2 = A4;
  conf.groupPin_3 = A3;
  conf.groupPin_4 = A2;
  conf.groupPin_5 = A1;
  conf.groupPin_6 = A0;
  conf.sensorPin_1 = 2;
  conf.sensorPin_2 = 4;
  conf.sensorPin_3 = 5;
  conf.sensorPin_4 = 6;
  conf.sensorPin_5 = 7;
  conf.sensorPin_6 = 8;
  conf.sensorPin_7 = 9;
  conf.scanDuration = 100;
  remoteKeyboard = new RemoteKeyboard(&conf);
  remoteKeyboard->begin();
}

void loop()
{
  updateManager.UpdateAll();
}
