#include "Arduino.h"
#include "SunSensor.h"



SunSensor::SunSensor(int up, int down, int left, int right)
{
	photoresistorUp = up;
	photoresistorDown = down;
	photoresistorLeft = left;
	photoresistorRight = right;
}

int SunSensor::ReadSensor(int sensor)
{
	switch (sensor)
	{
		case 0:
			return analogRead(photoresistorUp);

		case 1:
		{
			int value = analogRead(photoresistorDown);
			value = +0.00000116609801f * (float)value*(float)value*(float)value
					-0.00060386345742f * (float)value*(float)value
					+0.40445609014287f * (float)value
					-0.37649389499207f;

			return value;
		}

		case 2:
			return analogRead(photoresistorLeft);

		case 3:
		{
			int value = analogRead(photoresistorRight);
			value = -0.00000011351995f * (float)value*(float)value*(float)value
					-0.00005591689339f * (float)value*(float)value
					+1.17694951659063f * (float)value
					-1.29187338325229f;

			return value;
		}

		default:
			return -1;			// Niema takiego rezystora
	}
}

SunSensor::SunDirection SunSensor::CalculateDirection()
{
	SunDirection result;
	result.verticalRatio   = ReadSensor(0) - ReadSensor(1);		// Slonce ucieklo do gory (+)			do dolu (-)
	result.horizontalRatio = ReadSensor(3) - ReadSensor(2);		// Slonce ucieklo w prawo (+)			w lewo (-)
	return result;
}

void SunSensor::SetDrag(int)
{
}
