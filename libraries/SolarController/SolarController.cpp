#include "Arduino.h"
#include "SolarController.h"

SolarController::SolarController(Configuration* pConfiguration)
{
	DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_1"));

	ready = false;

	if (pConfiguration->radioListener)
		pConfiguration->radioListener->Attach(RADIO_SOLARCONTROLLER, this);
	else
		return;

	if (pConfiguration->bridgeVertical)
		bridgeVertical = pConfiguration->bridgeVertical;
	else
		return;

	if (pConfiguration->bridgeHorizontal)
		bridgeHorizontal = pConfiguration->bridgeHorizontal;
	else
		return;

	if (pConfiguration->ledIndicator)
		ledIndicator = pConfiguration->ledIndicator;
	else
		return;

	if (pConfiguration->sunSensor)
		sunSensor = pConfiguration->sunSensor;
	else
		return;

	if (pConfiguration->expander)
		expander = pConfiguration->expander;
	else
		return;

	if (eeproManager.GetReady())							
	{														
		eeproManager.ReadData(&eepromData, SENDER_CONTROLLER);		
		
		DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_2"));
	}														
	else													
	{														
		eepromData.delay = 10;								
		eepromData.sensitivity = 30;						
		eepromData.overtake = 0;							
		eepromData.nightThreshold = 30;						
		eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);
		
		DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_3"));
	}														
	delay = eepromData.delay;								
	sensitivity = eepromData.sensitivity;					
	overtake = eepromData.overtake;							
	nightThreshold = eepromData.nightThreshold;				

	bridgeInitPWM = 13 << 8;
	bridgeInitPWM = bridgeInitPWM | 2 << 2;

	wifiOffPin = pConfiguration->wifiOffPin;
	expander->pinMode(wifiOffPin, OUTPUT);
	uint16_t* subMessage = new uint16_t();
	*subMessage = MESSAGE_TURN_ON_WIFI;
	timeKeeper.RequestMessage(millis() + 3000, this, (void**)subMessage);

	serialListener.Attach(SERIAL_SOLARCONTROLLER, this);
	SetState(State::idle);

	DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_4"));

	ready = true;
}

void SolarController::Update()
{
	SwitchState();						// Tylko tu wolno wywolac ta funkcje

	switch (currentState)
	{
		case State::idle:
			IdleProcedure();
			break;
	
		case State::sunAware:
			SunAwareProcedure();
			break;
	
		case State::nightAware:
			NightAwareProcedure();
			break;
	
		case State::followSun:
			FollowSunProcedure();
			break;
	
		case State::manual:
			ManualProcedure();
			break;
	
		case State::foldDown:
			FoldDownProcedure();
			break;
	
		case State::sleep:
			SleepProcedure();
			break;
	
		default:
			SetState(State::idle);
			break;
	}
}

