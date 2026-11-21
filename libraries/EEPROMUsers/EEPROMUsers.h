// Plik umozliwiajacy wspolprace EEPROManagera z klasami
// specyficznymi dla tego projektu

#ifndef EEPROMUsers_h
#define EEPROMUsers_h

#define SENDER_CONTROLLER 14
struct Controller_Data
{
	int delay;
	int sensitivity;
	int overtake;
	int nightThreshold;
};

#define SENDER_SUNSENSOR 15
struct SunSensor_Data
{
	int drag;
	int overtakeVertical;
	int overtakeHorizontal;
};

#endif