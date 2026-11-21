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


#include <EEPROManager.h>
#include <EEPROMUsers.h>


class SunSensor
{
	public:
		enum Degree
		{
			degree_0,
			degree_90,
			degree_180,
			degree_270
		};

		struct SunDirection
		{
			int verticalRatio;
			int horizontalRatio;
		};

		SunSensor(uint8_t, uint8_t, uint8_t, uint8_t, Degree = degree_0);
		int ReadSensor(int);
		SunDirection CalculateDirection();
		int GetDrag() const;
		void SetDrag(int);
		void GetOvertake(int&, int&) const;
		void SetOvertake(int, int);
	
	private:
		struct Light
		{
			unsigned long time;		// Kiedy byl pomiar
			int value;					// Jaki byl wynik
		};

		uint8_t photoresistorUp;
		uint8_t photoresistorDown;
		uint8_t photoresistorLeft;
		uint8_t photoresistorRight;
		SunSensor_Data eepromData;
		int drag;
		int overtakeVertical;
		int overtakeHorizontal;
		Degree degree;
		Light light_0;
		Light light_1;
		Light light_2;
		Light light_3;

		int Lerp(int, int, float) const;
		int CompensateDrag(int, Light&);
};

#endif