bool SolarController::TakeMessage(int message, void* sender, void** additionalData)
{
	int overtakeVertical;		// Te zmienne sa potrzebne w kilku roznych case'ach ponizej, aby uniknac wielokrotnej
	int overtakeHorizontal;		// deklaracji wewnatrz nich, deklaruje je tutaj

	switch (message)
	{
		case MESSAGE_SERIAL_HAVE_DATA:
		{
			byte* receivedData = (byte*)additionalData;		// Dla wygody odrazu rzutuje sobie ten wskaznik
			int subMessage = ((int*)receivedData)[0];				// 2 pierwsze bajty 'additionalData' zawieraja pod-wiadomosc

			switch (subMessage)
			{
				case MESSAGE_MANUAL:
				{
					DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_21"));

					SetState(State::manual);
					break;
				}

				case MESSAGE_AUTO:
				{
					DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_22"));

					SetState(State::idle);
					break;
				}

				case MESSAGE_GET:
				{
					DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_23"));

					byte dataPack[32];
					((int*)dataPack)[0] = MESSAGE_GET_RESPONSE;
					dataPack[2] = currentState;
					((float*)dataPack)[1] = -1.0f;
					((float*)dataPack)[2] = -1.0f;
					((float*)dataPack)[3] = -1.0f;
					((int*)dataPack)[8] = delay;
					((int*)dataPack)[9] = sensitivity;
					((int*)dataPack)[10] = overtake;
					((int*)dataPack)[11] = nightThreshold;
					((int*)dataPack)[12] = sunSensor->ReadSensor(0);
					((int*)dataPack)[13] = sunSensor->GetDrag();
					sunSensor->GetOvertake(overtakeVertical, overtakeHorizontal);
					((int*)dataPack)[14] = overtakeVertical;
					((int*)dataPack)[15] = overtakeHorizontal;

					dataPack[3] = 0;
					BridgeH::State bridgeState;
					bridgeVertical->GetState(&bridgeState);
					if (bridgeState == BridgeH::State::working || bridgeState == BridgeH::State::workingPWM)
						bitSet(dataPack[3], 0);
					bridgeHorizontal->GetState(&bridgeState);
					if (bridgeState == BridgeH::State::working || bridgeState == BridgeH::State::workingPWM)
						bitSet(dataPack[3], 1);

					serialListener.SendData(SERIAL_WIFIMODULEESP, 32, dataPack);
					break;
				}

				case MESSAGE_SET:
				{
					DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_24"));

					delay = ((int*)receivedData)[1];
					sensitivity = ((int*)receivedData)[2];
					overtake = ((int*)receivedData)[3];
					nightThreshold = ((int*)receivedData)[4];
					sunSensor->SetDrag(((int*)receivedData)[5]);
					sunSensor->SetOvertake(((int*)receivedData)[6], ((int*)receivedData)[7]);

					eepromData.delay = delay;
					eepromData.sensitivity = sensitivity;
					eepromData.overtake = overtake;
					eepromData.nightThreshold = nightThreshold;
					eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

					byte dataPack[3];
					((int*)dataPack)[0] = MESSAGE_SET_RESPONSE;
					dataPack[2] = true;
					serialListener.SendData(SERIAL_WIFIMODULEESP, 3, dataPack);
					break;
				}

				case MESSAGE_MOTOR:
				{
					DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_25"));

					byte dataPack[3];
					((int*)dataPack)[0] = MESSAGE_MOTOR_RESPONSE;

					if (currentState == State::manual)
					{
						if (bitRead(receivedData[2], 0))
						{
							bridgeVertical->SetState(BridgeH::State::working, 1);
						}
						else
						{
							if (bitRead(receivedData[2], 1))
							{
								bridgeVertical->SetState(BridgeH::State::working, 2);
							}
							else
							{
								bridgeVertical->SetState(BridgeH::State::off);
							}
						}
						
						if (bitRead(receivedData[2], 2))
						{
							bridgeHorizontal->SetState(BridgeH::State::working, 1);
						}
						else
						{
							if (bitRead(receivedData[2], 3))
							{
								bridgeHorizontal->SetState(BridgeH::State::working, 2);
							}
							else
							{
								bridgeHorizontal->SetState(BridgeH::State::off);
							}
						}

						dataPack[2] = true;
					}
					else
					{
						dataPack[2] = false;
					}

					serialListener.SendData(SERIAL_WIFIMODULEESP, 3, dataPack);
					break;
				}

				case MESSAGE_TURN_OFF_WIFI:
				{
					expander->digitalWrite(wifiOffPin, LOW);
					break;
				}
			}

			return true;
		}

		case MESSAGE_RADIO_HAVE_DATA:
		{
			ledIndicator->SetState(LedIndicator::State::blink, 0, 254);

			unsigned long subMessage = ((unsigned long*)additionalData)[0];		// 4 pierwsze bajty 'additionalData' zawieraja pod-wiadomosc

			switch (subMessage)
			{
				case BUTTON_ON:
				{
					DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_26"));

					Reset();
					break;
				}

				case BUTTON_STOP:
				{
					DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_27"));

					SetState(State::manual);
					break;
				}
			}

			if (currentState == State::manual)
			{
				switch (subMessage)
				{
					case BUTTON_PLAY:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_28"));

						SetState(State::idle);
						break;
					}

					case BUTTON_UP:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_29"));

						BridgeH::State state;
						bridgeVertical->GetState(&state);

						if (state == BridgeH::State::off)
							bridgeVertical->SetState(BridgeH::State::working, 1);
						else
							bridgeVertical->SetState(BridgeH::State::off);

						break;
					}

					case BUTTON_DOWN:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_30"));

						BridgeH::State state;
						bridgeVertical->GetState(&state);

						if (state == BridgeH::State::off)
							bridgeVertical->SetState(BridgeH::State::working, 2);
						else
							bridgeVertical->SetState(BridgeH::State::off);

						break;
					}

					case BUTTON_LEFT:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_31"));

						BridgeH::State state;
						bridgeHorizontal->GetState(&state);

						if (state == BridgeH::State::off)
							bridgeHorizontal->SetState(BridgeH::State::working, 1);
						else
							bridgeHorizontal->SetState(BridgeH::State::off);

						break;
					}

					case BUTTON_RIGHT:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_32"));

						BridgeH::State state;
						bridgeHorizontal->GetState(&state);

						if (state == BridgeH::State::off)
							bridgeHorizontal->SetState(BridgeH::State::working, 2);
						else
							bridgeHorizontal->SetState(BridgeH::State::off);

						break;
					}

					case BUTTON_1:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_33"));

						updateInfo.data_1 = 1;
						break;
					}

					case BUTTON_2:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_34"));

						updateInfo.data_1 = 2;
						break;
					}

					case BUTTON_3:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_35"));

						updateInfo.data_1 = 3;
						break;
					}

					case BUTTON_4:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_36"));

						updateInfo.data_1 = 4;
						break;
					}

					case BUTTON_5:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_37"));

						updateInfo.data_1 = 5;
						break;
					}

					case BUTTON_6:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_38"));

						updateInfo.data_1 = 6;
						break;
					}

					case BUTTON_7:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_39"));

						updateInfo.data_1 = 7;
						break;
					}

					case BUTTON_8:
					{
						DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_40"));

						updateInfo.data_1 = 8;
						break;
					}

					case BUTTON_VOLUME_UP:
					{
						switch (updateInfo.data_1)
						{
						case 1:
							delay += 10;
							eepromData.delay = delay;
							eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_41\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(delay);
							break;

						case 2:
							sensitivity += 1;
							eepromData.sensitivity = sensitivity;
							eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_42\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(sensitivity);
							break;

						case 3:
							nightThreshold += 1;
							eepromData.nightThreshold = nightThreshold;
							eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_43\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(nightThreshold);
							break;

						case 4:
							sunSensor->SetDrag(sunSensor->GetDrag() + 100);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_44\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(sunSensor->GetDrag());
							break;

						case 5:
							overtake += 1;
							eepromData.overtake = overtake;
							eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_45\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(overtake);
							break;

						case 6:
							expander->digitalWrite(wifiOffPin, HIGH);

							DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_46"));
							break;

						case 7:
							sunSensor->GetOvertake(overtakeVertical, overtakeHorizontal);
							overtakeVertical += 1;
							sunSensor->SetOvertake(overtakeVertical, overtakeHorizontal);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_47\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(overtakeVertical);
							break;

						case 8:
							sunSensor->GetOvertake(overtakeVertical, overtakeHorizontal);
							overtakeHorizontal += 1;
							sunSensor->SetOvertake(overtakeVertical, overtakeHorizontal);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_48\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(overtakeHorizontal);
							break;
						}

						break;
					}

					case BUTTON_VOLUME_DOWN:
					{
						switch (updateInfo.data_1)
						{
						case 1:
							delay -= 10;

							if (delay < 0)			//
								delay = 0;			// Walidacja danych

							eepromData.delay = delay;
							eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_49\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(delay);
							break;

						case 2:
							sensitivity -= 1;

							if (sensitivity < 0)
								sensitivity = 0;

							eepromData.sensitivity = sensitivity;
							eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_50\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(sensitivity);
							break;

						case 3:
							nightThreshold -= 1;

							if (nightThreshold < 0)
								nightThreshold = 0;

							eepromData.nightThreshold = nightThreshold;
							eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_51\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(nightThreshold);
							break;

						case 4:
							sunSensor->SetDrag(sunSensor->GetDrag() - 100);		//Bez walidacji, bo robi ja klasa SunSensor

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_52\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(sunSensor->GetDrag());
							break;

						case 5:
							overtake -= 1;			// Bez walidacji ([+]wyprzedzenie , opoznienie[-])

							eepromData.overtake = overtake;
							eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_53\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(overtake);
							break;

						case 6:
							DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_54"));

							expander->digitalWrite(wifiOffPin, LOW);
							break;

						case 7:
							sunSensor->GetOvertake(overtakeVertical, overtakeHorizontal);
							overtakeVertical -= 1;
							sunSensor->SetOvertake(overtakeVertical, overtakeHorizontal);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_55\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(overtakeVertical);
							break;

						case 8:
							sunSensor->GetOvertake(overtakeVertical, overtakeHorizontal);
							overtakeHorizontal -= 1;
							sunSensor->SetOvertake(overtakeVertical, overtakeHorizontal);

							DEBUG_SOLAR_CONTROLLER_PRINT(F("1_56\x1F"));
							DEBUG_SOLAR_CONTROLLER_PRINTLN(overtakeHorizontal);
							break;
						}

						break;
					}
				}
			}

			return true;
		}

		case MESSAGE_TIME_KEEPER:
		{
			switch (*((uint16_t*)additionalData))
			{
				case MESSAGE_TURN_ON_WIFI:
					expander->digitalWrite(wifiOffPin, HIGH);
					break;
			}

			delete ((uint16_t*)additionalData);
			return true;
		}
	}

	return false;
}

