#ifndef RemoteController_h
#define RemoteController_h

#include <MessageReceiver.h>
#include <RemoteKeyboard.h>
#include <RadioListener.h>
#include <RadioUsers.h>


class RemoteController : public MessageReceiver
{
public:
	RemoteController(RadioBroadcaster*);
	bool TakeMessage(int, void*, void**);

private:
	RadioBroadcaster* pRadioBroadcaster;
};

#endif
