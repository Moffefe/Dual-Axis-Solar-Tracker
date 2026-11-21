#include "Arduino.h"
#include "LedIndicator.h"

LedIndicator::LedIndicator(PCF8574* expander, uint8_t ledUp, uint8_t ledDown, uint8_t ledLeft, uint8_t ledRight)
{
	LedIndicator::expander = expander;
	LedIndicator::ledUp = ledUp;
	LedIndicator::ledDown = ledDown;
	LedIndicator::ledLeft = ledLeft;
	LedIndicator::ledRight = ledRight;
	expander->pinMode(ledUp, OUTPUT);
	expander->pinMode(ledDown, OUTPUT);
	expander->pinMode(ledLeft, OUTPUT);
	expander->pinMode(ledRight, OUTPUT);

	currentPriority = 255;		//
	nextPriority = 255;			// Tuz po utworzeniu obiektu kompilator moze wyzerowac te pola (0 - najwyzszy priorytet, a tego nie chcê)

	SetState(State::off);
}

void LedIndicator::SetState(State newState, byte initializationData, byte priority)
{
	if ((priority <= currentPriority) && (priority <= nextPriority))
	{
		nextState = newState;
		nextPriority = priority;
		nextStateInit = 0b10000000 | initializationData;
		updateManager.Attach(this);
	}
}

void LedIndicator::SwitchState()
{
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
	SwitchState();

	switch (currentState)
	{
		case State::off:
		{
			expander->digitalWrite(ledUp, HIGH);
			expander->digitalWrite(ledDown, HIGH);
			expander->digitalWrite(ledLeft, HIGH);
			expander->digitalWrite(ledRight, HIGH);
			updateManager.Detach(this);
			return;
		}

		case State::flashLed:
		{
			if (0b01000000 & updateInfo.data_1)
			{
				if (1 & updateInfo.data_1)
					expander->digitalWrite(ledUp, LOW);
				else
					expander->digitalWrite(ledUp, HIGH);

				if (2 & updateInfo.data_1)
					expander->digitalWrite(ledDown, LOW);
				else
					expander->digitalWrite(ledDown, HIGH);

				if (4 & updateInfo.data_1)
					expander->digitalWrite(ledLeft, LOW);
				else
					expander->digitalWrite(ledLeft, HIGH);

				if (8 & updateInfo.data_1)
					expander->digitalWrite(ledRight, LOW);
				else
					expander->digitalWrite(ledRight, HIGH);
			}
			else
			{
				byte voltage = (0b00100000 & updateInfo.data_1) ? LOW : HIGH;

				if (1 & updateInfo.data_1)
					expander->digitalWrite(ledUp, voltage);
				if (2 & updateInfo.data_1)
					expander->digitalWrite(ledDown, voltage);
				if (4 & updateInfo.data_1)
					expander->digitalWrite(ledLeft, voltage);
				if (8 & updateInfo.data_1)
					expander->digitalWrite(ledRight, voltage);
			}

			updateManager.Detach(this);
			return;
		}

		case State::blink:
		{
			// Jesli jest to pierwszy
			// update od ustawienia stanu...
			if (0b10000000 & updateInfo.data_1)
			{
				expander->digitalWrite(ledUp, LOW);
				expander->digitalWrite(ledDown, LOW);
				expander->digitalWrite(ledLeft, LOW);
				expander->digitalWrite(ledRight, LOW);

				updateInfo.time_1 = millis() + 100;

				updateInfo.data_1 = 0b01000011;
				return;
			}

			if ((long)(millis() - updateInfo.time_1) >= 0)
			{
				if (0b01000000 & updateInfo.data_1)
				{
					// Aktualnie diody swieca...

					expander->digitalWrite(ledUp, HIGH);
					expander->digitalWrite(ledDown, HIGH);
					expander->digitalWrite(ledLeft, HIGH);
					expander->digitalWrite(ledRight, HIGH);

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
						expander->digitalWrite(ledUp, LOW);
						expander->digitalWrite(ledDown, LOW);
						expander->digitalWrite(ledLeft, LOW);
						expander->digitalWrite(ledRight, LOW);

						updateInfo.time_1 = millis() + 100;

						int blinks = 0b00001111 & updateInfo.data_1;
						blinks -= 1;
						updateInfo.data_1 = 0b01000000 | blinks;
					}
				}
			}

			return;
		}

		case State::foldDown:
		{
			// Jesli jest to pierwszy
			// update od ustawienia stanu...
			if (0b10000000 & updateInfo.data_1)
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
						expander->digitalWrite(ledLeft, HIGH);
						expander->digitalWrite(ledUp, LOW);
						updateInfo.time_1 = millis() + 250;
						updateInfo.data_1 = 1;
					}
					break;

				case 1:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						expander->digitalWrite(ledUp, HIGH);
						expander->digitalWrite(ledRight, LOW);
						updateInfo.time_1 = millis() + 250;
						updateInfo.data_1 = 2;
					}

				case 2:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						expander->digitalWrite(ledRight, HIGH);
						expander->digitalWrite(ledDown, LOW);
						updateInfo.time_1 = millis() + 250;
						updateInfo.data_1 = 3;
					}
					break;

				case 3:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						expander->digitalWrite(ledDown, HIGH);
						expander->digitalWrite(ledLeft, LOW);
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
			if (0b10000000 & updateInfo.data_1)
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
						expander->digitalWrite(ledUp, LOW);
						expander->digitalWrite(ledDown, LOW);
						expander->digitalWrite(ledLeft, LOW);
						expander->digitalWrite(ledRight, LOW);

						updateInfo.time_1 = millis() + 100;
						updateInfo.data_1 = 1;
					}
					break;

				case 1:
					if ((long)(millis() - updateInfo.time_1) >= 0)
					{
						expander->digitalWrite(ledUp, HIGH);
						expander->digitalWrite(ledDown, HIGH);
						expander->digitalWrite(ledLeft, HIGH);
						expander->digitalWrite(ledRight, HIGH);

						updateInfo.time_1 = millis() + 20000;
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