// "Zamawia" zmiane stanu
void SolarController::SetState(State newState, word initializationData)
{
	nextState = newState;
	nextStateInit = bit(15) | initializationData;
	updateManager.Attach(this);

	#if DEBUG_SOLAR_CONTROLLER
	switch (nextState)
	{
		case State::idle:
			DEBUG_SOLAR_CONTROLLER_PRINT(F("1_101\x1F"));
			break;

		case State::sunAware:
			DEBUG_SOLAR_CONTROLLER_PRINT(F("1_102\x1F"));
			break;

		case State::nightAware:
			DEBUG_SOLAR_CONTROLLER_PRINT(F("1_103\x1F"));
			break;

		case State::followSun:
			DEBUG_SOLAR_CONTROLLER_PRINT(F("1_104\x1F"));
			break;

		case State::manual:
			DEBUG_SOLAR_CONTROLLER_PRINT(F("1_105\x1F"));
			break;

		case State::foldDown:
			DEBUG_SOLAR_CONTROLLER_PRINT(F("1_106\x1F"));
			break;

		case State::sleep:
			DEBUG_SOLAR_CONTROLLER_PRINT(F("1_107\x1F"));
			break;
	}
	#endif
	DEBUG_SOLAR_CONTROLLER_PRINTLN(nextStateInit);
}

