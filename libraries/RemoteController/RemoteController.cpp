#include "RemoteController.h"

RemoteController::RemoteController(RadioBroadcaster* pRadioBroadcaster)
{
	RemoteController::pRadioBroadcaster = pRadioBroadcaster;
	RemoteController::pRadioBroadcaster->setProtocol({ 450,{ 1, 11 },{ 1, 21 },{ 1,  2 },{ 1,  1 }, true });
	RemoteController::pRadioBroadcaster->begin(10);
}

bool RemoteController::TakeMessage(int message, void* sender, void** additionalData)
{
	if (message == MESSAGE_REMOTE_KEYBOARD_BUTTON_ACTION)
	{
		uint8_t* dataPack = (uint8_t*)additionalData;
		uint32_t buttonCode = ((uint32_t*)dataPack)[0];
		uint8_t buttonState = dataPack[4];

		if (!buttonState)
			pRadioBroadcaster->SendData(RADIO_SOLARCONTROLLER, 4, dataPack);

		return true;
	}

	return false;
}
