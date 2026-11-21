/*
SunSensor - Klasa czujnika, ktory sledzi slonce
Filozof404
Domena publiczna
*/

#ifndef SunSensor_h
#define SunSensor_h

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
	
	private:
		int photoresistorUp;
		int photoresistorDown;
		int photoresistorLeft;
		int photoresistorRight;
};

#endif