// "Realizuje" zamowienie zmiany stanu
void SolarController::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void SolarController::IdleProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (bitRead(updateInfo.data_1, 15))
	{
		ledIndicator->SetState(LedIndicator::State::off);
		bridgeVertical->SetState(BridgeH::State::off);
		bridgeHorizontal->SetState(BridgeH::State::off);

		bitClear(updateInfo.data_1, 15);
		return;
	}

	// Jesli wykryto
	// niski poziom swiatla
	if (sunSensor->ReadSensor(0) < nightThreshold)
	{
		SetState(State::nightAware);
		return;
	}

	SunSensor::SunDirection sunDirection = sunSensor->CalculateDirection();

	// Jesli wykryto
	// roznice swiatla
	if (abs(sunDirection.horizontalRatio) > sensitivity || abs(sunDirection.verticalRatio) > sensitivity)
	{
		SetState(State::sunAware);
		return;
	}
}

void SolarController::SunAwareProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (bitRead(updateInfo.data_1, 15))
	{
		updateInfo.data_1 = 0;
		updateInfo.time_1 = millis() + ((unsigned long)delay)*1000L;		// Czas do odpalenia silnikow ('delay' przetrzymuje sekundy, dlatego mnozymy razy 1000)

		SunSensor::SunDirection sunDirection = sunSensor->CalculateDirection();

		if (sunDirection.horizontalRatio > sensitivity)
			bitSet(updateInfo.data_1, 3);
		else
		{
			if (sunDirection.horizontalRatio < -sensitivity)
				bitSet(updateInfo.data_1, 2);
		}
		if (sunDirection.verticalRatio > sensitivity)
			bitSet(updateInfo.data_1, 0);
		else
		{
			if (sunDirection.verticalRatio < -sensitivity)
				bitSet(updateInfo.data_1, 1);
		}

		ledIndicator->SetState(LedIndicator::State::flashLed, 0b01000000 | updateInfo.data_1);

		return;
	}

	SunSensor::SunDirection sunDirection = sunSensor->CalculateDirection();

	if (abs(sunDirection.horizontalRatio) > sensitivity || abs(sunDirection.verticalRatio) > sensitivity)
	{
		word newDirection = 0;

		if (sunDirection.horizontalRatio > sensitivity)
			bitSet(newDirection, 3);
		else
		{
			if (sunDirection.horizontalRatio < -sensitivity)
				bitSet(newDirection, 2);
		}

		if (sunDirection.verticalRatio > sensitivity)
			bitSet(newDirection, 0);
		else
		{
			if (sunDirection.verticalRatio < -sensitivity)
				bitSet(newDirection, 1);
		}

		if (updateInfo.data_1 != newDirection)
		{
			ledIndicator->SetState(LedIndicator::State::flashLed, 0b01000000 | newDirection);
			updateInfo.time_1 = millis() + ((unsigned long)delay) * 1000L;
			updateInfo.data_1 = newDirection;
		}
		else
		{
			if ((long)(millis() - updateInfo.time_1) >= 0)
				SetState(State::followSun, updateInfo.data_1);
		}
	}
	else
	{
		SetState(State::idle);
	}
}

