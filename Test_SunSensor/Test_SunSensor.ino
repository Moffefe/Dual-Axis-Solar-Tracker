#include <SunSensor.h>



SunSensor* sensor;



void setup()
{
  Serial.begin(9600);
  sensor = new SunSensor(A3, A2, A1, A0);
  sensor->SetDrag(300);
  eeproManager.ForceReady();
}

void loop()
{
  Serial.print(sensor->ReadSensor(0));
  Serial.print("\t");
  Serial.print(sensor->ReadSensor(1));
  Serial.print("\t");
  Serial.print(sensor->ReadSensor(2));
  Serial.print("\t");  
  Serial.println(sensor->ReadSensor(3));

  SunSensor::SunDirection sunDir = sensor->CalculateDirection();
  Serial.print(sunDir.verticalRatio);
  Serial.print("\t");
  Serial.println(sunDir.horizontalRatio);

  updateManager.UpdateAll();
  delay(100);
}
