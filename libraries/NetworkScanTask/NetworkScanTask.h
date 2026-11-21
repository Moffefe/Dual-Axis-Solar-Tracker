#ifndef NetworkScanTask_h
#define NetworkScanTask_h

#include <stdint.h>
#include <UpdateManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <TimeKeeper.h>
#include <ESP8266WiFi.h>



class NetworkScanTask : public UpdateManagerUser
{
public:
	NetworkScanTask(MessageReceiver*);
	void Update();

private:
	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};



	MessageReceiver* user;
	UpdateInfo updateInfo;
};

#endif