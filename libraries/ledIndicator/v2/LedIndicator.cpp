#include "Arduino.h"
#include "LedIndicator.h"



LedIndicator::LedIndicator(int ledUp, int ledDown, int ledLeft, int ledRight)
{
	LedIndicator::ledUp = ledUp;
	LedIndicator::ledDown = ledDown;
	LedIndicator::ledLeft = ledLeft;
	LedIndicator::ledRight = ledRight;
	pinMode(ledUp, OUTPUT);
	pinMode(ledDown, OUTPUT);
	pinMode(ledLeft, OUTPUT);
	pinMode(ledRight, OUTPUT);
	
	currentPriority = 255;		//
	nextPriority = 255;			// Tuz po utworzeniu obiektu kompilator moze wyzerowac te pola (0 - najwyzszy priorytet, a tego nie chcê)

	SetState(State::off);
}

void LedIndicator::SetState(State newState, byte initializationData, byte priority)
{
	// "Zamowienie" na nowy stan zostanie przyjete:
	// 1) jesli zamawiany stan jest wazniejszy od aktualnego
	// 2) jesli nie zamówiono jak dot¹d stanu wazniejszego
	if ( (priority <= currentPriority) && (priority <= nextPriority) )
	{
		nextState = newState;
		nextPriority = priority;
		nextStateInit = 0b10000000 | initializationData;
	}
}

void LedIndicator::SwitchState()
{
	// "Zamowienie" na nowy stan wykrywam
	// gdy ta zmienna nie rowna sie zero
	if (nextStateInit)
	{
		currentState = nextState;
		currentPriority = nextPriority;
		updateInfo.data_1 = nextStateInit;

		nextPriority = 255;
		nextStateInit = 0;
	}
}

