/*
Controller - glowno dowodzaca klasa w sterowniku solarnym
w sterowniku solarnym
Filozof404
Domena publiczna
Wersja 2
*/

#ifndef Controller_h
#define Controller_h

#define DEBUG_CONTROLLER  0
#if DEBUG_CONTROLLER
#	define DEBUG_CONTROLLER_PRINT(...)    Serial.print(__VA_ARGS__)
#	define DEBUG_CONTROLLER_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#	define DEBUG_CONTROLLER_PRINT(...)
#	define DEBUG_CONTROLLER_PRINTLN(...)
#endif

#include <RemoteCodes.h>
#include <IRremote.h>
#include <BridgeH.h>
#include <LedIndicator.h>
#include <SunSensor.h>


class Controller
{
	public:
		enum State
		{
			idle,
			sunAware,
			nightAware,
			followSun,
			manual,
			foldDown,
			sleep
		};

		struct Configuration
		{
			IRrecv* irReceiver;
			BridgeH* bridgeVertical;
			BridgeH* bridgeHorizontal;
			LedIndicator* ledIndicator;
			SunSensor* sunSensor;

			Configuration()
			{
				irReceiver = 0;
				bridgeVertical = 0;
				bridgeHorizontal = 0;
				ledIndicator = 0;
				sunSensor = 0;
			};
		};

		Controller(Configuration);
		void GetState(State*, word*) const;
		void Update();

	private:
		struct UpdateInfo
		{
			unsigned long time_1;
			word data_1;
		};

		IRrecv* irReceiver;				//
		BridgeH* bridgeVertical;		//
		BridgeH* bridgeHorizontal;		// Hardware
		LedIndicator* ledIndicator;		//
		SunSensor* sunSensor;			//
		bool ready;							// Mowi czy hardware jest prawidlowo podlaczony
		int delay;
		int sensitivity;
		int overtake;
		int nightThreshold;
		unsigned long irSignalRefresh;	// Przechowuje czas kiedy ma zostac zaktualizowany sygnal z pilota
		decode_results currentIrSignal;		// Przechowuje aktualny input z pilota
		State currentState;
		UpdateInfo updateInfo;
		State nextState;
		word nextStateInit;

		void SetState(State, word = 0);
		void SwitchState();
		void IdleProcedure();
		void SunAwareProcedure();
		void NightAwareProcedure();
		void FollowSunProcedure();
		void ManualProcedure();
		void FoldDownProcedure();
		void SleepProcedure();
		void Reset();
};

#endif