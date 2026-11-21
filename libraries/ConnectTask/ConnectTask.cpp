#include "Arduino.h"
#include "ConnectTask.h"

ConnectTask::ConnectTask(const String& ssid, const String& password, const String& apChannel, MessageReceiver* user) :
	ssid(ssid),
	password(password),
	apChannel(apChannel),
	user(user)
{
	SetState(State::disconnect);
}

void ConnectTask::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::disconnect:
			DisconnectProcedure();
			break;

		case State::connectAp:
			ConnectApProcedure();
			break;

		case State::createAp:
			CreateApProcedure();
			break;

		case State::end:
			EndProcedure();
			break;
	}
}

void ConnectTask::SetState(State newState, uint8_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);
}

void ConnectTask::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void ConnectTask::DisconnectProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		WiFi.mode(WIFI_OFF);
		updateInfo.time_1 = millis() + 2000;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		if (ssid.length() > 0 && password.length() > 0)
			SetState(State::connectAp);
		else
			SetState(State::createAp);
	}
}

void ConnectTask::ConnectApProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		DEBUG_CONNECT_TASK_PRINTLN("");
		DEBUG_CONNECT_TASK_PRINT(F("ConnectTask: Wykryto dane dostepowe do AP, laczenie..."));
		DEBUG_CONNECT_TASK_PRINTLN("");

		WiFi.mode(WIFI_STA);
		WiFi.begin(ssid.c_str(), password.c_str());
		updateInfo.data_1 = 60;
		updateInfo.time_1 = millis() + 500;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		if (WiFi.status() == WL_CONNECTED)
		{
			DEBUG_CONNECT_TASK_PRINTLN("");
			DEBUG_CONNECT_TASK_PRINT(F("ConnectTask: Udalo sie polaczyc z AP "));
			DEBUG_CONNECT_TASK_PRINT(WiFi.SSID());
			DEBUG_CONNECT_TASK_PRINT(F(", adres IP to "));
			DEBUG_CONNECT_TASK_PRINT(WiFi.localIP());
			DEBUG_CONNECT_TASK_PRINTLN("");

			uint8_t* additionalData = new uint8_t;
			*additionalData = 1;
			user->TakeMessage(MESSAGE_CONNECT_TASK_END, this, (void**)additionalData);
			SetState(State::end);
		}
		else
		{
			--updateInfo.data_1;
			if (updateInfo.data_1)
			{
				updateInfo.time_1 = millis() + 500;
			}
			else
			{
				DEBUG_CONNECT_TASK_PRINTLN("");
				DEBUG_CONNECT_TASK_PRINT(F("ConnectTask: Nie udalo sie polaczyc z AP"));
				DEBUG_CONNECT_TASK_PRINTLN("");

				uint8_t* additionalData = new uint8_t;
				*additionalData = 2;
				user->TakeMessage(MESSAGE_CONNECT_TASK_END, this, (void**)additionalData);
				SetState(State::end);
			}
		}
	}
}

void ConnectTask::CreateApProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		String tempString_1;
		tempString_1 += F("TRACKER_SETUP_AP");
		WiFi.mode(WIFI_AP_STA);
		WiFi.softAP(tempString_1.c_str(), "SET_AP_PASSWORD", apChannel.toInt());
		updateInfo.time_1 = millis() + 2000;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		DEBUG_CONNECT_TASK_PRINTLN("");
		DEBUG_CONNECT_TASK_PRINT(F("ConnectTask: Utworzono wlasne AP na kanale "));
		DEBUG_CONNECT_TASK_PRINT(apChannel);
		DEBUG_CONNECT_TASK_PRINTLN("");

		uint8_t* additionalData = new uint8_t;
		*additionalData = 0;
		user->TakeMessage(MESSAGE_CONNECT_TASK_END, this, (void**)additionalData);
		SetState(State::end);
	}
}

void ConnectTask::EndProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		updateManager.Detach(this);
		timeKeeper.RequestDelete(millis() + 100, this);
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}