void SolarController::NightAwareProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (bitRead(updateInfo.data_1, 15))
	{
		updateInfo.time_1 = millis() + 600000;		//10min
		bitClear(updateInfo.data_1, 15);
		return;
	}

	if (sunSensor->ReadSensor(0) < nightThreshold + 2)					// +2 jest po to aby wprowadziæ mala histereze
	{
		if ((long)(millis() - updateInfo.time_1) >= 0)
			SetState(State::foldDown);
	}
	else
	{
		SetState(State::idle);
	}
}

// Uzywa danych inicjalizacyjnych!
void SolarController::FollowSunProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (bitRead(updateInfo.data_1, 15))
	{
		if (1 & updateInfo.data_1)															//
			bridgeVertical->SetState(BridgeH::State::workingPWM, bridgeInitPWM | 1);		//
		else																				//
		{																					//
			if (2 & updateInfo.data_1)														//
				bridgeVertical->SetState(BridgeH::State::workingPWM, bridgeInitPWM | 2);	//
		}																					// W zaleznosci od danych inicjalizacyjnych
																							// wlaczam odpowiednie mostki
		if (4 & updateInfo.data_1)															//
			bridgeHorizontal->SetState(BridgeH::State::workingPWM, bridgeInitPWM | 1);		//
		else																				//
		{																					//
			if (8 & updateInfo.data_1)														//
				bridgeHorizontal->SetState(BridgeH::State::workingPWM, bridgeInitPWM | 2);	//
		}																					//

		bitSet(updateInfo.data_1, 4);
		updateInfo.time_1 = millis() + 30000;

		bitClear(updateInfo.data_1, 15);
		return;
	}

	if (bitRead(updateInfo.data_1, 4))
	{
		bitClear(updateInfo.data_1, 4);
		return;
	}

	SunSensor::SunDirection sunDirection = sunSensor->CalculateDirection();

	if ( abs(sunDirection.verticalRatio) > (sensitivity -5))
	{
		if( (1 & updateInfo.data_1) && (sunDirection.verticalRatio < 0) )
			bridgeVertical->SetState(BridgeH::State::off);							// Niestety, minelismy cel
		else
		{
			if ((2 & updateInfo.data_1) && (sunDirection.verticalRatio > 0))
				bridgeVertical->SetState(BridgeH::State::off);						// Niestety, minelismy cel
		}
	}
	else
	{
		bridgeVertical->SetState(BridgeH::State::off);			// Roznica miedzy tymi 2 sensorami juz jest taka mala, ze wystarczy
	}

	if ( abs(sunDirection.horizontalRatio) > (sensitivity - 5))
	{
		if ((4 & updateInfo.data_1) && (sunDirection.horizontalRatio > 0))
			bridgeHorizontal->SetState(BridgeH::State::off);
		else
		{
			if ((8 & updateInfo.data_1) && (sunDirection.horizontalRatio < 0))
				bridgeHorizontal->SetState(BridgeH::State::off);
		}
	}
	else
	{
		bridgeHorizontal->SetState(BridgeH::State::off);		// Roznica miedzy tymi 2 sensorami juz jest taka mala, ze wystarczy
	}

	BridgeH::State verticalState;					//
	BridgeH::State horizontalState;					// Pobieram stany
	bridgeVertical->GetState(&verticalState);		// mostkow H
	bridgeHorizontal->GetState(&horizontalState);	//

	if(verticalState == BridgeH::State::off)
		ledIndicator->SetState(LedIndicator::State::flashLed, 0b00000011);
	if(horizontalState == BridgeH::State::off)
		ledIndicator->SetState(LedIndicator::State::flashLed, 0b00001100);

	// Jesli oba mostki sa wylaczone,
	// albo mina juz przeznaczony czas...
	if ( ((verticalState == BridgeH::State::off) && (horizontalState == BridgeH::State::off)) || ((long)(millis() - updateInfo.time_1) >= 0) )
	{
		SetState(State::idle);
	}
}

