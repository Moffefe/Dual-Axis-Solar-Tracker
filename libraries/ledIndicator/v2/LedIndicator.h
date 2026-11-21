/*
   LedIndicator - Klasa sterujaca diodami LED w sterowniku solarnym
   Filozof404
   Domena publiczna
   Wersja 2
 */
 
#ifndef LedIndicator_h
#define LedIndicator_h

class LedIndicator
{
	public:
		enum State
		{
			off,
			flashLed,
			blink,			// Podwyzszony priorytet
			foldDown,
			sleep
		};

		LedIndicator(int, int, int, int);
		void SetState(State, byte = 0, byte = 255);
		void Update();

	private:
		struct UpdateInfo
		{
			unsigned long time_1;
			byte data_1;
		};

		int ledUp;				//
		int ledDown;			// Piny
		int ledLeft;			// Arduino
		int ledRight;			//
		State currentState;
		byte currentPriority;
		UpdateInfo updateInfo;
		State nextState;
		byte nextPriority;
		byte nextStateInit;

		void SwitchState();
};

#endif