/*
Glowno dowodzaca klasa w sterowniku solarnym
Filozof404
Domena publiczna
Wersja 3
*/
#ifndef SolarController_h
#define SolarController_h

#define DEBUG_SOLAR_CONTROLLER 1
#if DEBUG_SOLAR_CONTROLLER
#	define DEBUG_SOLAR_CONTROLLER_PRINT(...)    Serial.print(__VA_ARGS__)
#	define DEBUG_SOLAR_CONTROLLER_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#	define DEBUG_SOLAR_CONTROLLER_PRINT(...)
#	define DEBUG_SOLAR_CONTROLLER_PRINTLN(...)
#endif

#include <UpdateManager.h>
#include <EEPROManager.h>
#include <EEPROMUsers.h>
#include <SerialListener.h>
#include <SerialUsers.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <RemoteCodes.h>
#include <RadioListener.h>
#include <RadioUsers.h>
#include <BridgeH.h>
#include <LedIndicator.h>
#include <SunSensor.h>
#include <PCF8574.h>
#include <TimeKeeper.h>




class SolarController : public UpdateManagerUser, public MessageReceiver
{
public:
	struct Configuration
	{
		RadioListener* radioListener;
		BridgeH* bridgeVertical;
		BridgeH* bridgeHorizontal;
		LedIndicator* ledIndicator;
		SunSensor* sunSensor;
		PCF8574* expander;
		byte wifiOffPin;

		Configuration()
		{
			radioListener = NULL;
			bridgeVertical = NULL;
			bridgeHorizontal = NULL;
			ledIndicator = NULL;
			sunSensor = NULL;
			expander = NULL;
		};
	};

	SolarController(Configuration*);
	void Update();
	bool TakeMessage(int, void*, void**);

private:
	enum State
	{
		idle,
		sunAware,
		nightAware,
		followSun,
		manual,
		foldDown,
		sleep,
	};

	struct UpdateInfo
	{
		unsigned long time_1;
		word data_1;
	};

	BridgeH* bridgeVertical;					//
	BridgeH* bridgeHorizontal;					//
	LedIndicator* ledIndicator;					// Hardware
	SunSensor* sunSensor;						//
	PCF8574* expander;							//
	bool ready;										// Mowi czy hardware jest prawidlowo podlaczony
	Controller_Data eepromData;
	int delay;
	int sensitivity;
	int overtake;
	int nightThreshold;
	int bridgeInitPWM;						// Uzywana podczas uruchamiania mostkow
	byte wifiOffPin;							// Mowi do ktorego pinu ekspandera jest podlaczony CH_PD modulu WiFi
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