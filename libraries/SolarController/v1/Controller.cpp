#include "Arduino.h"
#include "Controller.h"




Controller::Controller(Configuration configuration)
{
	DEBUG_CONTROLLER_PRINTLN("Konstruktor klasy Controller");

	ready = false;										// Na razie traktujemy ten obiekt jako "niepe³nosprawny"

	if (configuration.irReceiver)
		irReceiver = configuration.irReceiver;
	else
		return;

	if (configuration.bridgeVertical)
		bridgeVertical = configuration.bridgeVertical;
	else
		return;

	if (configuration.bridgeHorizontal)
		bridgeHorizontal = configuration.bridgeHorizontal;
	else
		return;

	if (configuration.ledIndicator)
		ledIndicator = configuration.ledIndicator;
	else
		return;

	if (configuration.sunSensor)
		sunSensor = configuration.sunSensor;
	else
		return;

	DEBUG_CONTROLLER_PRINTLN("Udalo sie utworzyc obiekt klasy Controller");

	ready = true;							// Mo¿emy ju¿ TEN obiekt traktowaæ jako sprawny
	irSignalRefresh = millis();							// Lepiej nie zostawiac tego pola wyzerowanego
	Reset();															// Wejscie w stan domyslny		
}

void Controller::GetState(State* pState, word* pData) const
{
	*pState = currentState;
	*pData = updateInfo.data_1;
}

// "Zamawia" zmiane stanu
void Controller::SetState(State newState, word initializationData)
{
	nextState = newState;
	nextStateInit = 0b1000000000000000 | initializationData;

	DEBUG_CONTROLLER_PRINT("Wejscie w nowy stan: ");
	DEBUG_CONTROLLER_PRINT(nextState);
	DEBUG_CONTROLLER_PRINT("          ");
	DEBUG_CONTROLLER_PRINT("Dane inicjujace: ");
	DEBUG_CONTROLLER_PRINTLN(nextStateInit, BIN);
	DEBUG_CONTROLLER_PRINTLN("");
}

// "Realizuje" zamowienie zmiany stanu
void Controller::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void Controller::IdleProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (0b1000000000000000 & updateInfo.data_1)
	{
		ledIndicator->SetState(LedIndicator::State::off);
		bridgeVertical->TurnOff();
		bridgeHorizontal->TurnOff();

		updateInfo.data_1 = 0b0111111111111111 & updateInfo.data_1;
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

void Controller::SunAwareProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (0b1000000000000000 & updateInfo.data_1)
	{
		updateInfo.data_1 = 0;
		updateInfo.time_1 = millis() + delay;	// Czas do odpalenia silnikow

		SunSensor::SunDirection sunDirection = sunSensor->CalculateDirection();

		if (sunDirection.horizontalRatio > sensitivity)
			updateInfo.data_1 = 8 | updateInfo.data_1;
		else
		{
			if (sunDirection.horizontalRatio < -sensitivity)
				updateInfo.data_1 = 4 | updateInfo.data_1;
		}
		if (sunDirection.verticalRatio > sensitivity)
			updateInfo.data_1 = 1 | updateInfo.data_1;
		else
		{
			if (sunDirection.verticalRatio < -sensitivity)
				updateInfo.data_1 = 2 | updateInfo.data_1;
		}

		return;
	}

	SunSensor::SunDirection sunDirection = sunSensor->CalculateDirection();

	if (abs(sunDirection.horizontalRatio) > sensitivity || abs(sunDirection.verticalRatio) > sensitivity)
	{
		word newDirection = 0;

		if (sunDirection.horizontalRatio > sensitivity)
			newDirection = 8 | newDirection;
		else
		{
			if (sunDirection.horizontalRatio < -sensitivity)
				newDirection = 4 | newDirection;
		}

		if (sunDirection.verticalRatio > sensitivity)
			newDirection = 1 | newDirection;
		else
		{
			if (sunDirection.verticalRatio < -sensitivity)
				newDirection = 2 | newDirection;
		}

		if (updateInfo.data_1 != newDirection)
		{
			ledIndicator->SetState(LedIndicator::State::flashLed, 0b01000000 | newDirection);
			updateInfo.time_1 = millis() + delay;
			updateInfo.data_1 = newDirection;
		}
		else
		{
			ledIndicator->SetState(LedIndicator::State::flashLed, 0b01000000 | updateInfo.data_1);		// Upewniam sie, ze diody swieca tak jak trzeba

			if ((long)(millis() - updateInfo.time_1) >= 0)
				SetState(State::followSun, updateInfo.data_1);
		}
	}
	else
	{
		SetState(State::idle);
	}
}

