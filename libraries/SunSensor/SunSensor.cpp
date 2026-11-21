#include "Arduino.h"
#include "SunSensor.h"



SunSensor::SunSensor(uint8_t up, uint8_t down, uint8_t left, uint8_t right, SunSensor::Degree degree)
{
	photoresistorUp = up;
	photoresistorDown = down;
	photoresistorLeft = left;
	photoresistorRight = right;

	if (eeproManager.GetReady())													//
	{																				//
		eeproManager.ReadData(&eepromData, SENDER_SUNSENSOR);						//
		DEBUG_SUNSENSOR_PRINTLN(F("SunSensor: Udalo sie odczytac dane z EEPROM"));	//
	}																				//
	else																			//
	{																				//
		eepromData.drag = 0;														// Komunikacja z managerem pamieci nieulotnej
		eepromData.overtakeVertical = 0;											//
		eepromData.overtakeHorizontal = 0;											//
		eeproManager.WriteData(&eepromData, SENDER_SUNSENSOR);						//
		DEBUG_SUNSENSOR_PRINTLN(F("SunSensor: EEPROM uszkodzone"));					//
	}																				//
	drag = eepromData.drag;															//
	overtakeVertical = eepromData.overtakeVertical;									//
	overtakeHorizontal = eepromData.overtakeHorizontal;								//

	SunSensor::degree = degree;

	light_0 = {0, 0};
	light_1 = {0, 0};
	light_2 = {0, 0};
	light_3 = {0, 0};
}

int SunSensor::ReadSensor(int sensor)
{
	switch (degree)
	{
		case degree_90:
			switch (sensor)
			{
				case 0:
					sensor = 2;
					break;

				case 1:
					sensor = 3;
					break;

				case 2:
					sensor = 1;
					break;

				case 3:
					sensor = 0;
					break;
			}
			break;

		case degree_180:
			switch (sensor)
			{
				case 0:
					sensor = 1;
					break;

				case 1:
					sensor = 0;
					break;

				case 2:
					sensor = 3;
					break;

				case 3:
					sensor = 2;
					break;
			}
			break;

		case degree_270:
			switch (sensor)
			{
				case 0:
					sensor = 3;
					break;

				case 1:
					sensor = 2;
					break;

				case 2:
					sensor = 0;
					break;

				case 3:
					sensor = 1;
					break;
			}
			break;
	}

	switch (sensor)
	{
		case 0:
		{
			float value = analogRead(photoresistorUp);
			return CompensateDrag(value, light_0);
		}

		case 1:
		{
			float value = analogRead(photoresistorDown);
			return CompensateDrag(value, light_1);
		}

		case 2:
		{
			float value = analogRead(photoresistorLeft);
			return CompensateDrag(value, light_2);
		}

		case 3:
		{
			float value = analogRead(photoresistorRight);
			return CompensateDrag(value, light_3);
		}

		default:
			return -1;			// Niema takiego fotorezystora
	}
}

SunSensor::SunDirection SunSensor::CalculateDirection()
{
	SunDirection result;
	result.verticalRatio   = ReadSensor(0) - ReadSensor(1);		// Slonce ucieklo do gory (+)			do dolu (-)
	result.horizontalRatio = ReadSensor(3) - ReadSensor(2);		// Slonce ucieklo w prawo (+)			w lewo (-)

	result.verticalRatio += overtakeVertical;
	result.horizontalRatio += overtakeHorizontal;

	DEBUG_SUNSENSOR_PRINT(result.verticalRatio);
	DEBUG_SUNSENSOR_PRINT("\t");
	DEBUG_SUNSENSOR_PRINTLN(result.horizontalRatio);

	return result;
}

int SunSensor::GetDrag() const
{
	return drag;
}

void SunSensor::SetDrag(int drag)
{
	(drag < 0) ? SunSensor::drag = 0 : SunSensor::drag = drag;
	eepromData.drag = SunSensor::drag;
	eeproManager.WriteData(&eepromData, SENDER_SUNSENSOR);
}

void SunSensor::GetOvertake(int& overtakeVertical, int& overtakeHorizontal) const
{
	overtakeVertical = SunSensor::overtakeVertical;
	overtakeHorizontal = SunSensor::overtakeHorizontal;
}

void SunSensor::SetOvertake(int overtakeVertical, int overtakeHorizontal)
{
	SunSensor::overtakeVertical = overtakeVertical;
	SunSensor::overtakeHorizontal = overtakeHorizontal;
	eepromData.overtakeVertical = SunSensor::overtakeVertical;
	eepromData.overtakeHorizontal = SunSensor::overtakeHorizontal;
	eeproManager.WriteData(&eepromData, SENDER_SUNSENSOR);
}

// t==0 zwroci a
// t==1 zwroci b
int SunSensor::Lerp(int a, int b, float t) const
{
	float safeT;
	safeT = constrain(t, 0.0f, 1.0f);

	int potential = a - b;
	potential = potential * safeT;
	return a - potential;
}

// Jest to pomocnicza funkcja
// dla funkcji ReadSensor()
// i tylko w niej uzywana.
int SunSensor::CompensateDrag(int value, Light& light)
{
	if (drag != 0)
	{
		long deltaTime = millis() - light.time;

		if (deltaTime > 0)
		{
			float t = (float)deltaTime / (float)drag;
			value = Lerp(light.value, value, t);
		}
	}

	light.time = millis();
	light.value = value;
	return value;
}
