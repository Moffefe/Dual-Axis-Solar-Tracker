#include "Arduino.h"
#include "NetworkScanTask.h"



NetworkScanTask::NetworkScanTask(MessageReceiver* user) :
	user(user)
{
	updateManager.Attach(this);
}

void NetworkScanTask::Update()
{
	if (bitRead(updateInfo.data_1, 0))
	{
		if ((long)(millis() - updateInfo.time_1) >= 0)
		{
			user->TakeMessage(MESSAGE_NETWORK_SCAN_TASK_END, this, NULL);
			updateManager.Detach(this);
			timeKeeper.RequestDelete(millis() + 100, this);
		}
	}
	else
	{
		WiFi.scanNetworks(true, true);
		updateInfo.time_1 = millis() + 10000;
		bitSet(updateInfo.data_1, 0);
	}
}