void SolarController::ManualProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (bitRead(updateInfo.data_1, 15))
	{
		ledIndicator->SetState(LedIndicator::State::off);
		bridgeVertical->SetState(BridgeH::State::off);
		bridgeHorizontal->SetState(BridgeH::State::off);

		updateInfo.data_1 = 1;				// 1 - oznacza ustawianie 'delay',   2 - 'sensitivity'...
		updateManager.Detach(this);
		return;
	}
}

void SolarController::FoldDownProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (bitRead(updateInfo.data_1, 15))
	{
		ledIndicator->SetState(LedIndicator::State::foldDown);
		updateInfo.time_1 = millis() + 7200000;
		bridgeHorizontal->SetState(BridgeH::State::working, 2);			// Wlacz mostek w prawo...
		bridgeVertical->SetState(BridgeH::State::working, 2);			// ...i w dol

		bitClear(updateInfo.data_1, 15);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::sleep);
}

void SolarController::SleepProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (bitRead(updateInfo.data_1, 15))
	{
		ledIndicator->SetState(LedIndicator::State::sleep);
		bridgeHorizontal->SetState(BridgeH::State::off);
		bridgeVertical->SetState(BridgeH::State::off);

		updateInfo.data_1 = 0;
		return;
	}

	if (sunSensor->ReadSensor(0) > nightThreshold * 2)
	{
		if (updateInfo.data_1)
		{
			if ((long)(millis() - updateInfo.time_1) >= 0)
				SetState(State::idle);
		}
		else
		{
			updateInfo.time_1 = millis() + 7200000;	// Ustawiam timer na 2 godziny do przodu
			updateInfo.data_1 = 1;
		}
	}
	else
		if(updateInfo.data_1)
			updateInfo.data_1 = 0;
}

void SolarController::Reset()
{
	DEBUG_SOLAR_CONTROLLER_PRINTLN(F("1_300"));

	delay = 10;				// Opoznienie zadzialania silnikow (w sekundach)
	sensitivity = 30;			// Wymagana roznica miedzy fotorezystorami
	overtake = 0;					// Wyprzedzanie slonca
	nightThreshold = 30;				// Prog nocy

	eepromData.delay = delay;
	eepromData.sensitivity = sensitivity;
	eepromData.overtake = overtake;
	eepromData.nightThreshold = nightThreshold;
	eeproManager.WriteData(&eepromData, SENDER_CONTROLLER);

	SetState(State::idle);
}
