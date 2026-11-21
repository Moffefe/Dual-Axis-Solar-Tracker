#include "Arduino.h"
#include "SunSensor.h"



SunSensor::SunSensor(int up, int down, int left, int right)
{
	photoresistorUp = up;
	photoresistorDown = down;
	photoresistorLeft = left;
	photoresistorRight = right;
	drag = 0;
	light_0 = {0};
	light_1 = {0};
	light_2 = {0};
	light_3 = {0};
}

int SunSensor::ReadSensor(int sensor)
{
	switch (sensor)
	{
		case 0:
		{
			int value = analogRead(photoresistorUp);
			long deltaTime = millis() - light_0.time;

			if (!(drag == 0 || deltaTime <= 0))
			{
				if (deltaTime < drag)
				{
					// Jesli odczytywanie zachodzi z
					// wieksza czestotliwoscia niz 'drag',
					// to wynik przesowa sie w strone
					// 'light_0.value' (poprzedniego wyniku, a nie prawdziwego)

					float t = (float)deltaTime / (float)drag;
					value = Lerp(light_0.value, value, t);
				}
			}

			light_0.time = millis();		// Aktualizuje ta strukture, aby moc
			light_0.value = value;			// nastepny odczyt takze poprawic
			return value;
		}

		case 1:
		{
			int value = analogRead(photoresistorDown);
			value = +0.00000116609801f * (float)value*(float)value*(float)value
					-0.00060386345742f * (float)value*(float)value
					+0.40445609014287f * (float)value
					-0.37649389499207f;
			long deltaTime = millis() - light_1.time;

			if (!(drag == 0 || deltaTime <= 0))
			{
				if (deltaTime < drag)
				{
					float t = (float)deltaTime / (float)drag;
					value = Lerp(light_1.value, value, t);
				}
			}

			light_1.time = millis();
			light_1.value = value;
			return value;
		}

		case 2:
		{
			int value = analogRead(photoresistorLeft);
			long deltaTime = millis() - light_2.time;

			if (!(drag == 0 || deltaTime <= 0))
			{
				if (deltaTime < drag)
				{
					float t = (float)deltaTime / (float)drag;
					value = Lerp(light_2.value, value, t);
				}
			}

			light_2.time = millis();
			light_2.value = value;
			return value;
		}

		case 3:
		{
			int value = analogRead(photoresistorRight);
			value = -0.00000011351995f * (float)value*(float)value*(float)value
					-0.00005591689339f * (float)value*(float)value
					+1.17694951659063f * (float)value
					-1.29187338325229f;
			long deltaTime = millis() - light_3.time;

			if (!(drag == 0 || deltaTime <= 0))
			{
				if (deltaTime < drag)
				{
					float t = (float)deltaTime / (float)drag;
					value = Lerp(light_3.value, value, t);
				}
			}

			light_3.time = millis();
			light_3.value = value;
			return value;
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
}

// t==0 zwroci a
// t==1 zwroci b
int SunSensor::Lerp(int a, int b, float t) const
{
	float safeT;

	// Walidacja 't'
	safeT = constrain(t, 0.0f, 1.0f);

	int potential = a - b;				// Potencjal - czyli roznica poziomow miedzy 'a' i 'b'
	potential = potential * safeT;				// Odpowiednio zmniejszamy potencjal (znak zachowujemy)
	return a - potential;
}
