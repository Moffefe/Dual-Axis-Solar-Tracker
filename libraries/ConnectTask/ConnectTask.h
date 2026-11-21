#ifndef ConnectTask_h
#define ConnectTask_h

#define DEBUG_CONNECT_TASK 0
#if DEBUG_CONNECT_TASK
#	define DEBUG_CONNECT_TASK_PRINT(...)    Serial1.print(__VA_ARGS__)
#	define DEBUG_CONNECT_TASK_PRINTLN(...)  Serial1.println(__VA_ARGS__)
#else
#	define DEBUG_CONNECT_TASK_PRINT(...)
#	define DEBUG_CONNECT_TASK_PRINTLN(...)
#endif

#include <stdint.h>
#include <UpdateManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <TimeKeeper.h>
#include <ESP8266WiFi.h>



class ConnectTask : public UpdateManagerUser
{
public:
	ConnectTask(const String&, const String&, const String&, MessageReceiver*);
	void Update();

private:
	enum State
	{
		disconnect,
		connectAp,
		createAp,
		end
	};

	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};



	const String& ssid;
	const String& password;
	const String& apChannel;
	MessageReceiver* user;
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint8_t nextStateInit;



	void SetState(State, uint8_t = 0);
	void SwitchState();
	void DisconnectProcedure();
	void ConnectApProcedure();
	void CreateApProcedure();
	void EndProcedure();
};

#endif