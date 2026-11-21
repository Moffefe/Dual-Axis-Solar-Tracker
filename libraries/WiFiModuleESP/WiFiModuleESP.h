/*
   WiFiModuleESP - Klasa odpowiedzialna za komunikacje sieciowa Sterownika Solarnego, dzialajaca
   po stronie ESP8266.
   Filozof404
   Domena publiczna
   Wersja 2
 */
#ifndef WiFiModuleESP_h
#define WiFiModuleESP_h

#define DEBUG_WIFIMODULEESP 1
#if DEBUG_WIFIMODULEESP
#define DEBUG_WIFIMODULEESP_LOG(...) serialListener.Log(__VA_ARGS__)
//#define DEBUG_WIFIMODULEESP_LOG(...) Serial1.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <UpdateManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <TimeKeeper.h>
#include <SerialListener.h>
#include <SerialUsers.h>
#include <ESP8266WiFi.h>
#define FS_NO_GLOBALS
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#define ENABLE_ARDUINO_FEATURES	1
#include <SdFat.h>
#include <LinkedList.h>
#include <mimetable.h>
#include <ConnectTask.h>
#include <NetworkScanTask.h>
using namespace mime;



class WiFiModuleESP : public UpdateManagerUser, public MessageReceiver
{
public:
	WiFiModuleESP(SdFat&);
	void Update();
	bool TakeMessage(int, void*, void**);
	void CallbackNotFound(AsyncWebServerRequest*);						//
	size_t CallbackNotFoundFiller(uint16_t, uint8_t*, size_t, size_t);	// 404
	void CallbackNotFoundDisconnect(uint16_t);							//
	void CallbackFormat(AsyncWebServerRequest*);							// format
	void CallbackWeakFormat(AsyncWebServerRequest*);							// weak-format
	void CallbackShutdown(AsyncWebServerRequest*);									// shutdown
	void CallbackRestart(AsyncWebServerRequest*);										// restart
	void CallbackUpload(AsyncWebServerRequest*);												// upload
	void CallbackUploadBodyHandler(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t);	//
	void CallbackSetTime(AsyncWebServerRequest*);													// set-time
	void CallbackSetNetworkCredentials(AsyncWebServerRequest*);											// set-network-credentials
	void CallbackSetApChannel(AsyncWebServerRequest*);														// set-ap-channel
	void CallbackNetworkScan(AsyncWebServerRequest*);					// network-scan
	void CallbackStartCollectingLogs(AsyncWebServerRequest*);				// start-collecting-logs
	void CallbackStopCollectingLogs(AsyncWebServerRequest*);					// stop-collecting-logs
	void CallbackListDirectory(AsyncWebServerRequest*);								// list-directory
	void CallbackSwitchToAuto(AsyncWebServerRequest*);					//
	void CallbackSwitchToManual(AsyncWebServerRequest*);				//
	void CallbackGetStatus(AsyncWebServerRequest*);						// Komunikacja z Atmega
	void CallbackSetParameters(AsyncWebServerRequest*);					//
	void CallbackControlMotor(AsyncWebServerRequest*);					//
	void CallbackAsyncRequestResult(AsyncWebServerRequest*);				// async-request-result
	static void WeakFormat();

private:
	enum State
	{
		wait,
		start,
		offline,
		online
	};

	struct UpdateInfo
	{
		unsigned long time_1;
		uint32_t data_1;
	};

	struct AssetFileName
	{
		String shortName;
		String realName;
	};

	struct Config
	{
		String ssid;
		String password;
		String connectRetries;
		String connectRetriesCurrent;
		String apChannel;
	};

	struct UploadRequest
	{
		File file;
		String fileName;			// Nie chce uzywac file.name() ze wzgledu na to, ze w CallbackUploadBodyHandler() wielokrotnie bedzie trzeba porownywac nazwe pliku z parametrem zadania, ktory jest objektem String
		size_t targetFileSize;			// Calkowity rozmiar pliku w bajtach (jaki bedzie po skonczeniu pobierania)
		unsigned long timeOut;				// Czas (punkt w przyszlosci) po ktorym przerywamy transfer pliku
		bool active;							// Zadanie upload'u pliku jest wazne tylko gdy jest ustawione na true (Synchronizator pomiedzy petla glowna, a przerwaniami)



		UploadRequest()
		{
			fileName = "";
			targetFileSize = 0;
			timeOut = 0;
			active = false;
		}

		void BeginUpload(const String& fileName, const size_t targetFileSize)
		{
			if (active)
				EndUpload();

			file.open(fileName.c_str(), O_WRITE | O_CREAT | O_TRUNC);
			UploadRequest::fileName = fileName;
			UploadRequest::targetFileSize = targetFileSize;
			timeOut = millis() + 150000;						// 2min 30sek
			active = true;
		}

		void EndUpload()
		{
			active = false;

			if (file)
			{
				if (file.size() != targetFileSize)
					file.remove();

				file.close();
			}
		}
	};

	struct DownloadRequest
	{
		static uint16_t nextFreeKey;		// Dzieki niej kazdy nowy obiekt otrzymuje niepowtarzalny identyfikator (przewiniecie nie jest problemem)
		uint16_t key;							// Klucz przypisany do jednego zadania downloadu. Umozliwia przeszukanie listy 'downloadRequests' wewnatrz metody CallbackNotFoundFiller()
		String fileName;
		bool justStarted;



		DownloadRequest(const String& fileName)
		{
			key = nextFreeKey;
			++nextFreeKey;
			DownloadRequest::fileName = fileName;
			justStarted = true;
		}
	};

	struct AsyncRequest
	{
		bool locked;
		String url;
		unsigned long invokeTime;
		String result;



		AsyncRequest()
		{
			Reset();
		}

		void Reset()
		{
			locked = false;
			url = "";
			invokeTime = 0;
			result = "";
		}
	};



	AsyncWebServer* pServer;
	SdFat& sdFat;
	Ivanseidel::LinkedList<AssetFileName*> assetFileNames;
	Config config;
	UploadRequest uploadRequest;
	Ivanseidel::LinkedList<DownloadRequest*> downloadRequests;
	AsyncRequest asyncRequest;
	uint16_t semaphores;
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint32_t nextStateInit;



	void SetState(State, uint32_t = 0);
	void SwitchState();
	void WaitProcedure();
	void StartProcedure();
	void OfflineProcedure();
	void OnlineProcedure();
	void CreateServer();
	static void SuccessResponse(AsyncWebServerRequest*, String* = NULL);
	static void ErrorResponse(AsyncWebServerRequest*, String* = NULL);
	bool BeginAsyncRequest(AsyncWebServerRequest*);
	static String GetContentType(const String&);
	static bool WeakFormat(File&, Ivanseidel::LinkedList<String>&, char*, uint16_t);
	void PrepareMassMemory();
	void CloseMassMemory();
	static bool ReadLineFromFile(File&, char*, size_t);
	void ReadConfigFile();
	void WriteConfigFile();
	bool TryAcquireSemaphore(uint8_t);
	void ReleaseSemaphore(uint8_t);
};

#endif
