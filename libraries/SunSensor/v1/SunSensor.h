/*
SunSensor - Klasa czujnika, ktory sledzi slonce
Filozof404
Domena publiczna
*/

#ifndef SunSensor_h
#define SunSensor_h

#define DEBUG_SUNSENSOR 0
#if DEBUG_SUNSENSOR
#	define DEBUG_SUNSENSOR_PRINT(...)    Serial.print(__VA_ARGS__)
#	define DEBUG_SUNSENSOR_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#	define DEBUG_SUNSENSOR_PRINT(...)
#	define DEBUG_SUNSENSOR_PRINTLN(...)
#endif

class SunSensor
{
	public:
		struct SunDirection
		{
			int verticalRatio;
			int horizontalRatio;
		};

		SunSensor(int, int, int, int);
		int ReadSensor(int);
		SunDirection CalculateDirection();
		int GetDrag() const;
		void SetDrag(int);
	
	private:
		struct Light
		{
			unsigned long time;		// Kiedy byl pomiar
			int value;					// Jaki byl wynik
		};


		int photoresistorUp;
		int photoresistorDown;
		int photoresistorLeft;
		int photoresistorRight;
		int drag;					// Bezwladnosc fotorezystorow (w milisekundach)
		Light light_0;
		Light light_1;
		Light light_2;
		Light light_3;

		int Lerp(int, int, float) const;	// Uzywana przy kompensowaniu bezwladnosci
};

#endif