void Controller::NightAwareProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (0b1000000000000000 & updateInfo.data_1)
	{
		updateInfo.time_1 = millis() + 600000;		//10min
		ledIndicator->SetState(LedIndicator::State::off);

		updateInfo.data_1 = 0b0111111111111111 & updateInfo.data_1;
		return;
	}

	if (sunSensor->ReadSensor(0) < nightThreshold)
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
void Controller::FollowSunProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (0b1000000000000000 & updateInfo.data_1)
	{
		updateInfo.time_1 = millis() + 60000;		// Ustawiam czas na minute do przodu (masz minute na przestawienie)

		if (1 & updateInfo.data_1)
			bridgeVertical->TurnOn(1);
		else
		{
			if (2 & updateInfo.data_1)
				bridgeVertical->TurnOn(2);
		}

		if (4 & updateInfo.data_1)
			bridgeHorizontal->TurnOn(1);
		else
		{
			if (8 & updateInfo.data_1)
				bridgeHorizontal->TurnOn(2);
		}

		updateInfo.data_1 = 0b0111111111111111 & updateInfo.data_1;
		return;
	}

	SunSensor::SunDirection sunDirection = sunSensor->CalculateDirection();

	if ( abs(sunDirection.verticalRatio) > (sensitivity -5))
	{
		if( (1 & updateInfo.data_1) && (sunDirection.verticalRatio < 0) )
			bridgeVertical->TurnOff();												// Niestety, minelismy cel
		else
		{
			if ((2 & updateInfo.data_1) && (sunDirection.verticalRatio > 0))
				bridgeVertical->TurnOff();											// Niestety, minelismy cel
		}
	}
	else
	{
		bridgeVertical->TurnOff();			// Roznica miedzy tymi 2 sensorami juz jest taka mala, ze wystarczy
	}

	if ( abs(sunDirection.horizontalRatio) > (sensitivity - 5))
	{
		if ((4 & updateInfo.data_1) && (sunDirection.horizontalRatio > 0))
			bridgeHorizontal->TurnOff();
		else
		{
			if ((8 & updateInfo.data_1) && (sunDirection.horizontalRatio < 0))
				bridgeHorizontal->TurnOff();
		}
	}
	else
	{
		bridgeHorizontal->TurnOff();		// Roznica miedzy tymi 2 sensorami juz jest taka mala, ze wystarczy
	}

	if (bridgeVertical->GetState() == 0)
		ledIndicator->SetState(LedIndicator::State::flashLed, 0b00000011);
	if (bridgeHorizontal->GetState() == 0)
		ledIndicator->SetState(LedIndicator::State::flashLed, 0b00001100);

	if ( ((bridgeVertical->GetState() == 0) && (bridgeHorizontal->GetState() == 0)) ||
		 ((long)(millis() - updateInfo.time_1) >= 0) )
	{
		SetState(State::idle);
	}
}

