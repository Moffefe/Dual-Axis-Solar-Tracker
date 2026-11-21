/*
   LedIndicator - Klasa sterujaca diodami LED w sterowniku solarnym
   Filozof404
   Domena publiczna
   Wersja 3
 */
#ifndef LedIndicator_h
#define LedIndicator_h


#include <UpdateManager.h>
#include "PCF8574.h"


class LedIndicator : public UpdateManagerUser
{
	public:
		enum State
		{
			off,
			flashLed,
			blink,
			foldDown,
			sleep
		};

		LedIndicator(PCF8574*, uint8_t, uint8_t, uint8_t, uint8_t);
		void SetState(State, byte = 0, byte = 255);
		void Update();

	private:
		struct UpdateInfo
		{
			unsigned long time_1;
			byte data_1;
		};

		PCF8574* expander;			//
		uint8_t ledUp;				//
		uint8_t ledDown;			// Hardware
		uint8_t ledLeft;			//
		uint8_t ledRight;			//
		State currentState;
		byte currentPriority;
		UpdateInfo updateInfo;
		State nextState;
		byte nextPriority;
		byte nextStateInit;

		void SwitchState();
};

#endif