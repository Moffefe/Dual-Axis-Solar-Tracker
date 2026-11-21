#include <SolarController.h>



SolarController* pSolarController;    // Jeden aby wszystkimi sterowac



void setup()
{ 
  serialListener.begin();
  timeKeeper.begin();
  
  SolarController::Configuration conf;
  
  conf.radioListener = new RadioListener();             // Przygotowuje odbiornik radiowy
  conf.radioListener->begin(digitalPinToInterrupt(3));  //
  
  conf.bridgeVertical = new BridgeH(4, 6, 5, 7);      // Przygotowuje mostki H
  conf.bridgeHorizontal = new BridgeH(8, 10, 9, 11);  //

  PCF8574* pExpander = new PCF8574();                             //
  pExpander->begin(0x20);                                         // Przygotowuje LedIndicator
  conf.ledIndicator = new LedIndicator(pExpander, 2, 3, 4, 5);    //
  
  conf.sunSensor = new SunSensor(A3, A2, A1, A0);       // Przygotowuje SunSensor

  conf.expander = pExpander;      // Przygotowuje elementy dzieki ktorym SolarController
  conf.wifiOffPin = 1;            // moze wylaczyc modul WiFi

  pSolarController = new SolarController(&conf);
  
  eeproManager.ForceReady();
}

void loop()
{
  updateManager.UpdateAll();
}