void Controller::ManualProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (0b1000000000000000 & updateInfo.data_1)
	{
		ledIndicator->SetState(LedIndicator::State::off);
		bridgeVertical->TurnOff();
		bridgeHorizontal->TurnOff();

		updateInfo.data_1 = 1;				// 1 - oznacza ustawianie 'delay',   2 - 'sensitivity'...
		return;
	}

	switch (currentIrSignal.value)
	{
		case BUTTON_PLAY:
		{
			SetState(State::idle);
			break;
		}

		case BUTTON_UP:
		{
			if (bridgeVertical->GetState() != 0b00000001)
				bridgeVertical->TurnOn(1);
			else
				bridgeVertical->TurnOff();

			break;
		}

		case BUTTON_DOWN:
		{
			if (bridgeVertical->GetState() != 0b00000010)
				bridgeVertical->TurnOn(2);
			else
				bridgeVertical->TurnOff();

			break;
		}

		case BUTTON_LEFT:
		{
			if (bridgeHorizontal->GetState() != 0b00000001)
				bridgeHorizontal->TurnOn(1);
			else
				bridgeHorizontal->TurnOff();

			break;
		}

		case BUTTON_RIGHT:
		{
			if (bridgeHorizontal->GetState() != 0b00000010)
				bridgeHorizontal->TurnOn(2);
			else
				bridgeHorizontal->TurnOff();

			break;
		}

		case BUTTON_1:
		{
			updateInfo.data_1 = 1;
			break;
		}

		case BUTTON_2:
		{
			updateInfo.data_1 = 2;
			break;
		}

		case BUTTON_3:
		{
			updateInfo.data_1 = 3;
			break;
		}

		case BUTTON_4:
		{
			updateInfo.data_1 = 4;
			break;
		}

		case BUTTON_5:
		{
			updateInfo.data_1 = 5;
			break;
		}

		case BUTTON_VOLUME_UP:
		{
			switch (updateInfo.data_1)
			{
				case 1:
					delay += 1000;			// + 1 sekunda
					DEBUG_CONTROLLER_PRINT("Zwiekszono opoznienie, wynosi ono teraz ");
					DEBUG_CONTROLLER_PRINT(delay);
					DEBUG_CONTROLLER_PRINTLN(" milisekund");
					break;

				case 2:
					sensitivity += 1;
					DEBUG_CONTROLLER_PRINT("Zmniejszono czulosc, wynosi ona teraz ");
					DEBUG_CONTROLLER_PRINTLN(sensitivity);
					break;

				case 3:
					nightThreshold += 1;
					DEBUG_CONTROLLER_PRINT("Zwiekszono prog nocy, wynosi on teraz ");
					DEBUG_CONTROLLER_PRINTLN(nightThreshold);
					break;

				case 4:
					sunSensor->SetDrag(sunSensor->GetDrag() + 100);
					DEBUG_CONTROLLER_PRINT("Zwiekszono bezwladnosc, wynosi ona teraz ");
					DEBUG_CONTROLLER_PRINT(sunSensor->GetDrag());
					DEBUG_CONTROLLER_PRINTLN(" milisekund");
					break;

				case 5:
					overtake += 1;
					DEBUG_CONTROLLER_PRINT("Zwiekszono wyprzedzenie, wynosi ono teraz ");
					DEBUG_CONTROLLER_PRINTLN(overtake);
					break;
			}

			break;
		}

		case BUTTON_VOLUME_DOWN:
		{
			switch (updateInfo.data_1)
			{
				case 1:
					delay -= 1000;			// - 1 sekunda

					if (delay < 0)			//
						delay = 0;			// Walidacja danych

					DEBUG_CONTROLLER_PRINT("Zmniejszono opoznienie, wynosi ono teraz ");
					DEBUG_CONTROLLER_PRINT(delay);
					DEBUG_CONTROLLER_PRINTLN(" milisekund");
					break;

				case 2:
					sensitivity -= 1;

					if (sensitivity < 0)
						sensitivity = 0;

					DEBUG_CONTROLLER_PRINT("Zwiekszono czulosc, wynosi ona teraz ");
					DEBUG_CONTROLLER_PRINTLN(sensitivity);
					break;

				case 3:
					nightThreshold -= 1;

					if (nightThreshold < 0)
						nightThreshold = 0;

					DEBUG_CONTROLLER_PRINT("Zmniejszono prog nocy, wynosi on teraz ");
					DEBUG_CONTROLLER_PRINTLN(nightThreshold);
					break;

				case 4:
					sunSensor->SetDrag(sunSensor->GetDrag() - 100);		//Bez walidacji, bo robi ja klasa SunSensor
					DEBUG_CONTROLLER_PRINT("Zmniejszono bezwladnosc, wynosi ona teraz ");
					DEBUG_CONTROLLER_PRINT(sunSensor->GetDrag());
					DEBUG_CONTROLLER_PRINTLN(" milisekund");
					break;

				case 5:
					overtake -= 1;			// Bez walidacji ([+]wyprzedzenie , opoznienie[-])
					DEBUG_CONTROLLER_PRINT("Zmniejszono wyprzedzenie, wynosi ono teraz ");
					DEBUG_CONTROLLER_PRINTLN(overtake);
					break;
			}

			break;
		}
	}
}

void Controller::FoldDownProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (0b1000000000000000 & updateInfo.data_1)
	{
		ledIndicator->SetState(LedIndicator::State::foldDown);
		updateInfo.time_1 = millis() + 900000;						// 15 min wystarcza aby sie zlozyc
		bridgeHorizontal->TurnOn(1);										// Wlacz mostek w lewo...
		bridgeVertical->TurnOn(2);											// ...i w dol

		updateInfo.data_1 = 0b0111111111111111 & updateInfo.data_1;
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::sleep);
}

void Controller::SleepProcedure()
{
	// Jesli jest to pierwszy
	// update od ustawienia stanu
	if (0b1000000000000000 & updateInfo.data_1)
	{
		ledIndicator->SetState(LedIndicator::State::sleep);
		bridgeHorizontal->TurnOff();
		bridgeVertical->TurnOff();

		updateInfo.data_1 = 0b0111111111111111 & updateInfo.data_1;
		return;
	}

	if (sunSensor->ReadSensor(0) > nightThreshold*2)
	{
		SetState(State::idle);
		return;
	}
}

void Controller::Reset()
{
	delay = 10000;			// Opoznienie zadzialania silnikow
	sensitivity = 30;			// Wymagana roznica miedzy fotorezystorami
	overtake = 0;					// Wyprzedzanie slonca
	nightThreshold = 30;				// Prog nocy

	sunSensor->SetDrag(100);

	SetState(State::idle);
}

void Controller::Update()
{
	if (ready)
	{
		SwitchState();						// Tylko tu wolno wywo³ac ta funkcje

		if ((long)(millis() - irSignalRefresh) >= 0)
		{
			if (irReceiver->decode(&currentIrSignal))
			{
				ledIndicator->SetState(LedIndicator::State::blink, 0, 254);
				irReceiver->resume();
			}

			irSignalRefresh = millis() + 250;		// Nastepny odczyt IR nastapi za 250ms
		}

		switch (currentIrSignal.value)
		{
			case BUTTON_ON:
			{
				Reset();
				break;
			}

			case BUTTON_STOP:
			{
				SetState(State::manual);
				break;
			}
		}

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

		currentIrSignal.value = 0;			// Zeby nastepne wywolanie Update() nie szlo, z takim samym sygnalem IR
		ledIndicator->Update();
	}
}