void LedIndicator::Update()
{
	SwitchState();				// Obiekt moze zmienic stan tylko na poczatku Update(), nie w srodku

	switch(currentState)
	{
		case State::off:
		{
			// Jesli jest to pierwszy
			// update od ustawienia stanu...
			if(0b10000000 & updateInfo.data_1)
			{
				digitalWrite(ledUp, LOW);
				digitalWrite(ledDown, LOW);
				digitalWrite(ledLeft, LOW);
				digitalWrite(ledRight, LOW);

				updateInfo.data_1 = 0b01111111 & updateInfo.data_1;	// Odnotowuje start tego stanu
				return;
			}
			
			return;
		}
		
		case State::flashLed:
		{
			// Jesli jest to pierwszy
			// update od ustawienia stanu...
			if(0b10000000 & updateInfo.data_1)
			{
				if (0b01000000 & updateInfo.data_1)
				{
					if (1 & updateInfo.data_1)
						digitalWrite(ledUp, HIGH);
					else
						digitalWrite(ledUp, LOW);

					if (2 & updateInfo.data_1)
						digitalWrite(ledDown, HIGH);
					else
						digitalWrite(ledDown, LOW);

					if (4 & updateInfo.data_1)
						digitalWrite(ledLeft, HIGH);
					else
						digitalWrite(ledLeft, LOW);

					if (8 & updateInfo.data_1)
						digitalWrite(ledRight, HIGH);
					else
						digitalWrite(ledRight, LOW);
				}
				else
				{
					byte voltage = (0b00100000 & updateInfo.data_1) ? HIGH : LOW;

					if (1 & updateInfo.data_1)
						digitalWrite(ledUp, voltage);
					if (2 & updateInfo.data_1)
						digitalWrite(ledDown, voltage);
					if (4 & updateInfo.data_1)
						digitalWrite(ledLeft, voltage);
					if (8 & updateInfo.data_1)
						digitalWrite(ledRight, voltage);
				}

				updateInfo.data_1 = 0b01111111 & updateInfo.data_1;
				return;
			}
			
			return;
		}
		
		case State::blink:
		{
			// Jesli jest to pierwszy
			// update od ustawienia stanu...
			if(0b10000000 & updateInfo.data_1)
			{
				digitalWrite(ledUp, HIGH);
				digitalWrite(ledDown, HIGH);
				digitalWrite(ledLeft, HIGH);
				digitalWrite(ledRight, HIGH);

				updateInfo.time_1 = millis() + 100;

				updateInfo.data_1 = 0b01000011;
				return;
			}

			if ((long)(millis() - updateInfo.time_1) >= 0)
			{
				if (0b01000000 & updateInfo.data_1)
				{
					// Aktualnie diody swieca...

					digitalWrite(ledUp, LOW);
					digitalWrite(ledDown, LOW);
					digitalWrite(ledLeft, LOW);
					digitalWrite(ledRight, LOW);

					updateInfo.time_1 = millis() + 38;

					updateInfo.data_1 = 0b10111111 & updateInfo.data_1;
				}
				else
				{
					// Aktualnie diody nie swieca...

					if ((0b00001111 & updateInfo.data_1) == 0)
					{
						currentPriority = 255;			// Mrugnolem juz wystarczajaca ilosc razy, wiec zadanie wykonane
						SetState(State::off);
						return;
					}
					else
					{
						digitalWrite(ledUp, HIGH);
						digitalWrite(ledDown, HIGH);
						digitalWrite(ledLeft, HIGH);
						digitalWrite(ledRight, HIGH);

						updateInfo.time_1 = millis() + 100;

						int blinks = 0b00001111 & updateInfo.data_1;
						blinks -= 1;
						updateInfo.data_1 = 0b01000000 | blinks;
					}
				}
			}
		}

		case State::foldDown:
		{
			// Jesli jest to pierwszy
			// update od ustawienia stanu...
			if(0b10000000 & updateInfo.data_1)
			{
				updateInfo.time_1 = millis();
				updateInfo.data_1 = 0;

				return;
			}
			
			switch (updateInfo.data_1)
			{
				case 0:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						digitalWrite(ledLeft, LOW);
						digitalWrite(ledUp, HIGH);
						updateInfo.time_1 = millis() + 250;
						updateInfo.data_1 = 1;
					}
					break;

				case 1:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						digitalWrite(ledUp, LOW);
						digitalWrite(ledRight, HIGH);
						updateInfo.time_1 = millis() + 250;
						updateInfo.data_1 = 2;
					}

				case 2:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						digitalWrite(ledRight, LOW);
						digitalWrite(ledDown, HIGH);
						updateInfo.time_1 = millis() + 250;
						updateInfo.data_1 = 3;
					}
					break;

				case 3:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						digitalWrite(ledDown, LOW);
						digitalWrite(ledLeft, HIGH);
						updateInfo.time_1 = millis() + 250;
						updateInfo.data_1 = 0;
					}
					break;
			}

			return;
		}
		
		case State::sleep:
		{
			// Jesli jest to pierwszy
			// update od ustawienia stanu
			if(0b10000000 & updateInfo.data_1)
			{
				updateInfo.time_1 = millis();
				updateInfo.data_1 = 0;

				return;
			}
			
			switch (updateInfo.data_1)
			{
				case 0:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						digitalWrite(ledUp, LOW);
						digitalWrite(ledDown, LOW);
						digitalWrite(ledLeft, LOW);
						digitalWrite(ledRight, LOW);

						updateInfo.time_1 = millis() + 5000;
						updateInfo.data_1 = 1;
					}
					break;

				case 1:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						digitalWrite(ledUp, HIGH);
						digitalWrite(ledDown, HIGH);
						digitalWrite(ledLeft, HIGH);
						digitalWrite(ledRight, HIGH);

						updateInfo.time_1 = millis() + 100;
						updateInfo.data_1 = 0;
					}
					break;
			}

			return;
		}

		default:
		{
			SetState(State::off);
			return;
		}
	}
}